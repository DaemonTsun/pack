
#include <stdio.h> // snprintf, getline

[[noreturn]] extern void exit(int code);

#include "fs/path.hpp"
#include "shl/file_stream.hpp"
#include "shl/memory.hpp"
#include "shl/string.hpp"
#include "shl/format.hpp"
#include "shl/io.hpp"
#include "shl/compare.hpp"
#include "shl/print.hpp"
#include "shl/error.hpp"
#include "shl/defer.hpp"
#include "pack/package.hpp"
#include "pack/pack_writer.hpp"
#include "pack/pack_reader.hpp"

#include "packer_info.hpp"

#define PACK_INDEX_EXTENSION "_index"

#define stream_format(StreamPtr, ...) tprint((StreamPtr)->handle, __VA_ARGS__)

struct arguments
{
    bool verbose;           // -v
    bool force;             // -f
    bool extract;           // -x
    bool generate_header;   // -g
    bool list;              // -l
    bool treat_index_as_file; // -i
    fs::path out_path;      // -o
    fs::path base_path;     // -b, defaults to current working directory
    array<const_string> input_files; // anything thats not an arg
};

static const arguments default_arguments
{
    .verbose = false,
    .force = false,
    .extract = false,
    .generate_header = false,
    .list = false,
    .treat_index_as_file = false
};

static void init(arguments *args)
{
    assert(args != nullptr);
    fs::init(&args->out_path);
    fs::init(&args->base_path);
    init(&args->input_files);
}

static void free(arguments *args)
{
    assert(args != nullptr);
    fs::free(&args->out_path);
    fs::free(&args->base_path);
    free(&args->input_files);
}

struct packer_path
{
    fs::path input_path;
    fs::path target_path; // the name it has inside the package
};

static void free(packer_path *p)
{
    assert(p != nullptr);
    fs::free(&p->input_path);
    fs::free(&p->target_path);
}

static bool _add_path_files(fs::const_fs_string path, array<packer_path> *out_paths, arguments *args, error *err)
{
    if (args->verbose)
        tprint(" adding path '%s'\n", path.c_str);

    if (!fs::exists(path))
    {
        format_error(err, 1, "can't pack path because path does not exist: %s", path.c_str);
        return false;
    }

    if (fs::is_file(path))
    {
        if (args->treat_index_as_file)
        {
            packer_path *pp = ::add_at_end(out_paths);
            fill_memory(pp, 0);
            fs::set_path(&pp->input_path, path);
            fs::relative_path(&args->base_path, path, &pp->target_path);
            return true;
        }

        // check for index file, then get files from that
        fs::const_fs_string ext = fs::file_extension(path);

        if (!::ends_with(ext, PACK_INDEX_EXTENSION))
        {
            packer_path *pp = ::add_at_end(out_paths);
            fill_memory(pp, 0);
            fs::set_path(&pp->input_path, path);
            fs::relative_path(&args->base_path, path, &pp->target_path);
            return true;
        }

        if (args->verbose)
            tprint(" adding entries of index file %s\n", path.c_str);

        array<fs::path> index_paths{};
        defer { free<true>(&index_paths); };

        FILE *f = fopen(path.c_str, "r");
        defer { fclose(f); };

        char* line = nullptr;
        size_t len = 0;

        while ((getline(&line, &len, f)) != -1)
        {
            if (len == 0)
                continue;

            const_string strline = to_const_string(line);

            if (strline.size > 0)
                strline.size--;

            if (is_blank(strline))
                continue;

            if (begins_with(strline, "##"_cs))
                continue;

            fs::path *index_path = ::add_at_end(&index_paths);
            fill_memory(index_path, 0);

            if (!fs::weakly_canonical_path(strline, index_path, err))
                return false;
        }

        if (line != nullptr)
            _libc_free(line);

        for_array(ipath, &index_paths)
            if (!_add_path_files(to_const_string(ipath), out_paths, args, err))
                return false;
    }
    else if (fs::is_directory(path))
    {
        for_path(it, path, fs::iterate_option::Fullpaths)
        {
            if (!_add_path_files(it->path, out_paths, args, err))
                return false;
        }
    }
    else
    {
        format_error(err, 2, "cannot add unknown path %s", path.c_str);
        return false;
    }

    return true;
}

