
#include "t1/t1.hpp"
#include "fs/path.hpp"
#include "shl/error.hpp"
#include "shl/print.hpp"
#include "shl/string.hpp"
#include "shl/defer.hpp"
#include "pack/pack_writer.hpp"
#include "pack/pack_reader.hpp"
#include "pack/pack_loader.hpp"

#include "testpack.h"

fs::path out_path{};
fs::path out_file{};
fs::path test_file1{};
const char *test_filename1 = "test_file.txt";

#define assert_flag_set(expr, flag)\
    assert_equal(expr & flag, flag)

void setup()
{
    fs::path exe_dir{};
    defer { fs::free(&exe_dir); };

    fs::get_executable_directory_path(&exe_dir);

    fs::set_current_path(exe_dir);

    fs::set_path(&out_path, exe_dir);

    fs::set_path(&out_file, exe_dir);
    fs::append_path(&out_file, "tmp");

    fs::set_path(&test_file1, exe_dir);
    fs::append_path(&test_file1, test_filename1);
}

void cleanup()
{
    fs::free(&out_path);
    fs::free(&out_file);
    fs::free(&test_file1);
}

define_test(pack_writer_writes_value_entries)
{
    error err{};
    pack_writer writer{};
    defer { free(&writer); };

    u32 value = 8;
    const char *name = "num";

    pack_writer_add_entry(&writer, &value, name);

    assert_equal(pack_writer_write_to_file(&writer, out_file, &err), true);
    assert_equal(err.error_code, 0);

    pack_reader reader{};
    defer { free(&reader); };

    assert_equal(pack_reader_load_from_path(&reader, out_file, &err), true);
    assert_equal(err.error_code, 0);

    assert_equal(reader.toc->entry_count, 1);

    pack_reader_entry entry{};

    pack_reader_get_entry(&reader, 0, &entry);

    assert_equal(compare_strings(entry.name, name), 0);
    assert_equal(entry.size, (s64)sizeof(value));
    assert_equal(entry.flags, PACK_TOC_NO_FLAGS);
    assert_equal(*(u32*)(entry.content), value);
}

define_test(pack_writer_writes_value_entries2)
{
    error err{};
    pack_writer writer{};
    defer { free(&writer); };

    float value1 = 3.14f;
    const char *name1 = "f";
    const char *value2 = "abc";
    const char *name2 = "name of abc";

    pack_writer_add_entry(&writer, &value1, name1);
    pack_writer_add_entry(&writer, value2, name2);

    assert_equal(pack_writer_write_to_file(&writer, out_file, &err), true);
    assert_equal(err.error_code, 0);

    pack_reader reader{};
    defer { free(&reader); };

    assert_equal(pack_reader_load_from_path(&reader, out_file, &err), true);
    assert_equal(err.error_code, 0);

    assert_equal(reader.toc->entry_count, 2);

    pack_reader_entry entry{};
    pack_reader_get_entry(&reader, 0, &entry);

    assert_equal(compare_strings(entry.name, name1), 0);
    assert_equal(entry.size, (s64)sizeof(value1));
    assert_equal(entry.flags, PACK_TOC_NO_FLAGS);
    assert_equal(*(float*)(entry.content), value1);

    pack_reader_get_entry(&reader, 1, &entry);

    assert_equal(compare_strings(entry.name, name2), 0);
    assert_equal(entry.size, 3);
    assert_equal(entry.flags, PACK_TOC_NO_FLAGS);
    assert_equal(compare_strings((char*)(entry.content), value2, string_length(value2)), 0);
}

define_test(pack_writer_writes_files)
{
    error err{};
    pack_writer writer{};
    defer { free(&writer); };

    assert_equal(pack_writer_add_file(&writer, test_file1, true, &err), true);
    assert_equal(err.error_code, 0);

    assert_equal(pack_writer_write_to_file(&writer, out_file, &err), true);
    assert_equal(err.error_code, 0);

    pack_reader reader{};
    defer { free(&reader); };

    assert_equal(pack_reader_load_from_path(&reader, out_file, &err), true);
    assert_equal(err.error_code, 0);

    assert_equal(reader.toc->entry_count, 1);

    pack_reader_entry entry{};
    pack_reader_get_entry(&reader, 0, &entry);

    assert_equal(compare_strings(entry.name, to_const_string(test_file1)), 0);
    assert_equal(entry.size, 21);
    assert_flag_set(entry.flags, PACK_TOC_FLAG_FILE);
    assert_equal(compare_strings((char*)(entry.content), "This is a test file.\n"_cs), 0);
}

define_test(pack_loader_loads_package_file)
{
    error err{};
    pack_loader loader{};
    defer { free(&loader); };

    fs::path pth{};
    defer { free(&pth); };
    fs::set_path(&pth, out_path);
    fs::append_path(&pth, testpack_pack); // Defined in testpack.h

    assert_equal(pack_loader_load_package_file(&loader, pth.c_str(), &err), true);
    assert_equal(err.error_code, 0);

    assert_equal(testpack_pack_file_count, 1);

    pack_entry entry{};

    assert_equal(pack_loader_load_entry(&loader, testpack_pack__test_file_txt, &entry, &err), true);
    assert_equal(err.error_code, 0);

    assert_not_equal(entry.data, nullptr);
    assert_equal(entry.size, 21);

    pack_loader_load_entry(&loader, testpack_pack__test_file_txt, &entry);
    pack_loader_load_entry(&loader, testpack_pack__test_file_txt, &entry);
    pack_loader_load_entry(&loader, testpack_pack__test_file_txt, &entry);
    pack_loader_load_entry(&loader, testpack_pack__test_file_txt, &entry);
    pack_loader_load_entry(&loader, testpack_pack__test_file_txt, &entry);

    assert_not_equal(entry.data, nullptr);
    assert_equal(entry.size, 21u);
}

define_test(pack_loader_loads_files)
{
    error err{};
    pack_loader loader{};
    defer { free(&loader); };

    pack_loader_load_files(&loader, testpack_pack_files, testpack_pack_file_count);

    assert_equal(testpack_pack_file_count, 1);

    pack_entry entry{};
    assert_equal(pack_loader_load_entry(&loader, testpack_pack__test_file_txt, &entry, &err), true);
    assert_equal(err.error_code, 0);

    assert_not_equal(entry.data, (char*)nullptr);
    assert_equal(entry.size, 21);

    pack_loader_load_entry(&loader, testpack_pack__test_file_txt, &entry);
    pack_loader_load_entry(&loader, testpack_pack__test_file_txt, &entry);
    pack_loader_load_entry(&loader, testpack_pack__test_file_txt, &entry);
    pack_loader_load_entry(&loader, testpack_pack__test_file_txt, &entry);
    pack_loader_load_entry(&loader, testpack_pack__test_file_txt, &entry);

    assert_not_equal(entry.data, nullptr);
    assert_equal(entry.size, 21u);
}

define_test_main(setup, cleanup);
