
// v1.0

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory_stream.hpp"

memory_stream::operator char*() const
{
    return current(this);
}

void init(memory_stream *stream)
{
    assert(stream != nullptr);

    stream->data = nullptr;
    stream->size = 0;
    stream->block_size = 1;
    stream->position = 0;
}

bool open(memory_stream *stream, size_t size, bool check_open, bool free_on_close)
{
    assert(stream != nullptr);
    
    if (check_open && !close(stream, free_on_close))
        return false;

    stream->data = reinterpret_cast<char*>(malloc(size));

    if (stream->data == nullptr)
        return false;

    stream->size = size;
    stream->position = 0;

    return true;
}

bool open(memory_stream *stream, char *in, size_t size, bool check_open, bool free_on_close)
{
    assert(stream != nullptr);
    assert(in != nullptr);
    
    if (check_open && !close(stream, free_on_close))
        return false;

    stream->data = in;

    if (stream->data == nullptr)
        return false;

    stream->size = size;
    stream->position = 0;

    return true;
}

bool close(memory_stream *stream, bool free_)
{
    assert(stream != nullptr);

    if (stream->data == nullptr)
        return true;

    if (free_)
        free(stream->data);

    stream->data = nullptr;

    return true;
}

bool is_open(const memory_stream *stream)
{
    assert(stream != nullptr);

    return stream->data != nullptr;
}

bool is_at_end(const memory_stream *stream)
{
    assert(stream != nullptr);

    return stream->position >= stream->size;
}

bool is_ok(const memory_stream *stream)
{
    assert(stream != nullptr);

    return (stream->data != nullptr)
        && (stream->position < stream->size);
}

size_t block_count(const memory_stream *stream)
{
    assert(stream != nullptr);
    return stream->size / stream->block_size;
}

char *current(const memory_stream *stream)
{
    assert(stream != nullptr);
    assert(stream->data != nullptr);
    
    return stream->data + stream->position;
}

char *current_block_start(const memory_stream *stream)
{
    assert(stream != nullptr);
    assert(stream->data != nullptr);
    assert(stream->block_size > 0);

    return stream->data + ((stream->position / stream->block_size) * stream->block_size);
}

size_t current_block_number(const memory_stream *stream)
{
    assert(stream != nullptr);
    assert(stream->block_size > 0);

    return stream->position / stream->block_size;
}

size_t current_block_offset(const memory_stream *stream)
{
    assert(stream != nullptr);
    assert(stream->block_size > 0);

    return (stream->position / stream->block_size) * stream->block_size;
}

int seek(memory_stream *stream, long offset, int whence)
{
    assert(stream != nullptr);

    if (whence == SEEK_SET)
        stream->position = offset;
    else if (whence == SEEK_CUR)
        stream->position = stream->position + offset;
    else if (whence == SEEK_END)
        stream->position = stream->size + offset;
    else
        return 1;

    if (stream->position > stream->size)
        stream->position = stream->size;

    return 0;
}

int seek_block(memory_stream *stream, long nth_block, int whence)
{
    assert(stream != nullptr);

    if (whence != SEEK_CUR)
        return seek(stream, nth_block * stream->block_size, whence);

    // SEEK_CUR
    size_t cur = (stream->position / stream->block_size) * stream->block_size;
    return seek(stream, cur + nth_block * stream->block_size, SEEK_SET);
}

size_t tell(memory_stream *stream)
{
    assert(stream != nullptr);
    return stream->position;
}

size_t getpos(memory_stream *stream)
{
    assert(stream != nullptr);
    return stream->position;
}

void rewind(memory_stream *stream)
{
    assert(stream != nullptr);
    stream->position = 0;
}

size_t read(memory_stream *stream, void *out, size_t size)
{
    assert(stream != nullptr);
    assert(stream->data != nullptr);
    assert(out != nullptr);
    assert(stream->position < stream->size);

    size_t read_size = size;
    size_t rest = stream->size - stream->position;

    if (read_size > rest)
        read_size = rest;

    if (read_size > 0)
    {
        memcpy(out, stream->data + stream->position, read_size);
        stream->position += read_size;
    }

    return read_size;
}

size_t read(memory_stream *stream, void *out, size_t size, size_t nmemb)
{
    assert(stream != nullptr);
    assert(stream->data != nullptr);
    assert(out != nullptr);
    assert(stream->position < stream->size);

    size_t read_size = size * nmemb;
    size_t rest = stream->size - stream->position;

    if (read_size > rest)
        read_size = (rest / size) * size;

    if (read_size > 0)
    {
        memcpy(out, stream->data + stream->position, read_size);
        stream->position += read_size;
    }

    return read_size / size;
}

