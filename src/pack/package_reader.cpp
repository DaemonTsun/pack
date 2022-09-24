
#include <assert.h>
#include <string.h>

#include "shl/string.hpp"
#include "pack/package_reader.hpp"

void read_package(package_reader *reader)
{
    assert(reader != nullptr);
    assert(is_open(&reader->memory));

    if (reader->memory.size < sizeof(package_header))
        throw std::runtime_error("read_package: content smaller than expected");

    get_at(&reader->memory, &reader->header, 0);

    if (strncmp(reader->header->magic, PACK_HEADER_MAGIC, strlen(PACK_HEADER_MAGIC)) != 0)
        throw std::runtime_error("read_package: invalid package magic number");

    // TODO: version
    // TODO: flags

    u64 toc_pos = reader->header->toc_offset;

    if (toc_pos >= reader->memory.size - sizeof(package_toc))
        throw std::runtime_error(str("read_package: toc position ", std::hex, toc_pos, " out of range"));

    get_at(&reader->memory, &reader->toc, toc_pos);

    if (strncmp(reader->toc->magic, PACK_TOC_MAGIC, strlen(PACK_TOC_MAGIC)) != 0)
        throw std::runtime_error("read_package: invalid toc magic number");
}

void read(package_reader *reader, file_stream *stream)
{
    assert(reader != nullptr);
    assert(stream != nullptr);

    calculate_file_size(stream);
    open(&reader->memory, stream->size, false);
    read_entire_file(stream, reader->memory.data);
    read_package(reader);
}

void read(package_reader *reader, const char *path)
{
    assert(reader != nullptr);
    assert(path != nullptr);

    file_stream stream;

    if (!open(&stream, path))
        throw std::runtime_error(str("package_reader read: could not open file for reading '", path, "'"));
    
    read(reader, &stream);
    close(&stream);
}

void get_package_entry_from_toc(const package_reader *reader, const package_toc_entry *toc_entry, package_reader_entry *entry)
{
    get_at(&reader->memory, &entry->name, toc_entry->name_offset);
    get_at(&reader->memory, &entry->content, toc_entry->offset);
    entry->size = toc_entry->size;
    entry->flags = toc_entry->flags;
}

void get_toc_entry(const package_reader *reader, u64 n, package_toc_entry **toc_entry)
{
    get_at(&reader->memory, toc_entry, reader->header->toc_offset + sizeof(package_toc) + n * sizeof(package_toc_entry));
}

void get_package_entry(const package_reader *reader, u64 n, package_reader_entry *entry)
{
    assert(reader != nullptr);
    assert(entry != nullptr);
    assert(n < reader->toc->entry_count);

    package_toc_entry *toc_entry;
    get_toc_entry(reader, n, &toc_entry);

    get_package_entry_from_toc(reader, toc_entry, entry);
}

bool get_package_entry_by_name(const package_reader *reader, const char *name, package_reader_entry *entry)
{
    assert(reader != nullptr);
    assert(entry != nullptr);
    assert(name != nullptr);

    package_toc_entry *toc_entry;
    u64 len = strlen(name);
    bool found = false;

    for (u64 i = 0; i < reader->toc->entry_count; ++i)
    {
        get_toc_entry(reader, i, &toc_entry);
        const char *tocname = reader->memory.data + toc_entry->name_offset;

        if (strncmp(tocname, name, len) == 0)
        {
            get_package_entry_from_toc(reader, toc_entry, entry);
            found = true;
            break;
        }
    }

    return found;
}

void free(package_reader *reader)
{
    assert(reader != nullptr);

    close(&reader->memory);
}
