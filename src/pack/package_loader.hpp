
#pragma once

#include "shl/memory_stream.hpp"
#include "pack/package_reader.hpp"

enum class package_loader_mode
{
    Package,
    Files
};

struct package_loader
{
    package_loader_mode mode;

    union _loader_data
    {
        package_reader reader;

        struct _file_data
        {
            const char **ptr;
            u64 count;
        } files;
    } data;
};

#ifdef NDEBUG
#define load_package(loader, PACKAGE)\
    load_package_file(loader, PACKAGE)
#else
#define load_package(loader, PACKAGE)\
    load_files(loader, PACKAGE##_files, PACKAGE##_file_count)
#endif

void init(package_loader *loader);

void load_package_file(package_loader *loader, const char *filename);
void load_files(package_loader *loader, const char **files, u64 file_count);
bool load_entry(package_loader *loader, u64 entry, memory_stream *stream);

void free(package_loader *loader);