static char _getchar(error *err)
{
    char ret = 0;

    if (io_read(stdin_handle(), &ret, 1, err) != 1)
        return (char)-1;

    return ret;
}

static char _choice_prompt(const char *message, const char *choices, arguments *args, error *err)
{
    if (args->force)
        return *choices;

    char c;
    char input;

    do
    {
        put("\n");
        put(message);

        do
        {
            input = _getchar(err);
        }
        while (is_space(input) && input != (char)-1);

        if (input == (char)-1)
            return 'x';

        c = to_lower(input);
    }
    while (index_of(choices, c) == -1);

    return c;
}

static bool _pack(arguments *args, error *err)
{
    if (args->out_path.size == 0)
    {
        set_error(err, 1, "no output file specified");
        return false;
    }

    fs::path outp{};
    fs::weakly_canonical_path(args->out_path, &outp);
    defer { fs::free(&outp); };

    if (fs::exists(&outp))
    {
        if (!fs::is_file(&outp))
        {
            format_error(err, 2, "output file exists but is not a file: %s", outp.c_str());
            return false;
        }

        auto msg = tformat("output file %s already exists. overwrite? [y / n]: ", outp.c_str());
        char choice = _choice_prompt(msg.c_str, "yn", args, err);

        if (choice != 'y')
        {
            put("aborting");
            exit(0);
        }
    }

    array<packer_path> paths{};
    defer { free<true>(&paths); };

    fs::path epath{};
    defer { fs::free(&epath); };

    for_array(input_path, &args->input_files)
    {
        if (!fs::weakly_canonical_path(to_const_string(*input_path), &epath, err))
            return false;

        if (!_add_path_files(to_const_string(epath), &paths, args, err))
            return false;
    }

    if (args->verbose)
    {
        put("\nfiles:\n");

        for_array(pth, &paths)
            tprint("  %s -> %s\n", pth->input_path.c_str(), pth->target_path.c_str());
    }

    pack_writer writer{};
    init(&writer);
    defer { free(&writer); };
    
    for_array(pth, &paths)
        if (!pack_writer_add_file(&writer, pth->input_path.c_str(), pth->target_path.c_str(), true, err))
            return false;

    return pack_writer_write_to_file(&writer, outp.c_str(), err);
}

static void _sanitize_name(string *s)
{
    // printf("before: %s, %lu\n", s->data, s->data.size);
    replace_all(s, '.', '_'); 
    replace_all(s, '/', '_'); 

    s64 i = 0;

    while (s->data[i] == '_' && i < s->size)
        i++;

    if (i > 0)
        remove_elements((array<char>*)s, 0, i);

    char c;
    for (s64 j = 0; j < s->size - 1;)
    {
        c = s->data[j];

        if (c != '_')
        {
            ++j;
            continue;
        }

        if (c == s->data[j + 1])
        {
            // slow but whatever
            remove_elements((array<char>*)s, j, 1);
            s->data[s->size] = '\0';
        }
        else ++j;
    }

    s->data[s->size] = '\0';
    // printf("after: %s\n", s->data);
}

