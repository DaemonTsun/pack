
#include <stdio.h>
#include <string.h>
#include <t1/t1.hpp>

#include "fs/path.hpp"
#include "shl/error.hpp"
#include "shl/string.hpp"
#include "pack/package_writer.hpp"
#include "pack/package_reader.hpp"
#include "pack/package_loader.hpp"

#include "testpack.h"

#define PATH_MAX 4096
char out_path[PATH_MAX] = {0};
char out_file[PATH_MAX] = {0};
char test_file1[PATH_MAX] = {0};
const char *test_filename1 = "/test_file.txt";

#define assert_flag_set(expr, flag)\
    assert_equal(expr & flag, flag)

void setup()
{
    fs::path o;
    fs::path parent;
    fs::get_executable_path(&o);
    fs::parent_path(&o, &parent);

    copy_string(o.c_str(), out_path, string_length(o.c_str()));

    fs::set_current_path(&parent);

    char *s = strrchr(out_path, '/');

    if (s != nullptr)
        *s = '\0';

    copy_string(out_path, out_file, PATH_MAX);
    copy_string(out_path, test_file1, PATH_MAX);

    u64 len = string_length(out_file);
    copy_string("/tmp", out_file + len, 4);
    strncpy(out_file + len, "/tmp", 4);

    copy_string(test_filename1, test_file1 + len, strlen(test_filename1)+1);
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

define_test(package_loader_loads_package_file)
{
    package_loader loader;

    fs::path pth(out_path);
    fs::append_path(&pth, testpack_pack);

    load_package_file(&loader, pth.c_str());

    assert_equal(testpack_pack_file_count, 1);

    memory_stream stream;
    init(&stream);

    load_entry(&loader, testpack_pack__test_file_txt, &stream);

    assert_equal(stream.size, 21u);

    load_entry(&loader, testpack_pack__test_file_txt, &stream);
    load_entry(&loader, testpack_pack__test_file_txt, &stream);
    load_entry(&loader, testpack_pack__test_file_txt, &stream);
    load_entry(&loader, testpack_pack__test_file_txt, &stream);
    load_entry(&loader, testpack_pack__test_file_txt, &stream);

    assert_equal(stream.size, 21u);

    free(&loader);
}

define_test(package_loader_loads_files)
{
    package_loader loader;

    load_files(&loader, testpack_pack_files, testpack_pack_file_count);

    assert_equal(testpack_pack_file_count, 1);

    memory_stream stream;
    init(&stream);

    load_entry(&loader, testpack_pack__test_file_txt, &stream);

    assert_equal(stream.size, 21u);

    load_entry(&loader, testpack_pack__test_file_txt, &stream);
    load_entry(&loader, testpack_pack__test_file_txt, &stream);
    load_entry(&loader, testpack_pack__test_file_txt, &stream);
    load_entry(&loader, testpack_pack__test_file_txt, &stream);
    load_entry(&loader, testpack_pack__test_file_txt, &stream);

    assert_equal(stream.size, 21u);

    free(&loader);
}

define_test_main(setup(), cleanup());
