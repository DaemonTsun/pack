
#include "shl/string.hpp"
#include "shl/error.hpp"
#include "shl/memory.hpp"
#include "shl/streams.hpp"

#include "pack/pack_reader.hpp"

void init(pack_reader *reader)
{
    assert(reader != nullptr);

    fill_memory(reader, 0);
}

void free(pack_reader *reader)
{
    assert(reader != nullptr);

    if (reader->content != nullptr)
        dealloc(reader->content, reader->content_size);

    fill_memory(reader, 0);
}

bool pack_reader_load(pack_reader *reader, const char *data, s64 size, error *err)
{
    assert(reader != nullptr);
    assert(data != nullptr);
    
    reader->content = (char*)alloc(size);
    reader->content_size = size;

    copy_memory(data, reader->content, size);

    if (!pack_reader_parse(reader, err))
    {
        free(reader);
        return false;
    }

    return true;
}

bool pack_reader_load_from_path(pack_reader *reader, const char *path, error *err)
{
    assert(reader != nullptr);
    assert(path != nullptr);

    memory_stream mem{};

    if (!read_entire_file(path, &mem, err))
        return false;

    reader->content = mem.data;
    reader->content_size = mem.size;

    if (!pack_reader_parse(reader, err))
    {
        free(reader);
        return false;
    }

    return true;
}

bool pack_reader_parse(pack_reader *reader, error *err)
{
    assert(reader != nullptr);
    assert(reader->content != nullptr);

    if (reader->content_size < (s64)sizeof(package_header))
    {
        format_error(err, 1, "reader_parse: package content (%x) smaller than header (%x)", reader->content_size, (s64)sizeof(package_header));
        return false;
    }

    reader->header = (package_header*)reader->content;

    if (compare_strings(reader->header->magic, PACK_HEADER_MAGIC, string_length(PACK_HEADER_MAGIC)) != 0)
    {
        set_error(err, 2, "read_package: invalid package magic number");
        return false;
    }

    // TODO: version
    // TODO: flags

    s64 toc_pos = reader->header->toc_offset;

    if (toc_pos >= reader->content_size - (s64)sizeof(package_toc))
    {
        format_error(err, 3, "reader_parse: toc position (%x + %x) outside bounds of package (%x)", toc_pos, (s64)sizeof(package_toc), reader->content_size);
        return false;
    }

    reader->toc = (package_toc*)(reader->content + toc_pos);

    if (compare_strings(reader->toc->magic, PACK_TOC_MAGIC, string_length(PACK_TOC_MAGIC)) != 0)
    {
        set_error(err, 4, "reader_parse: invalid toc magic number");
        return false;
    }

    return true;
}

static void _get_package_entry_from_toc(const pack_reader *reader, const package_toc_entry *toc_entry, pack_reader_entry *entry)
{
    entry->name = reader->content + toc_entry->name_offset;
    entry->content = reader->content + toc_entry->offset;
    entry->size =  toc_entry->size;
    entry->flags = toc_entry->flags;
}

static package_toc_entry *_get_toc_entry(const pack_reader *reader, s64 n)
{
    return (package_toc_entry*)(reader->content + reader->header->toc_offset + sizeof(package_toc) + n * sizeof(package_toc_entry));
}

void pack_reader_get_entry(const pack_reader *reader, s64 n, pack_reader_entry *out_entry)
{
    assert(reader != nullptr);
    assert(out_entry != nullptr);
    assert(n < reader->toc->entry_count);

    package_toc_entry *toc_entry = _get_toc_entry(reader, n);
    _get_package_entry_from_toc(reader, toc_entry, out_entry);
}

bool pack_reader_get_entry_by_name(const pack_reader *reader, const char *name, pack_reader_entry *out_entry)
{
    assert(reader != nullptr);
    assert(reader->toc != nullptr);
    assert(out_entry != nullptr);
    assert(name != nullptr);

    package_toc_entry *toc_entry = nullptr;
    s64 len = string_length(name);
    bool found = false;

    for (s64 i = 0; i < reader->toc->entry_count; ++i)
    {
        _get_toc_entry(reader, i);
        const char *tocname = reader->content + toc_entry->name_offset;

        if (compare_strings(tocname, name, len) == 0)
        {
            _get_package_entry_from_toc(reader, toc_entry, out_entry);
            found = true;
            break;
        }
    }

    return found;
}