static bool _generate_header(arguments *args, error *err)
{
    if (args->out_path.size == 0)
    {
        set_error(err, 1, "no output file specified");
        return false;
    }

    fs::path *opath = &args->out_path;

    if (fs::exists(opath))
    {
        if (!fs::is_file(opath))
        {
            format_error(err, 2, "not a writable file: %s", opath->c_str());
            return false;
        }

        auto msg = tformat("generated header file %s exists, overwrite? [y / n]: ", opath->c_str());
        char choice = _choice_prompt(msg.c_str, "yn", args, err);

        if (choice != 'y')
        {
            puts("aborting");
            exit(0);
        }
    }

    file_stream stream{};
    defer { free(&stream); };
    
    if (!init(&stream, opath->c_str(), open_mode::WriteTrunc, err))
        return false;

    stream_format(&stream, R"(// this file was generated by pack packer v%s

#pragma once
)", packer_VERSION);

    pack_reader reader{};
    defer { free(&reader); };

    pack_reader_entry entry{};

    fs::path input_path{};
    defer { fs::free(&input_path); };

    fs::path rel{};
    defer { fs::free(&rel); };

    string var_prefix{};
    defer { free(&var_prefix); };

    string var_name{};
    defer { free(&var_name); };

    for_array(path_, &args->input_files)
    {
        if (!fs::weakly_canonical_path(*path_, &input_path, err))
            return false;

        if (!fs::is_file(&input_path))
        {
            format_error(err, 3, "not a file: %s", input_path.c_str());
            return false;
        }

        fs::relative_path(&args->base_path, &input_path, &rel);

        if (args->verbose)
            tprint("reading toc of archive %s\n", input_path.c_str());

        set_string(&var_prefix, to_const_string(rel));
        _sanitize_name(&var_prefix);

        free(&reader);
        init(&reader);

        if (!pack_reader_load_from_path(&reader, rel.c_str(), err))
            return false;

        stream_format(&stream, "\n#define %s \"%s\"\n", var_prefix.data, rel.c_str());
        stream_format(&stream, "#define %s_file_count %u\n", var_prefix.data, reader.toc->entry_count);
        stream_format(&stream, "[[maybe_unused]] static const char *%s_files[] = {\n", var_prefix.data);

        // find max entry name length
        s64 maxnamelen = 0;

        for (s64 i = 0; i < reader.toc->entry_count; ++i)
        {
            pack_reader_get_entry(&reader, i, &entry);
            stream_format(&stream, "    \"%s\",\n", entry.name);

            s64 len = string_length(entry.name);

            if (len > maxnamelen)
                maxnamelen = len;
        };

        write(&stream, "};\n\n", 4);

        char entry_format_str[256] = {0};
        sprintf(entry_format_str, "#define %%s__%%-%lus %%u\n", maxnamelen);

        for (s64 i = 0; i < reader.toc->entry_count; ++i)
        {
            pack_reader_get_entry(&reader, i, &entry);

            if (args->verbose)
                tprint("  adding entry %s\n", entry.name);

            set_string(&var_name, entry.name);
            _sanitize_name(&var_name);

            stream_format(&stream, entry_format_str, var_prefix.data, var_name.data, i);
        }
    }

    return true;
}

static bool _extract_package(arguments *args, pack_reader *reader, error *err)
{
    pack_reader_entry entry{};
    fs::path outp{};
    fs::set_path(&outp, args->out_path);
    defer { fs::free(&outp); };

    fs::path epath{};
    defer { fs::free(&epath); };

    fs::path parent{};
    defer { fs::free(&parent); };

    bool always_overwrite = false;
    bool never_overwrite = false;

    if (!fs::exists(&outp) && !fs::create_directories(&outp, fs::permission::User, err))
        return false;

    for (s64 i = 0; i < reader->toc->entry_count; ++i)
    {
        pack_reader_get_entry(reader, i, &entry);

        if ((entry.flags & PACK_TOC_FLAG_FILE) != PACK_TOC_FLAG_FILE)
        {
            if (args->verbose)
                tprint("  skipping non-file entry %d: %s\n", i, entry.name);

            continue;
        }

        fs::set_path(&epath, outp);
        fs::append_path(&epath, entry.name);

        fs::parent_path(&epath, &parent);

        if (!fs::exists(&parent) && !fs::create_directories(&parent, fs::permission::User, err))
            return false;

        if (fs::exists(&epath))
        {
            if (never_overwrite)
            {
                tprint("skipping existing file %\n", to_const_string(epath));
                continue;
            }

            if (!fs::is_file(&epath))
            {
                format_error(err, 1, "entry output path exists but is not a file: %s", epath.data);
                return false;
            }

            if (!always_overwrite)
            {
                auto msg = tformat("output file %s already exists. overwrite? [y / n / (a)lways overwrite / n(e)ver overwrite]: ", epath.c_str());
                char choice = _choice_prompt(msg.c_str, "ynae", args, err);

                if (choice == 'n')
                {
                    put("skipping");
                    continue;
                }
                else if (choice == 'a')
                {
                    put("always overwriting");
                    always_overwrite = true;
                }
                else if (choice == 'e')
                {
                    put("never overwriting");
                    tprint("skipping existing file %s\n", epath.c_str());
                    never_overwrite = true;
                    continue;
                }
                else if (choice == 'x')
                {
                    put("aborting");
                    exit(0);
                }
            }
        }

        if (args->verbose)
            printf("  %08lx bytes %s\n", entry.size, epath.c_str());

        io_handle h = io_open(epath.c_str(), open_mode::WriteTrunc, err);
        
        if (h == INVALID_IO_HANDLE)
            return false;

        defer { io_close(h); };

        if (io_write(h, entry.content, entry.size, err) == -1)
            return false;
    }

    return true;
}

