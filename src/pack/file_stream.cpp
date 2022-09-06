
// v1.0

#include <assert.h>
#include <stdarg.h>
#include "file_stream.hpp"

file_stream::operator FILE*() const
{
    return this->handle;
}

void init(file_stream *stream)
{
    assert(stream != nullptr);

    stream->handle = nullptr;
    stream->size = 0;
    stream->block_size = 1;
}

bool open(file_stream *stream, const char *path, const char *mode, bool check_open, bool calc_size)
{
    assert(stream != nullptr);

    if (check_open && !close(stream))
        return false;

    assert(path != nullptr);
    assert(mode != nullptr);
    stream->handle = fopen(path, mode);

    if (stream->handle == nullptr)
        return false;

    if (calc_size)
        calculate_file_size(stream);

    return true;
}

bool close(file_stream *stream)
{
    assert(stream != nullptr);

    if (stream->handle == nullptr)
        return true;

    bool ret = fclose(stream->handle) == 0;

    if (!ret)
        return ret;

    stream->handle = nullptr;

    return ret;
}

bool is_open(const file_stream *stream)
{
    assert(stream != nullptr);

    return stream->handle != nullptr;
}

bool is_at_end(file_stream *stream)
{
    assert(stream != nullptr);
    assert(stream->handle != nullptr);

    return feof(stream->handle) != 0;
}

bool has_error(file_stream *stream)
{
    assert(stream != nullptr);
    assert(stream->handle != nullptr);

    return ferror(stream->handle) != 0;
}

void clear_error(file_stream *stream)
{
    assert(stream != nullptr);
    assert(stream->handle != nullptr);

    clearerr(stream->handle);
}

bool is_ok(file_stream *stream)
{
    assert(stream != nullptr);

    return (stream->handle != nullptr)
        && (feof(stream->handle) == 0)
        && (ferror(stream->handle) == 0);
}

size_t calculate_file_size(file_stream *stream)
{
    assert(stream != nullptr);
    assert(stream->handle != nullptr);

    size_t curpos = tell(stream);
    seek(stream, 0, SEEK_END);
    size_t sz = tell(stream);
    seek(stream, curpos);

    stream->size = sz;
    return sz;
}

size_t block_count(const file_stream *stream)
{
    assert(stream != nullptr);
    return stream->size / stream->block_size;
}

int seek(file_stream *stream, long offset, int whence)
{
    assert(stream != nullptr);
    assert(stream->handle != nullptr);
    return fseeko(stream->handle, offset, whence);
}

int seek_block(file_stream *stream, long nth_block, int whence)
{
    assert(stream != nullptr);
    assert(stream->handle != nullptr);

    size_t bs = stream->block_size;
    size_t block_pos = nth_block * bs;

    if (whence != SEEK_CUR)
        return fseeko(stream->handle, block_pos, whence);

    // SEEK_CUR
    size_t cur = (ftello(stream->handle) / bs) * bs;
    return fseeko(stream->handle, cur + block_pos, SEEK_SET);
}

size_t tell(file_stream *stream)
{
    assert(stream != nullptr);
    assert(stream->handle != nullptr);

    return ftello(stream->handle);
}

bool getpos(file_stream *stream, fpos_t *pos)
{
    assert(stream != nullptr);
    assert(stream->handle != nullptr);
    assert(pos != nullptr);
    
    return fgetpos(stream->handle, pos) == 0;
}

bool rewind(file_stream *stream)
{
    assert(stream != nullptr);
    assert(stream->handle != nullptr);
    
    return fseeko(stream->handle, 0L, SEEK_SET) == 0;
}

size_t read(file_stream *stream, void *out, size_t size)
{
    assert(stream != nullptr);
    assert(stream->handle != nullptr);
    assert(out != nullptr);
    
    return fread(out, 1, size, stream->handle);
}

size_t read(file_stream *stream, void *out, size_t size, size_t nmemb)
{
    assert(stream != nullptr);
    assert(stream->handle != nullptr);
    assert(out != nullptr);
    
    return fread(out, size, nmemb, stream->handle);
}

size_t read_at(file_stream *stream, void *out, size_t offset, size_t size)
{
    assert(stream != nullptr);
    assert(stream->handle != nullptr);
    assert(out != nullptr);
    
    if (fseeko(stream->handle, offset, SEEK_SET) != 0)
        return 0;

    return fread(out, 1, size, stream->handle);
}

size_t read_at(file_stream *stream, void *out, size_t offset, size_t size, size_t nmemb)
{
    assert(stream != nullptr);
    assert(stream->handle != nullptr);
    assert(out != nullptr);
    
    if (fseeko(stream->handle, offset, SEEK_SET) != 0)
        return 0;

    return fread(out, size, nmemb, stream->handle);
}

