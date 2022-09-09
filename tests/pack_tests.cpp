
#include <stdio.h>
#include <string.h>
#include <t1/t1.hpp>

#include "pack/shl/filesystem.hpp"
#include "pack/package_writer.hpp"
#include "pack/package_reader.hpp"

char out_path[PATH_MAX];
char out_file[PATH_MAX];
char test_file1[PATH_MAX];
const char *test_filename1 = "/test_file.txt";

#define assert_flag_set(expr, flag)\
    assert_equal(expr & flag, flag)

void setup()
{
    get_executable_path(out_path);

    char *s = strrchr(out_path, '/');

    if (s != nullptr)
        *s = '\0';

    strncpy(out_file, out_path, PATH_MAX);
    strncpy(test_file1, out_path, PATH_MAX);

    u64 len = strlen(out_file);
    strncpy(out_file + len, "/tmp", 4);

    strncpy(test_file1 + len, test_filename1, strlen(test_filename1)+1);
    printf("%s\n", test_file1);
}

void cleanup() {}

define_test(package_writer_writes_value_entries)
{
    package_writer writer;

    u32 value = 8;
    const char *name = "num";

    add_entry(&writer, value, name);

    write(&writer, out_file);
    free(&writer);

    package_reader reader;
    read(&reader, out_file);

    assert_equal(reader.toc->entry_count, 1);

    package_reader_entry entry;
    get_package_entry(&reader, 0, &entry);

    assert_equal(strcmp(entry.name, name), 0);
    assert_equal(entry.size, sizeof(value));
    assert_equal(entry.flags, PACK_TOC_NO_FLAGS);
    assert_equal(*reinterpret_cast<u32*>(entry.content), value);

    free(&reader);
}

define_test(package_writer_writes_value_entries2)
{
    package_writer writer;

    float value1 = 3.14f;
    const char *name1 = "f";
    const char *value2 = "abc";
    const char *name2 = "name of abc";

    add_entry(&writer, value1, name1);
    add_entry(&writer, value2, name2);

    write(&writer, out_file);
    free(&writer);

    package_reader reader;
    read(&reader, out_file);

    assert_equal(reader.toc->entry_count, 2);

    package_reader_entry entry;
    get_package_entry(&reader, 0, &entry);

    assert_equal(strcmp(entry.name, name1), 0);
    assert_equal(entry.size, sizeof(value1));
    assert_equal(entry.flags, PACK_TOC_NO_FLAGS);
    assert_equal(*reinterpret_cast<float*>(entry.content), value1);

    get_package_entry(&reader, 1, &entry);

    assert_equal(strcmp(entry.name, name2), 0);
    assert_equal(entry.size, 3);
    assert_equal(entry.flags, PACK_TOC_NO_FLAGS);
    assert_equal(strncmp(reinterpret_cast<char*>(entry.content), value2, strlen(value2)), 0);

    free(&reader);
}

define_test(package_writer_writes_files)
{
    package_writer writer;

    add_file(&writer, test_file1);

    write(&writer, out_file);
    free(&writer);

    package_reader reader;
    read(&reader, out_file);

    assert_equal(reader.toc->entry_count, 1);

    package_reader_entry entry;
    get_package_entry(&reader, 0, &entry);

    assert_equal(strcmp(entry.name, test_file1), 0);
    assert_equal(entry.size, 21u);
    assert_flag_set(entry.flags, PACK_TOC_FLAG_FILE);
    assert_equal(strncmp(reinterpret_cast<char*>(entry.content), "This is a test file.\n", 21u), 0);

    free(&reader);
}

define_test_main(setup(), cleanup());
