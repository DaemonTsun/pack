
#include "shl/assert.hpp"
#include "shl/error.hpp"
#include "shl/defer.hpp"
#include "shl/memory.hpp"
#include "shl/streams.hpp"
#include "pack/package.hpp"
#include "pack/pack_writer.hpp"

void init(pack_writer_entry *entry)
{
    assert(entry != nullptr);

    fill_memory(entry, 0);
    init(&entry->name);
}

void free(pack_writer_entry *entry)
{
    assert(entry != nullptr);

    free(&entry->name);

    if (entry->type == pack_writer_entry_type::Memory)
        free(&entry->memory);
}

void init(pack_writer *writer)
{
    assert(writer != nullptr);

    init(&writer->entries);
}

void free(pack_writer *writer)
{
    assert(writer != nullptr);

    free<true>(&writer->entries);
}

bool pack_writer_add_file(pack_writer *writer, const char *path, bool lazy, error *err)
{
    return pack_writer_add_file(writer, path, path, lazy, err);
}

bool pack_writer_add_file(pack_writer *writer, const char *path, const char *name, bool lazy, error *err)
{
    assert(writer != nullptr);
    assert(path != nullptr);

    pack_writer_entry *entry = add_at_end(&writer->entries);
    init(entry);
    entry->flags = PACK_TOC_FLAG_FILE;
    copy_string(name, &entry->name);

    file_stream stream{};

    if (!init(&stream, path, open_mode::Read, err))
        return false;

    defer { free(&stream); };

    s64 fsize = get_file_size(&stream, err);

    if (fsize <= 0)
        return false;

    if (lazy)
    {
        entry->type = pack_writer_entry_type::File;
        entry->file.path = path;
        entry->file.size = fsize;
    }
    else
    {
        entry->type = pack_writer_entry_type::Memory;
        init(&entry->memory, fsize);

        if (!read_entire_file(&stream, entry->memory.data, fsize, err))
            return false;
    }

    return true;
}

void pack_writer_add_entry(pack_writer *writer, pack_writer_entry *entry)
{
    assert(writer != nullptr);
    assert(entry  != nullptr);

    pack_writer_entry *x = add_at_end(&writer->entries);
    *x = *entry;
}

void pack_writer_add_entry(pack_writer *writer, const char *str, const char *name)
{
    assert(writer != nullptr);
    assert(str != nullptr);
    assert(name != nullptr);

    pack_writer_entry *entry = add_at_end(&writer->entries);
    init(entry);
    entry->flags = PACK_TOC_NO_FLAGS;
    entry->type = pack_writer_entry_type::Memory;
    copy_string(name, &entry->name);

    s64 len = string_length(str);
    init(&entry->memory, len);
    copy_memory(str, entry->memory.data, len);
}

void pack_writer_add_entry(pack_writer *writer, void *data, s64 size, const char *name)
{
    assert(writer != nullptr);
    assert(data != nullptr);
    assert(name != nullptr);

    pack_writer_entry *entry = add_at_end(&writer->entries);
    init(entry);
    entry->flags = PACK_TOC_NO_FLAGS;
    entry->type = pack_writer_entry_type::Memory;
    copy_string(name, &entry->name);

    init(&entry->memory, size);
    copy_memory(data, entry->memory.data, size);
}

static bool _write_entry(file_stream *out, pack_writer_entry *entry, error *err)
{
    if (entry->type == pack_writer_entry_type::Memory)
    {
        write(out, entry->memory.data, entry->memory.size);
    }
    else
    {
        // Lazy file writing, we read the entire file to memory then just write that
        file_stream stream{};
        defer { free(&stream); };

        if (!init(&stream, entry->file.path, open_mode::Read, err))
            return false;

        s64 size = get_file_size(&stream, err);

        if (size < 0)
            return false;
        
        memory_stream mem{};
        defer { free(&mem); };

        if (!read_entire_file(&stream, &mem, err))
            return false;

        write(out, mem.data, mem.size);
    }

    return true;
}

inline static s64 _entry_size(pack_writer_entry *entry)
{
    if (entry->type == pack_writer_entry_type::Memory)
        return entry->memory.size;
    else
        return entry->file.size;
}

