
#include <assert.h>
#include "shl/file_stream.hpp"
#include "pack/package_loader.hpp"

void init(package_loader *loader)
{
    assert(loader != nullptr);

    loader->mode = package_loader_mode::Files;
    loader->data.files.ptr = nullptr;
    loader->data.files.count = 0;
}

void load_package_file(package_loader *loader, const char *filename)
{
    assert(loader != nullptr);

    loader->mode = package_loader_mode::Package;
    read(&loader->data.reader, filename);
}

void load_files(package_loader *loader, const char **files, u64 file_count)
{
    assert(loader != nullptr);
    assert(files != nullptr);

    loader->mode = package_loader_mode::Files;
    loader->data.files.ptr = files;
    loader->data.files.count = file_count;
}

bool load_entry(package_loader *loader, u64 entry, memory_stream *stream)
{
    assert(loader != nullptr);

    if (loader->mode == package_loader_mode::Package)
    {
        assert(entry < loader->data.reader.toc->entry_count);

        package_reader_entry rentry;
        get_package_entry(&loader->data.reader, entry, &rentry);

        // DONT copy the data into the stream, the entire package
        // is already loaded by the package reader.
        // only set the pointer to the data and it's size.
        if (!open(stream, rentry.content, rentry.size, false, false))
            return false;
    }
    else
    {
        assert(entry < loader->data.files.count);
        file_stream fstream;

        if (!open(&fstream, loader->data.files.ptr[entry], MODE_READ, false, true))
            return false;

        // in file streams, we close the current stream and
        // copy the file into the new stream
        if (!open(stream, fstream.size, true, true))
            return false;

        read_entire_file(&fstream, stream->data);
    }

    return true;
}

void free(package_loader *loader)
{
    assert(loader != nullptr);

    if (loader->mode == package_loader_mode::Package)
        free(&loader->data.reader);

    loader->mode = package_loader_mode::Files;
    loader->data.files.ptr = nullptr;
    loader->data.files.count = 0;
}
