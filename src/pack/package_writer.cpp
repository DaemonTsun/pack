
#include <string.h>
#include <stdexcept>

#include "pack/package.hpp"
#include "pack/package_writer.hpp"

void add_file(package_writer *writer, file_stream *stream, const char *name)
{
    if (!is_open(stream))
        throw std::runtime_error("add_file: stream is not open");

    package_write_entry &entry = writer->entries.emplace_back();
    calculate_file_size(stream);
    
    open(&entry.content, stream->size, false);
    read_entire_file(stream, entry.content.data);

    entry.name = name;
}

void add_file(package_writer *writer, const char *path)
{
    file_stream stream;
    init(&stream);
    open(&stream, path);
    add_file(writer, &stream, path);
}

void add_entry(package_writer *writer, package_write_entry *entry)
{
    package_write_entry &x = writer->entries.emplace_back();
    x = *entry;
}

void add_entry(package_writer *writer, const char *str, const char *name)
{
    package_write_entry &entry = writer->entries.emplace_back();
    open(&entry.content, strlen(str));
    write(&entry.content, str);
    entry.name = name;

}

void add_entry(package_writer *writer, void *data, size_t size, const char *name)
{
    package_write_entry &entry = writer->entries.emplace_back();
    open(&entry.content, reinterpret_cast<char*>(data), size);
    entry.name = name;
}

void write(package_writer *writer, file_stream *out)
{
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
        write(out, entry.content.data, entry.content.size);
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
    toc.entry_count = entry_count;

    write(out, &toc);

    u64 toc_entry_pos = toc_pos + sizeof(toc);
    
    for (u64 i = 0; i < entry_count; ++i)
    {
        auto &entry = writer->entries[i];
        package_toc_entry toc_entry;
        toc_entry.offset = content_offsets[i];
        toc_entry.size = entry.content.size;
        toc_entry.name_offset = name_offsets[i];

        write(out, &toc_entry);
    }
}

void write(package_writer *writer, const char *out_path)
{
    file_stream stream;
    init(&stream);
    open(&stream, out_path, MODE_WRITE);

    write(writer, &stream);

    close(&stream);
}
