
#pragma once

#include "shl/file_stream.hpp"
#include "shl/memory_stream.hpp"
#include "pack/package.hpp"

struct package_reader_entry
{
    const char *name;
    u64 flags;

    char *content;
    u64 size;
};

struct package_reader
{
    memory_stream memory;

    package_header *header;
    package_toc *toc;
};

void read_package(package_reader *reader);
void read(package_reader *reader, file_stream *stream);
void read(package_reader *reader, const char *path);

// gets the nth package entry
void get_package_entry(const package_reader *reader, u64 n, package_reader_entry *entry);
// gets the first entry with the given name, returns false if not found, true if found
bool get_package_entry_by_name(const package_reader *reader, const char *name, package_reader_entry *entry);

void free(package_reader *reader);