size_t read_block(file_stream *stream, void *out)
{
    assert(stream != nullptr);
    assert(stream->handle != nullptr);
    assert(out != nullptr);

    return fread(out, 1, stream->block_size, stream->handle);
}

size_t read_block(file_stream *stream, void *out, size_t nth_block)
{
    assert(out != nullptr);
    
    seek_block(stream, nth_block, SEEK_SET);
    return fread(out, 1, stream->block_size, stream->handle);
}

size_t read_blocks(file_stream *stream, void *out, size_t block_count)
{
    assert(stream != nullptr);
    assert(stream->handle != nullptr);
    assert(out != nullptr);

    return fread(out, stream->block_size, block_count, stream->handle);
}

size_t read_blocks(file_stream *stream, void *out, size_t nth_block, size_t block_count)
{
    assert(out != nullptr);

    seek_block(stream, nth_block, SEEK_SET);
    return fread(out, stream->block_size, block_count, stream->handle);
}

size_t read_entire_file(file_stream *stream, void *out, size_t max_size)
{
    assert(stream != nullptr);
    assert(stream->handle != nullptr);
    assert(out != nullptr);
    
    size_t old_pos = ftello(stream->handle);
    fseeko(stream->handle, 0, SEEK_SET);

    size_t sz = stream->size;

    if (sz > max_size)
        sz = max_size;

    size_t ret = fread(out, 1, sz, stream->handle);
    fseeko(stream->handle, old_pos, SEEK_SET);

    return ret;
}

size_t write(file_stream *stream, const void *in, size_t size)
{
    assert(stream != nullptr);
    assert(stream->handle != nullptr);
    assert(in != nullptr);

    return fwrite(in, 1, size, stream->handle);
}

size_t write(file_stream *stream, const void *in, size_t size, size_t nmemb)
{
    assert(stream != nullptr);
    assert(stream->handle != nullptr);
    assert(in != nullptr);

    return fwrite(in, size, nmemb, stream->handle);
}

size_t write(file_stream *stream, const char *str)
{
    assert(stream != nullptr);
    assert(stream->handle != nullptr);
    assert(str != nullptr);

    return fputs(str, stream->handle);
}

size_t write_at(file_stream *stream, const void *in, size_t offset, size_t size)
{
    assert(stream != nullptr);
    assert(stream->handle != nullptr);
    assert(in != nullptr);

    if (fseeko(stream->handle, offset, SEEK_SET) != 0)
        return 0;

    return fwrite(in, 1, size, stream->handle);
}

size_t write_at(file_stream *stream, const void *in, size_t offset, size_t size, size_t nmemb)
{
    assert(stream != nullptr);
    assert(stream->handle != nullptr);
    assert(in != nullptr);

    if (fseeko(stream->handle, offset, SEEK_SET) != 0)
        return 0;

    return fwrite(in, size, nmemb, stream->handle);
}

size_t write_at(file_stream *stream, const char *str, size_t offset)
{
    assert(stream != nullptr);
    assert(stream->handle != nullptr);
    assert(str != nullptr);

    if (fseeko(stream->handle, offset, SEEK_SET) != 0)
        return 0;

    return fputs(str, stream->handle);
}

size_t write_block(file_stream *stream, const void *in)
{
    assert(stream != nullptr);
    assert(stream->handle != nullptr);
    assert(in != nullptr);

    return fwrite(in, 1, stream->block_size, stream->handle);
}

size_t write_block(file_stream *stream, const void *in, size_t nth_block)
{
    assert(in != nullptr);

    seek_block(stream, nth_block, SEEK_SET);
    return fwrite(in, 1, stream->block_size, stream->handle);
}

size_t write_blocks(file_stream *stream, const void *in, size_t block_count)
{
    assert(stream != nullptr);
    assert(stream->handle != nullptr);
    assert(in != nullptr);

    return fwrite(in, stream->block_size, block_count, stream->handle);
}

size_t write_blocks(file_stream *stream, const void *in, size_t nth_block, size_t block_count)
{
    assert(in != nullptr);

    seek_block(stream, nth_block, SEEK_SET);
    return fwrite(in, stream->block_size, block_count, stream->handle);
}

int format(FILE *stream, const char *format_string, ...)
{
    va_list arg;
    int ret;

    va_start(arg, format_string);
    ret = vfprintf(stream, format_string, arg);
    va_end(arg);

    return ret;
}

int format(file_stream *stream, const char *format_string, ...)
{
    va_list arg;
    int ret;

    va_start(arg, format_string);
    ret = vfprintf(stream->handle, format_string, arg);
    va_end(arg);

    return ret;
}

int flush(file_stream *stream)
{
    return fflush(stream->handle);
}