bool pack_writer_write_to_file(pack_writer *writer, const char *out_path, error *err)
{
    assert(writer != nullptr);
    assert(out_path != nullptr);

    io_handle h = io_open(out_path, open_mode::WriteTrunc, err);
    defer { io_close(h); };

    if (h == INVALID_IO_HANDLE)
        return false;

    return pack_writer_write_to_file(writer, h, 0, err);
}

bool pack_writer_write_to_file(pack_writer *writer, io_handle h, s64 offset, error *err)
{
    assert(writer != nullptr);
    assert(h != INVALID_IO_HANDLE);

    /* This writes everything directly to the handle because if the entries
       are quite large, building up a single buffer to write to the handle at
       once would be quite difficult.
       So for simplicity, this just writes directly to the handle.
    */

    s64 entry_count = writer->entries.size;

    package_header header{};
    copy_string(PACK_HEADER_MAGIC, header.magic, 4);
    header.version = PACK_VERSION;
    header.flags = PACK_NO_FLAGS;

    header.toc_offset = 0;   // placeholder
    header.names_offset = 0; // placeholder
    header.names_size = 0;   // placeholder
    
    file_stream _out{};
    _out.handle = h;
    file_stream *out = &_out;

    if (io_seek(h, offset, IO_SEEK_SET, err) < 0)
        return false;

    // write header
    if (write(out, &header, err) < 0)
        return false;

    // write the entry contents
    array<s64> content_offsets{};
    init(&content_offsets, entry_count);
    defer { free(&content_offsets); };

    for (s64 i = 0; i < entry_count; ++i)
    {
        pack_writer_entry *entry = writer->entries.data + i;
        content_offsets[i] = tell(out, err);

        if (!_write_entry(out, entry, err))
            return false;

        if (seek_next_alignment(out, 8, err) < 0)
            return false;
    }

    // write the name table
    s64 name_table_pos = tell(out, err);

    if (name_table_pos < 0)
        return false;

    if (write_at(out, &name_table_pos, offset_of(package_header, names_offset), err) < 0)
        return false;

    if (seek(out, name_table_pos, IO_SEEK_SET, err) < 0)
        return false;

    array<s64> name_offsets{};
    init(&name_offsets, entry_count);
    defer { free(&name_offsets); };

    for (s64 i = 0; i < entry_count; ++i)
    {
        name_offsets[i] = tell(out, err);

        if (write(out, (const char*)(writer->entries[i].name), err) < 0)
            return false;

        if (write(out, "\0", 1, err) < 0)
            return false;
    }

    s64 npos = tell(out, err);
    
    if (npos < 0)
        return false;

    assert(name_table_pos <= npos);
    s64 name_table_size = npos - name_table_pos;

    if (write_at(out, &name_table_size, offset_of(package_header, names_size), err) < 0)
        return false;

    if (seek(out, npos, IO_SEEK_SET, err) < 0)
        return false;

    if (seek_next_alignment(out, 8) < 0)
        return false;

    // write the toc
    s64 toc_pos = tell(out, err);

    if (toc_pos < 0)
        return false;

    if (write_at(out, &toc_pos, offset_of(package_header, toc_offset), err) < 0)
        return false;

    if (seek(out, toc_pos, IO_SEEK_SET, err) < 0)
        return false;

    package_toc toc{};
    copy_string(PACK_TOC_MAGIC, toc.magic, 4);
    toc._padding = 0;
    toc.entry_count = entry_count;

    if (write(out, &toc, err) < 0)
        return false;

    // u64 toc_entry_pos = toc_pos + sizeof(toc);
    
    for (s64 i = 0; i < entry_count; ++i)
    {
        pack_writer_entry *entry = writer->entries.data + i;
        package_toc_entry toc_entry{};
        toc_entry.offset = content_offsets[i];
        toc_entry.size = _entry_size(entry);
        toc_entry.name_offset = name_offsets[i];
        toc_entry.flags = entry->flags;

        if (write(out, &toc_entry, err) < 0)
            return false;
    }

    return true;
}