static bool _extract_packages(arguments *args, error *err)
{
    fs::path p{};
    defer { fs::free(&p); };

    for_array(path, &args->input_files)
    {
        fs::set_path(&p, path->c_str);

        if (!fs::is_file(&p))
        {
            format_error(err, 1, "not a file: '%s'", path->c_str);
            return false;
        }

        if (args->verbose)
            tprint("extracting archive %s\n", path);

        pack_reader reader{};
        defer { free(&reader); };

        if (!pack_reader_load_from_path(&reader, path->c_str, err))
            return false;

        if (!_extract_package(args, &reader, err))
            return false;
    }

    return true;
}

static void _print_pack_reader_entry_flags(file_stream *out, const pack_reader_entry *entry)
{
    s64 count = 0;

    if ((entry->flags & PACK_TOC_FLAG_FILE) == PACK_TOC_FLAG_FILE)
    {
        put(out->handle, 'F');
        count++;
    }

    stream_format(out, "%.*s", Max(8 - count, (s64)0), "        ");
}

template<typename T>
constexpr inline T dec_digits(T x)
{
    T i = 0;

    while (x > 0)
    {
        x = x / 10;
        ++i;
    }

    return i;
}

static bool _list_package_contents(arguments *args, error *err)
{
    file_stream out{};
    out.handle = stdout_handle();

    if (args->out_path.size > 0)
    {
        if (!init(&out, args->out_path.c_str(), open_mode::Write, err))
            return false;
    }

    for_array(input, &args->input_files)
    {
        stream_format(&out, "contents of package %s:\n", input->c_str);

        pack_reader reader{};
        if (!pack_reader_load_from_path(&reader, input->c_str, err))
            return false;

        defer { free(&reader); };

        stream_format(&out, "% entries found\n", reader.toc->entry_count);
        pack_reader_entry entry{};

        s64 digits = dec_digits(reader.toc->entry_count);
        char digit_fmt[16] = {0};
        format(digit_fmt, 15, "  \%0%ldd ", digits);

        if (args->verbose)
            stream_format(&out, "\n  %.*s flags    offset   size     name\n", digits, "n               ");
        else
            stream_format(&out, "\n  %.*s flags    name\n", digits, "n               ");

        for (s64 i = 0; i < reader.toc->entry_count; ++i)
        {
            pack_reader_get_entry(&reader, i, &entry);
            stream_format(&out, digit_fmt, i);
            _print_pack_reader_entry_flags(&out, &entry);
            
            if (args->verbose)
                stream_format(&out, " %08x %08x", (char*)(entry.content) - reader.content, entry.size);

            stream_format(&out, " %s\n", entry.name);
        }
    }

    return true;
}

