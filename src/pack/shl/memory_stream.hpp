#pragma once

/* memory_stream
 * v1.2
 * add seek_next_alignment
 *
 * stream api for memory, similar to file_stream
 */

#include <stddef.h>

struct memory_stream
{
    char *data;
    size_t size;
    size_t block_size;
    size_t position;

    explicit operator char*() const;
};

void init(memory_stream *stream);

bool open(memory_stream *stream, size_t size, bool check_open = false, bool free_on_close = true);
bool open(memory_stream *stream, char *in, size_t size, bool check_open = false, bool free_on_close = true);
bool close(memory_stream *stream, bool free = true);
bool is_open(const memory_stream *stream);
bool is_at_end(const memory_stream *stream);

// is_open && !is_at_end
bool is_ok(const memory_stream *stream);

size_t block_count(const memory_stream *stream);
// returns a pointer in the data at the current position
char *current(const memory_stream *stream);
char *current_block_start(const memory_stream *stream);
size_t current_block_number(const memory_stream *stream);
size_t current_block_offset(const memory_stream *stream);

int seek(memory_stream *stream, long offset, int whence = SEEK_SET);
int seek_block(memory_stream *stream, long nth_block, int whence = SEEK_SET);
// seeks to next alignment if unaligned or does nothing if aligned
int seek_next_alignment(memory_stream *stream, size_t alignment);
size_t tell(memory_stream *stream);
bool getpos(memory_stream *stream, fpos_t *pos);
void rewind(memory_stream *stream);

// read & write perform memcpy
// returns number of bytes read
size_t read(memory_stream *stream, void *out, size_t size);
// returns number of items read
size_t read(memory_stream *stream, void *out, size_t size, size_t nmemb);

template<typename T>
size_t read(memory_stream *stream, T *out)
{
    return read(stream, out, sizeof(T));
}

// returns bytes read
size_t read_at(memory_stream *stream, void *out, size_t offset, size_t size);
// returns number of items read
size_t read_at(memory_stream *stream, void *out, size_t offset, size_t size, size_t nmemb);

template<typename T>
size_t read_at(memory_stream *stream, T *out, size_t offset)
{
    return read_at(stream, out, offset, sizeof(T));
}

// returns bytes read
size_t read_block(memory_stream *stream, void *out);
size_t read_block(memory_stream *stream, void *out, size_t nth_block);
// returns number of items read
size_t read_blocks(memory_stream *stream, void *out, size_t block_count);
size_t read_blocks(memory_stream *stream, void *out, size_t nth_block, size_t block_count);

size_t copy_entire_stream(memory_stream *stream, void *out, size_t max_size = -1u);

// returns number of bytes written
size_t write(memory_stream *stream, const void *in, size_t size);
// returns number of items written
size_t write(memory_stream *stream, const void *in, size_t size, size_t nmemb);
size_t write(memory_stream *stream, const char *str);

template<typename T>
size_t write(memory_stream *stream, const T *in)
{
    return write(stream, in, sizeof(T));
}

size_t write_at(memory_stream *stream, const void *in, size_t offset, size_t size);
size_t write_at(memory_stream *stream, const void *in, size_t offset, size_t size, size_t nmemb);
size_t write_at(memory_stream *stream, const char *str, size_t offset);

template<typename T>
size_t write_at(memory_stream *stream, const T *in, size_t offset)
{
    return write_at(stream, in, offset, sizeof(T));
}

size_t write_block(memory_stream *stream, const void *in);
size_t write_block(memory_stream *stream, const void *in, size_t nth_block);
size_t write_blocks(memory_stream *stream, const void *in, size_t block_count);
size_t write_blocks(memory_stream *stream, const void *in, size_t nth_block, size_t block_count);

// get does not perform memcpy
template<typename T>
void get(const memory_stream *stream, T **out)
{
    *out = reinterpret_cast<T*>(current(stream));
}

template<typename T>
void get_aligned(const memory_stream *stream, T **out)
{
    size_t pos = (stream->position / sizeof(T)) * sizeof(T)
    *out = reinterpret_cast<T*>(stream->data + pos);
}

template<typename T>
void get_at(const memory_stream *stream, T **out, size_t offset)
{
    *out = reinterpret_cast<T*>(stream->data + offset);
}