size_t read_at(memory_stream *stream, void *out, size_t offset, size_t size)
{
    assert(stream != nullptr);
    assert(stream->data != nullptr);
    assert(out != nullptr);
    
    seek(stream, offset, SEEK_SET);
    return read(stream, out, size);
}

size_t read_at(memory_stream *stream, void *out, size_t offset, size_t size, size_t nmemb)
{
    assert(stream != nullptr);
    assert(stream->data != nullptr);
    assert(out != nullptr);

    seek(stream, offset, SEEK_SET);
    return read(stream, out, size, nmemb);
}

size_t read_block(memory_stream *stream, void *out)
{
    assert(stream != nullptr);
    assert(stream->data != nullptr);
    assert(out != nullptr);
    assert(stream->position < stream->size);

    return read(stream, out, stream->block_size);
}

size_t read_block(memory_stream *stream, void *out, size_t nth_block)
{
    assert(stream != nullptr);
    assert(stream->data != nullptr);
    assert(out != nullptr);

    seek_block(stream, nth_block);
    return read(stream, out, stream->block_size);
}

size_t read_blocks(memory_stream *stream, void *out, size_t block_count)
{
    assert(stream != nullptr);
    assert(stream->data != nullptr);
    assert(out != nullptr);
    assert(stream->position < stream->size);

    return read(stream, out, stream->block_size, block_count);
}

size_t read_blocks(memory_stream *stream, void *out, size_t nth_block, size_t block_count)
{
    assert(stream != nullptr);
    assert(stream->data != nullptr);
    assert(out != nullptr);
    assert(stream->position < stream->size);

    seek_block(stream, nth_block);
    return read(stream, out, stream->block_size, block_count);
}

size_t copy_entire_stream(memory_stream *stream, void *out, size_t max_size)
{
    assert(stream != nullptr);
    assert(stream->data != nullptr);
    assert(out != nullptr);

    if (max_size > stream->size)
        max_size = stream->size;

    if (max_size > 0)
        memcpy(out, stream->data, max_size);

    return max_size;
}

size_t write(memory_stream *stream, const void *in, size_t size)
{
    assert(stream != nullptr);
    assert(stream->data != nullptr);
    assert(in != nullptr);
    assert(stream->position < stream->size);

    size_t write_size = size;
    size_t rest = stream->size - stream->position;

    if (write_size > rest)
        write_size = rest;

    if (write_size > 0)
    {
        memcpy(stream->data + stream->position, in, write_size);
        stream->position += write_size;
    }

    return write_size;
}

size_t write(memory_stream *stream, const void *in, size_t size, size_t nmemb)
{
    assert(stream != nullptr);
    assert(stream->data != nullptr);
    assert(in != nullptr);
    assert(stream->position < stream->size);

    size_t write_size = size * nmemb;
    size_t rest = stream->size - stream->position;

    if (write_size > rest)
        write_size = (rest / size) * size;

    if (write_size > 0)
    {
        memcpy(stream->data + stream->position, in, write_size);
        stream->position += write_size;
    }

    return write_size / size;
}

size_t write_at(memory_stream *stream, const void *in, size_t offset, size_t size)
{
    assert(stream != nullptr);
    assert(stream->data != nullptr);
    assert(in != nullptr);

    seek(stream, offset, SEEK_SET);
    return write(stream, in, size);
}

size_t write_at(memory_stream *stream, const void *in, size_t offset, size_t size, size_t nmemb)
{
    assert(stream != nullptr);
    assert(stream->data != nullptr);
    assert(in != nullptr);

    seek(stream, offset, SEEK_SET);
    return write(stream, in, size, nmemb);
}

size_t write_block(memory_stream *stream, const void *in)
{
    assert(stream != nullptr);
    assert(stream->data != nullptr);
    assert(in != nullptr);
    assert(stream->position < stream->size);

    return write(stream, in, stream->block_size);
}

size_t write_block(memory_stream *stream, const void *in, size_t nth_block)
{
    assert(stream != nullptr);
    assert(stream->data != nullptr);
    assert(in != nullptr);

    seek_block(stream, nth_block);
    return write(stream, in, stream->block_size);
}

size_t write_blocks(memory_stream *stream, const void *in, size_t block_count)
{
    assert(stream != nullptr);
    assert(stream->data != nullptr);
    assert(in != nullptr);
    assert(stream->position < stream->size);

    return write(stream, in, stream->block_size, block_count);
}

size_t write_blocks(memory_stream *stream, const void *in, size_t nth_block, size_t block_count)
{
    assert(stream != nullptr);
    assert(stream->data != nullptr);
    assert(in != nullptr);

    seek_block(stream, nth_block);
    return write(stream, in, stream->block_size, block_count);
}
