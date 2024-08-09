
#pragma once

/* pack_writer.hpp

Used to create package files from code.
 */

#include "shl/string.hpp"
#include "shl/array.hpp"
#include "shl/memory_stream.hpp"
#include "pack/package.hpp"

enum class pack_writer_entry_type
{
    Memory,
    File // loaded lazily when writing
};

struct pack_writer_file
{
    const char *path;
    u64 size;
};

struct pack_writer_entry
{
    // name, usually path
    string name;
    u64 flags;
    pack_writer_entry_type type;

    union
    {
        memory_stream memory;
        pack_writer_file file;
    };
};

void init(pack_writer_entry *entry);
void free(pack_writer_entry *entry);

struct pack_writer
{
    array<pack_writer_entry> entries;
};

void init(pack_writer *writer);
void free(pack_writer *writer);

bool pack_writer_add_file(pack_writer *writer, const char *path, bool lazy = true, error *err = nullptr);
bool pack_writer_add_file(pack_writer *writer, const char *path, const char *name, bool lazy = true, error *err = nullptr);

void pack_writer_add_entry(pack_writer *writer, pack_writer_entry *entry);
void pack_writer_add_entry(pack_writer *writer, const char *str, const char *name = "");
void pack_writer_add_entry(pack_writer *writer, void *data, s64 size, const char *name = "");

template<typename T>
inline void pack_writer_add_entry(pack_writer *writer, T *data, const char *name = "")
{
    pack_writer_add_entry(writer, reinterpret_cast<void*>(data), sizeof(T), name);
}

bool pack_writer_write_to_file(pack_writer *writer, const char *out_path, error *err = nullptr);
bool pack_writer_write_to_file(pack_writer *writer, io_handle handle, s64 offset = 0, error *err = nullptr);