static void _show_help_and_exit()
{
    put(packer_NAME R"( [-h] [-v] [-x | -g | -l] [-i] [-b <path>] -o <path> <files...>
  v)"   packer_VERSION R"(
  by )" packer_AUTHOR R"(

Packs, extracts or lists contents of archives.

ARGUMENTS:
  -h            Show this help and exit.
  -v            Show verbose output.
  -f            Force overwrite any files without prompting.
  -x            Extract instead of pack.
  -g            Generate a C header file of the entries of a given package.
  -l            List the contents of the input files.
  -i            Treat index files as normal files. Used when adding index files to
                a package.
  -o <path>     The output file / path.
  -b <path>     Specifies the base path, all file paths will be relative to it.
                Only used in packing, not extracting.

  <files>       The input files.
)");

    exit(0);
}

#define _next_arg(Var, Argc, Argv, I)\
    {\
        if ((I) >= (Argc) - 1)\
        {\
            format_error(err, 1, "argument '%s' missing parameter", (Argv)[(I)]);\
            return false;\
        }\
    \
        I += 1;\
        Var = (Argv)[(I)];\
    }

static bool _parse_arguments(int argc, char **argv, arguments *args, error *err)
{
    for (int i = 1; i < argc; ++i)
    {
        const_string arg = to_const_string(argv[i]);

        if (arg == "-h"_cs)
        {
            _show_help_and_exit();
            continue;
        }

        if (arg == "-v"_cs)
        {
            args->verbose = true;
            continue;
        }

        if (arg == "-f"_cs)
        {
            args->force = true;
            continue;
        }

        if (arg == "-x"_cs)
        {
            args->extract = true;
            continue;
        }

        if (arg == "-g"_cs)
        {
            args->generate_header = true;
            continue;
        }

        if (arg == "-l"_cs)
        {
            args->list = true;
            continue;
        }

        if (arg == "-i"_cs)
        {
            args->treat_index_as_file = true;
            continue;
        }

        if (arg == "-o"_cs)
        {
            const char *narg;
            _next_arg(narg, argc, argv, i);
            fs::set_path(&args->out_path, narg);
            continue;
        }

        if (arg == "-b"_cs)
        {
            const char *narg;
            _next_arg(narg, argc, argv, i);

            if (!fs::absolute_path(narg, &args->base_path, err))
                return false;

            continue;
        }

        if (compare_strings(arg, "-"_cs, 1) == 0)
        {
            format_error(err, 1, "unexpected argument '%s'", arg);
            return false;
        }

        add_at_end(&args->input_files, arg);
    }

    return true;
}

static bool _main(int argc, char **argv, error *err)
{
    arguments args = default_arguments;
    init(&args);
    defer { free(&args); };

    if (!_parse_arguments(argc, argv, &args, err))
        return false;

    if (args.base_path.size == 0)
    {
        if (!fs::get_current_path(&args.base_path, err))
            return false;
    }

    if (args.input_files.size == 0)
    {
        set_error(err, 1, "no input files");
        return false;
    }

    if (!fs::exists(&args.base_path) && !fs::create_directories(&args.base_path, fs::permission::User, err))
        return false;

    s32 action_count = 0;
    action_count += args.list ? 1 : 0;
    action_count += args.extract ? 1 : 0;
    action_count += args.generate_header ? 1 : 0;

    if (action_count > 1)
    {
        set_error(err, 2, "can only do one of extract (-x), generate header (-g) or list (-l)");
        return false;
    }

    bool ret = false;

    if (args.list)
        return _list_package_contents(&args, err);
    else if (args.generate_header)
        ret = _generate_header(&args, err);
    else if (args.extract)
        ret = _extract_packages(&args, err);
    else
        ret = _pack(&args, err);

    if (!ret)
        return false;

    put("done");
    return true;
}

int main(int argc, char **argv)
{
    error err{};

    if (!_main(argc, argv, &err))
    {
#ifndef NDEBUG
        tprint("[%:%] error %: %\n", err.file, (s64)err.line, err.error_code, err.what);
#else
        tprint("error %: %\n", err.error_code, err.what);
#endif
        return 1;
    }

    return 0;
}
