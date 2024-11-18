
#include "shl/file_stream.hpp"
#include "shl/memory.hpp"
#include "shl/assert.hpp"
#include "fs/path.hpp"

#include "pack/pack_loader.hpp"

void init(pack_loader *loader)
{
    assert(loader != nullptr);

    fill_memory(loader, 0);
}

void free(pack_loader *loader)
{
    assert(loader != nullptr);

    if (loader->mode == pack_loader_mode::Package)
        free(&loader->reader);
    else
    {
        fs::free(&loader->files.base_path);
        fs::free(&loader->files._entry_path);
        pack_loader_clear_loaded_file_entries(loader);
        free(&loader->files.loaded_entries);
    }

    fill_memory(loader, 0);
}

void pack_loader_clear_loaded_file_entries(pack_loader *loader)
{
    assert(loader != nullptr);

    for_array(entry, &loader->files.loaded_entries)
        if (entry->data != nullptr)
            dealloc((void*)entry->data, entry->size);
    
    fill_memory((void*)loader->files.loaded_entries.data, 0, sizeof(pack_file_entry) * loader->files.count);
}

bool pack_loader_load_package_file(pack_loader *loader, const char *filename, error *err)
{
    assert(loader != nullptr);

    free(loader);

    loader->mode = pack_loader_mode::Package;
    return pack_reader_load_from_path(&loader->reader, filename, err);
}

void pack_loader_load_files(pack_loader *loader, const char **files, s64 file_count, const char *base_path)
{
    assert(loader != nullptr);
    assert(files != nullptr);

    free(loader);

    loader->mode = pack_loader_mode::Files;
    loader->files.ptr = files;
    loader->files.count = file_count;

    if (base_path != nullptr)
        fs::path_set(&loader->files.base_path, base_path);
    else
        fs::path_set(&loader->files.base_path, ".");

    resize(&loader->files.loaded_entries, file_count);
    fill_memory((void*)loader->files.loaded_entries.data, 0, sizeof(pack_file_entry) * loader->files.count);
}

s64 pack_loader_entry_count(pack_loader *loader)
{
    assert(loader != nullptr);

    if (loader->mode == pack_loader_mode::Package)
        return loader->reader.toc->entry_count;
    else
        return loader->files.count;
}

bool pack_loader_load_entry(pack_loader *loader, s64 n, pack_entry *out_entry, error *err)
{
    assert(loader != nullptr);

    if (loader->mode == pack_loader_mode::Package)
    {
        assert(n < loader->reader.toc->entry_count);

        pack_reader_entry rentry{};
        pack_reader_get_entry(&loader->reader, n, &rentry);

        out_entry->data = rentry.content;
        out_entry->size = rentry.size;
        out_entry->name = rentry.name;
    }
    else
    {
        assert(n >= 0);
        assert(n < loader->files.count);
        assert(n < loader->files.loaded_entries.size);
        assert(loader->files.count == loader->files.loaded_entries.size);

        fs::path_set(&loader->files._entry_path, &loader->files.base_path);
        fs::path_append(&loader->files._entry_path, loader->files.ptr[n]);
        pack_file_entry *loaded_entry = loader->files.loaded_entries.data + n;
        io_handle fh;

        fh = io_open(loader->files._entry_path.c_str(), open_mode::Read, err);

        if (fh == INVALID_IO_HANDLE)
            return false;

        defer { io_close(fh); };

        s64 timestamp = 0;
        fs::filesystem_info info{};

        if (!fs::query_filesystem(fh, &info, fs::query_flag::FileTimes, err))
            return false;

#if Windows
        timestamp = (s64)info.detail.file_times.last_write_time;
#else
        timestamp = info.stx_mtime.tv_sec;
#endif

        out_entry->name = loader->files.ptr[n];

        if (loaded_entry->data != nullptr)
        {
            if (loaded_entry->timestamp >= timestamp)
            {
                out_entry->data = loaded_entry->data;
                out_entry->size = loaded_entry->size;
                return true;
            }
            else
            {
                dealloc((void*)loaded_entry->data, loaded_entry->size);
                fill_memory(loaded_entry, 0);
                loaded_entry->data = nullptr;
            }
        }

        if (loaded_entry->data == nullptr)
        {
            file_stream fstream{};
            fstream.handle = fh;

            s64 fsize = get_file_size(&fstream, err);

            if (fsize < 0)
                return false;
            
            loaded_entry->data = (char*)alloc(fsize+1);
            loaded_entry->size = fsize;

            if (!read_entire_file(&fstream, loaded_entry->data, fsize, err))
                return false;

            loaded_entry->data[fsize] = '\0';

            loaded_entry->timestamp = timestamp;
        }

        out_entry->data = loaded_entry->data;
        out_entry->size = loaded_entry->size;
    }

    return true;
}

const char *pack_loader_entry_name(pack_loader *loader, s64 entry, error *err)
{
    assert(loader != nullptr);
    assert(entry >= 0);

    if (loader->mode == pack_loader_mode::Package)
    {
        pack_entry ent{};

        if (!pack_loader_load_entry(loader, entry, &ent, err))
            return nullptr;

        return ent.name;
    }
    else
    {
        assert(entry < loader->files.count);
        return loader->files.ptr[entry];
    }
}
