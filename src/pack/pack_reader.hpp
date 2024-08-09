
#pragma once

/* pack_reader.hpp

Defines structs and functions for parsing package files and retrieving
information from them.
Unless you want to inspect package files, you probably don't need this directly.
Use pack_loader instead to load files from a package.
*/

#include "shl/error.hpp"

#include "pack/package.hpp"

struct pack_reader_entry
{
    const char *name;
    u64 flags;

    char *content;
    s64   size;
};

struct pack_reader
{
    char *content;
    s64 content_size;
    package_header  *header; // pointer into content
    package_toc     *toc;    // ditto
};

void init(pack_reader *reader);
void free(pack_reader *reader);

// data will be copied
bool pack_reader_load(pack_reader *reader, const char *data, s64 size, error *err);
bool pack_reader_load_from_path(pack_reader *reader, const char *path, error *err);

// after loading, parse checks if the loaded content is correct, and sets member pointers
bool pack_reader_parse(pack_reader *reader, error *err);

// Gets the nth package entry
void pack_reader_get_entry(const pack_reader *reader, s64 n, pack_reader_entry *out_entry);
// Gets the first entry with the given name, returns false if not found, true if found
bool pack_reader_get_entry_by_name(const pack_reader *reader, const char *name, pack_reader_entry *out_entry);
