
#pragma once

#include <string>
#include <vector>
#include "file_stream.hpp"
#include "memory_stream.hpp"

struct package_write_entry
{
    // name, usually path
    std::string name;
    memory_stream content;
};

struct package_writer
{
    std::vector<package_write_entry> entries;
};

void add_file(package_writer *writer, file_stream *stream, const char *name = "");
void add_file(package_writer *writer, const char *path);
void add_entry(package_writer *writer, package_write_entry *entry);
void add_entry(package_writer *writer, const char *str, const char *name = "");
void add_entry(package_writer *writer, void *data, size_t size, const char *name = "");

template<typename T>
void add_entry(package_writer *writer, T &&data, const char *name = "")
{
    add_entry(writer, reinterpret_cast<void*>(&data), sizeof(T), name);
}

void write(package_writer *writer, file_stream *out);
void write(package_writer *writer, const char *out_path);
