#pragma once

/* file_stream
 * v1.0
 *
 * more consistent file stream functions
 */

#include <stdio.h>

#define MODE_READ "r"
#define MODE_WRITE "w"
#define MODE_BINARY "b"

struct file_stream
{
    FILE *handle;
    size_t size;
    size_t block_size;

    explicit operator FILE*() const;
};

void init(file_stream *stream);

bool open(file_stream *stream, const char *path, const char *mode = MODE_READ, bool check_open = false, bool calc_size = true);
bool close(file_stream *stream);
bool is_open(const file_stream *stream);
bool is_at_end(file_stream *stream);
bool has_error(file_stream *stream);
void clear_error(file_stream *stream);

// is_open && !is_at_end && !has_error
bool is_ok(file_stream *stream);

// this calculates and sets the size of the stream
size_t calculate_file_size(file_stream *stream);
size_t block_count(const file_stream *stream);

int seek(file_stream *stream, long offset, int whence = SEEK_SET);
int seek_block(file_stream *stream, long nth_block, int whence = SEEK_SET);
size_t tell(file_stream *stream);
bool getpos(file_stream *stream, fpos_t *pos);
bool rewind(file_stream *stream);

// returns bytes read
size_t read(file_stream *stream, void *out, size_t size);
// returns number of items read
size_t read(file_stream *stream, void *out, size_t size, size_t nmemb);

template<typename T>
size_t read(file_stream *stream, T *out)
{
    return read(stream, out, sizeof(T));
}

// returns bytes read
size_t read_at(file_stream *stream, void *out, size_t offset, size_t size);
// returns number of items read
size_t read_at(file_stream *stream, void *out, size_t offset, size_t size, size_t nmemb);

template<typename T>
size_t read_at(file_stream *stream, T *out, size_t offset)
{
    return read_at(stream, out, offset, sizeof(T));
}

// returns bytes read
size_t read_block(file_stream *stream, void *out);
size_t read_block(file_stream *stream, void *out, size_t nth_block);
// returns number of items read
size_t read_blocks(file_stream *stream, void *out, size_t block_count);
size_t read_blocks(file_stream *stream, void *out, size_t nth_block, size_t block_count);

// uses stream->size, make sure its set and out has enough space
size_t read_entire_file(file_stream *stream, void *out, size_t max_size = -1u);

// returns number of bytes written
size_t write(file_stream *stream, const void *in, size_t size);
// returns number of items written
size_t write(file_stream *stream, const void *in, size_t size, size_t nmemb);
size_t write(file_stream *stream, const char *str);
size_t write_at(file_stream *stream, const void *in, size_t offset, size_t size);
size_t write_at(file_stream *stream, const void *in, size_t offset, size_t size, size_t nmemb);
size_t write_at(file_stream *stream, const char *str, size_t offset);
size_t write_block(file_stream *stream, const void *in);
size_t write_block(file_stream *stream, const void *in, size_t nth_block);
size_t write_blocks(file_stream *stream, const void *in, size_t block_count);
size_t write_blocks(file_stream *stream, const void *in, size_t nth_block, size_t block_count);

int format(FILE *stream, const char *format_string, ...);
int format(file_stream *stream, const char *format_string, ...);

int flush(file_stream *stream);

