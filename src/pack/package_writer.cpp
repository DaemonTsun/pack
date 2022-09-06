
#include <string.h>
#include <stdexcept>
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
    for (auto &entry : writer->entries)
        write(out, entry.content.data, entry.content.size);
}

void write(package_writer *writer, const char *out_path)
{
    file_stream stream;
    init(&stream);
    open(&stream, out_path, MODE_WRITE);

    write(writer, &stream);
}
