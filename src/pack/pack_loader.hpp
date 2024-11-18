
#pragma once

/* pack_loader.hpp

An abstraction over pack_reader, with the ability to load entries from either a
package file, or from individual files from a list of file paths.
 */

#include "shl/array.hpp"
#include "fs/path.hpp"
#include "pack/pack_reader.hpp"

/* pack_loader has two different modes for loading:
    Package: load resources from a .pack package file or
    Files: load files from individual files using 
 */
enum class pack_loader_mode
{
    Package = 1,
    Files   = 2
};

struct pack_entry
{
    const char *data;
    s64 size;
    const char *name; // usually the path of the entry
};

// used internally
struct pack_file_entry
{
    char *data;
    s64 size;
    s64 timestamp;
};

struct pack_loader
{
    pack_loader_mode mode;

    union
    {
        pack_reader reader;

        struct 
        {
            const char **ptr;
            s64 count;
            fs::path base_path;
            fs::path _entry_path;
            array<pack_file_entry> loaded_entries;
        } files;
    };
};

void init(pack_loader *loader);
void free(pack_loader *loader);

void pack_loader_clear_loaded_file_entries(pack_loader *loader);

bool pack_loader_load_package_file(pack_loader *loader, const char *filename, error *err = nullptr);
void pack_loader_load_files(pack_loader *loader, const char **files, s64 file_count, const char *base_path = nullptr);

// once either a package file or files are loaded, use this to get individual entries
bool pack_loader_load_entry(pack_loader *loader, s64 entry, pack_entry *out, error *err = nullptr);

s64 pack_loader_entry_count(pack_loader *loader);

// the name of the entry is stored in pack_entry, HOWEVER if the mode is file,
// pack_loader_load_entry will load the entry from disk, so if we only want the name,
// we'd load the entry for no reason. This function does not load the entry from disk and
// only retreives the path from the generated constants in file mode.
const char *pack_loader_entry_name(pack_loader *loader, s64 entry, error *err = nullptr);
