
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
        pack_loader_clear_loaded_file_entries(loader);

    fill_memory(loader, 0);
}

void pack_loader_clear_loaded_file_entries(pack_loader *loader)
{
    assert(loader != nullptr);

    for_array(entry, &loader->files.loaded_entries)
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

void pack_loader_load_files(pack_loader *loader, const char **files, s64 file_count)
{
    assert(loader != nullptr);
    assert(files != nullptr);

    free(loader);

    loader->mode = pack_loader_mode::Files;
    loader->files.ptr = files;
    loader->files.count = file_count;
    resize(&loader->files.loaded_entries, file_count);
    fill_memory((void*)loader->files.loaded_entries.data, 0, sizeof(pack_file_entry) * loader->files.count);
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
    }
    else
    {
        assert(n >= 0);
        assert(n < loader->files.count);
        assert(n < loader->files.loaded_entries.size);
        assert(loader->files.count == loader->files.loaded_entries.size);

        const char *path = loader->files.ptr[n];
        pack_file_entry *loaded_entry = loader->files.loaded_entries.data + n;
        io_handle fh;

        fh = io_open(path, open_mode::Read, err);

        if (fh == INVALID_IO_HANDLE)
            return false;

        defer { io_close(fh); };

        s64 timestamp = 0;
        fs::filesystem_info info{};

        if (!fs::get_filesystem_info(fh, &info, FS_QUERY_FILE_TIMES, err))
            return false;

#if Windows
        timestamp = (s64)info.detail.file_times.last_write_time;
#else
        timestamp = info.stx_mtime.tv_sec;
#endif

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
            
            loaded_entry->data = (char*)alloc(fsize);
            loaded_entry->size = fsize;

            if (!read_entire_file(&fstream, loaded_entry->data, fsize, err))
                return false;

            loaded_entry->timestamp = timestamp;
        }
    }

    return true;
}
