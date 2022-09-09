
#include <assert.h>
#include <string.h>
#include <stdexcept>

#include "pack/shl/string.hpp"
#include "pack/package.hpp"
#include "pack/package_writer.hpp"

void add_file(package_writer *writer, const char *path, bool lazy)
{
    assert(writer != nullptr);
    assert(path != nullptr);

    package_writer_entry &entry = writer->entries.emplace_back();
    entry.flags = PACK_TOC_FLAG_FILE;
    entry.name = path;

    file_stream stream;
    init(&stream);

    if (!open(&stream, path))
        throw std::runtime_error(str("add_file: could not open path '", path, "'"));

    calculate_file_size(&stream);

    if (lazy)
    {
        entry.type = package_writer_entry_type::File;
        entry.content.file.path = path;
        entry.content.file.size = stream.size;
    }
    else
    {
        entry.type = package_writer_entry_type::Memory;
        init(&entry.content.memory);

        open(&entry.content.memory, stream.size, false);
        read_entire_file(&stream, entry.content.memory.data);
    }

    close(&stream);
}

void add_entry(package_writer *writer, package_writer_entry *entry)
{
    assert(writer != nullptr);
    assert(entry != nullptr);

    package_writer_entry &x = writer->entries.emplace_back();
    x = *entry;
}

void add_entry(package_writer *writer, const char *str, const char *name)
{
    assert(writer != nullptr);
    assert(str != nullptr);
    assert(name != nullptr);

    package_writer_entry &entry = writer->entries.emplace_back();
    entry.name = name;
    entry.flags = PACK_TOC_NO_FLAGS;
    entry.type = package_writer_entry_type::Memory;

    open(&entry.content.memory, strlen(str));
    write(&entry.content.memory, str);
}

void add_entry(package_writer *writer, void *data, size_t size, const char *name)
{
    assert(writer != nullptr);
    assert(data != nullptr);
    assert(name != nullptr);

    package_writer_entry &entry = writer->entries.emplace_back();
    entry.name = name;
    entry.flags = PACK_TOC_NO_FLAGS;
    entry.type = package_writer_entry_type::Memory;

    open(&entry.content.memory, size, false);
    write(&entry.content.memory, data, size);
}

void write_entry(file_stream *out, package_writer_entry *entry)
{
    if (entry->type == package_writer_entry_type::Memory)
    {
        write(out, entry->content.memory.data, entry->content.memory.size);
    }
    else
    {
        // we read the entire file to memory, then just write that
        file_stream stream;
        open(&stream, entry->content.file.path);
        calculate_file_size(&stream);
        
        void *mem = malloc(stream.size);
        read_entire_file(&stream, mem);
        write(out, mem, stream.size);
        free(mem);
        close(&stream);
    }
}

u64 entry_size(package_writer_entry *entry)
{
    if (entry->type == package_writer_entry_type::Memory)
        return entry->content.memory.size;
    else
        return entry->content.file.size;
}

void write(package_writer *writer, file_stream *out)
{
    assert(writer != nullptr);
    assert(out != nullptr);

    size_t entry_count = writer->entries.size();

    package_header header;
    strncpy(header.magic, PACK_HEADER_MAGIC, 4);
    header.version = PACK_VERSION;
    header.flags = PACK_NO_FLAGS;

    header.toc_offset = 0;   // placeholder
    header.names_offset = 0; // placeholder
    header.names_size = 0;   // placeholder
    
    // write header
    write(out, &header);

    // write the entry contents
    std::vector<u64> content_offsets;
    content_offsets.resize(entry_count);

    for (u64 i = 0; i < entry_count; ++i)
    {
        auto &entry = writer->entries[i];
        content_offsets[i] = tell(out);
        write_entry(out, &entry);
        seek_next_alignment(out, 8);
    }

    // write the name table
    u64 name_table_pos = tell(out);
    write_at(out, &name_table_pos, offsetof(package_header, names_offset));
    seek(out, name_table_pos);

    std::vector<u64> name_offsets;
    name_offsets.resize(entry_count);

    for (u64 i = 0; i < entry_count; ++i)
    {
        name_offsets[i] = tell(out);
        write(out, writer->entries[i].name.c_str());
        write(out, "\0", 1);
    }

    u64 npos = tell(out);
    u64 name_table_size = npos - name_table_pos;
    write_at(out, &name_table_size, offsetof(package_header, names_size));
    seek(out, npos);

    seek_next_alignment(out, 8);

    // write the toc
    u64 toc_pos = tell(out);
    write_at(out, &toc_pos, offsetof(package_header, toc_offset));
    seek(out, toc_pos);

    package_toc toc;
    strncpy(toc.magic, PACK_TOC_MAGIC, 4);
    toc._padding = 0;
    toc.entry_count = entry_count;

    write(out, &toc);

    u64 toc_entry_pos = toc_pos + sizeof(toc);
    
    for (u64 i = 0; i < entry_count; ++i)
    {
        auto &entry = writer->entries[i];

        package_toc_entry toc_entry;
        toc_entry.offset = content_offsets[i];
        toc_entry.size = entry_size(&entry);
        toc_entry.name_offset = name_offsets[i];
        toc_entry.flags = entry.flags;

        write(out, &toc_entry);
    }
}

void write(package_writer *writer, const char *out_path)
{
    assert(writer != nullptr);
    assert(out_path != nullptr);

    file_stream stream;
    init(&stream);

    if (!open(&stream, out_path, MODE_WRITE))
        throw std::runtime_error(str("package_writer write: could not open file for writing '", out_path, "'"));

    write(writer, &stream);

    close(&stream);
}

void free(package_writer *writer)
{
    assert(writer != nullptr);

    for (auto &entry : writer->entries)
    {
        if (entry.type != package_writer_entry_type::Memory)
            continue;

        close(&entry.content.memory);
    }
}
