
#pragma once

#include <string>
#include <vector>

#include "shl/file_stream.hpp"
#include "shl/memory_stream.hpp"
#include "pack/package.hpp"

enum class package_writer_entry_type
{
    Memory,
    File // loaded lazily when writing
};

struct package_writer_file
{
    const char *path;
    u64 size;
};

struct package_writer_entry
{
    // name, usually path
    std::string name;
    u64 flags;
    package_writer_entry_type type;

    union
    {
        memory_stream memory;
        package_writer_file file;
    } content;
};

struct package_writer
{
    std::vector<package_writer_entry> entries;
};

void add_file(package_writer *writer, const char *path, const char *name, bool lazy = true);
void add_file(package_writer *writer, const char *path, bool lazy = true);
void add_entry(package_writer *writer, package_writer_entry *entry);
void add_entry(package_writer *writer, const char *str, const char *name = "");
void add_entry(package_writer *writer, void *data, size_t size, const char *name = "");

template<typename T>
void add_entry(package_writer *writer, T *data, const char *name = "")
{
    add_entry(writer, reinterpret_cast<void*>(data), sizeof(T), name);
}

template<typename T>
void add_entry(package_writer *writer, T &&data, const char *name = "")
{
    add_entry(writer, reinterpret_cast<void*>(&data), sizeof(T), name);
}

void write(package_writer *writer, file_stream *out);
void write(package_writer *writer, const char *out_path);
void free(package_writer *writer);
