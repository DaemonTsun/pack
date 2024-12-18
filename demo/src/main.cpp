
#include "shl/error.hpp"
#include "shl/print.hpp"
#include "pack/pack_loader.hpp"

#include "gen/testpack.h"

static bool _run(error *err)
{
    pack_loader loader{};
    defer { free(&loader); };

    init(&loader);

    fs::path exe_dir{};
    defer { fs::free(&exe_dir); };
    fs::get_executable_directory_path(&exe_dir);

#ifndef NDEBUG
    pack_loader_load_files(&loader, testpack_pack_files, testpack_pack_file_count, exe_dir.c_str());
#else
    fs::path_append(&exe_dir, testpack_pack);
    if (!pack_loader_load_package_file(&loader, exe_dir.c_str(), err))
        return false;
#endif

    pack_entry txt_entry{};

    if (!pack_loader_load_entry(&loader, testpack_pack__res_file1_txt, &txt_entry, err))
        return false;

    tprint("%s\n", const_string{txt_entry.data, txt_entry.size});
    tprint("\nFiles found:\n");

    for (s64 i = 0; i < pack_loader_entry_count(&loader); ++i)
        tprint("% %\n", i, pack_loader_entry_name(&loader, i));

    return true;
}

int main(int argc, const char *argv[])
{
    (void)argc;
    (void)argv;

    error err{};

    if (!_run(&err))
    {
        tprint("error: %s\n", err.what);
        return err.error_code;
    }

    return 0;
}
