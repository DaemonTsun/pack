
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "fs/path.hpp"
#include "fs/iterator.hpp"
#include "shl/file_stream.hpp"
#include "shl/memory.hpp"
#include "shl/string.hpp"
#include "shl/format.hpp"
#include "shl/print.hpp"
#include "shl/error.hpp"
#include "shl/defer.hpp"
#include "pack/package.hpp"
#include "pack/package_writer.hpp"
#include "pack/package_reader.hpp"

#include "packer_info.hpp"

#define PACK_INDEX_EXTENSION "_index"

struct arguments
{
    bool verbose; // -v

    bool force; // -f
    bool extract; // -x
    bool generate_header; // -g
    bool list; // -l
    bool treat_index_as_file; // -i
    const_string out_path; // -o

    fs::path base_path; // -b, defaults to current working directory
    array<const_string> input_files; // anything thats not an arg
};

const arguments default_arguments
{
    .verbose = false,
    .force = false,
    .extract = false,
    .generate_header = false,
    .list = false,
    .treat_index_as_file = false,
    .out_path = const_string{nullptr, 0},
};

void init(arguments *args)
{
    init(&args->input_files);
}

void free(arguments *args)
{
    free(&args->input_files);
}

bool is_or_make_directory(fs::path *path)
{
    if (!fs::exists(path))
    {
        if (!fs::create_directories(path))
            throw_error("could not create directory: '%s'", path->c_str());

        return true;
    }
    else
    {
        if (!fs::is_directory(path))
            return false;

        return true;
    }
}

struct packer_path
{
    fs::path input_path;
    fs::path target_path; // the name it has inside the package
};

void add_path_files(fs::path *path, array<packer_path> *paths, arguments *args)
{
    if (args->verbose)
        tprint(" adding path '%s'\n", path->c_str());

    if (!fs::exists(path))
        throw_error("can't pack path because path does not exist: %s", path->c_str());

    if (fs::is_file(path))
    {
        if (args->treat_index_as_file)
        {
            fs::path rel;
            fs::relative_path(&args->base_path, path, &rel);
            add_at_end(paths, packer_path{*path, rel});
            return;
        }

        // check for index file, then get files from that
        const char *ext = fs::extension(path);

        if (!ends_with(ext, PACK_INDEX_EXTENSION))
        {
            fs::path rel;
            fs::relative_path(&args->base_path, path, &rel);
            add_at_end(paths, packer_path{*path, rel});
            return;
        }

        if (args->verbose)
            tprint(" adding entries of index file %s\n", path->c_str());

        array<fs::path> index_paths;
        init(&index_paths);
        defer { free(&index_paths); };

        file_stream findex;
        open(&findex, path->c_str());

        char* line = nullptr;
        size_t len = 0;

        while ((getline(&line, &len, findex.handle)) != -1)
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

            fs::path *pth = add_elements(&index_paths, 1);
            fs::set_path(pth, strline.c_str);
            fs::absolute_canonical_path(pth);
        }

        close(&findex);

        if (line != nullptr)
            free_memory(line);

        for_array(ipath, &index_paths)
            add_path_files(ipath, paths, args);
    }
    else if (fs::is_directory(path))
    {
        iterate_path(p, path)
        {
            fs::path epath(*p);
            fs::absolute_canonical_path(&epath);
            add_path_files(&epath, paths, args);
        }
    }
    else
        throw_error("cannot add unknown path %s", path->c_str());
}

char choice_prompt(const char *message, const char *choices, arguments *args)
{
    if (args->force)
        return *choices;

    char c;
    unsigned int ret;

    do
    {
        tprint("\n%s", message);
prompt_no_message:

        ret = getchar();

        if (is_space(static_cast<char>(ret)))
            goto prompt_no_message;

        if (ret == EOF)
            return 'x';

        c = to_lower(static_cast<char>(ret));
    }
    while (strchr(choices, c) == nullptr);

    return c;
}

void pack(arguments *args)
{
    if (args->out_path.size == 0)
        throw_error("no output file specified");

    fs::path outp(args->out_path.c_str);
    fs::absolute_path(&outp);

    if (fs::exists(&outp))
    {
        if (!fs::is_file(&outp))
            throw_error("output file exists but is not a file: ", outp);

        auto msg = tformat("output file %s already exists. overwrite? [y / n]: ", outp.c_str());
        char choice = choice_prompt(msg.c_str, "yn", args);

        if (choice != 'y')
        {
            put("aborting");
            exit(0);
        }
    }

    array<packer_path> paths;
    init(&paths);
    defer { free(&paths); };

    for_array(input_path, &args->input_files)
    {
        fs::path epath(input_path->c_str);
        fs::absolute_canonical_path(&epath);
        add_path_files(&epath, &paths, args);
    }

    if (args->verbose)
    {
        put("\nfiles:");

        for_array(pth, &paths)
            tprint("  %s -> %s\n", pth->input_path.c_str(), pth->target_path.c_str());
    }

    package_writer writer;
    init(&writer);
    defer { free(&writer); };
    
    for_array(pth, &paths)
        add_file(&writer, pth->input_path.c_str(), pth->target_path.c_str());

    write(&writer, outp.c_str());
}

void sanitize_name(string *s)
{
    printf("before: %s, %lu\n", s->data.data, s->data.size);
    replace_all(s, '.', '_'); 
    replace_all(s, '/', '_'); 

    u64 i = 0;

    while (s->data[i] == '_' && i < string_length(s))
        i++;

    if (i > 0)
        remove_elements(&s->data, 0, i);


    char c;
    for (u64 j = 0; j < string_length(s) - 1;)
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
            remove_elements(&s->data, j, 1);
            s->data[s->data.size] = '\0';
        }
        else ++j;
    }

    s->data[s->data.size] = '\0';

    printf("after: %s\n", s->data.data);
}

void generate_header(arguments *args)
{
    if (args->out_path.size == 0)
        throw_error("no output file specified");

    fs::path opath(args->out_path.c_str);

    if (fs::exists(&opath))
    {
        if (!fs::is_file(&opath))
            throw_error("not a writable file: ", opath);

        auto msg = tformat("generated header file %s exists, overwrite? [y / n]: ", opath.c_str());
        char choice = choice_prompt(msg.c_str, "yn", args);

        if (choice != 'y')
        {
            puts("aborting");
            exit(0);
        }
    }

    file_stream stream;
    
    if (!open(&stream, opath.c_str(), MODE_WRITE, false, false))
        throw_error("could not open '%s' for writing", opath.c_str());

    format(&stream, R"(// this file was generated by pack packer v%s

#pragma once
)", packer_VERSION);

    package_reader reader;
    package_reader_entry entry;

    for_array(path_, &args->input_files)
    {
        fs::path path(path_->c_str);
        fs::absolute_canonical_path(&path);

        if (!fs::is_file(&path))
            throw_error("not a file: %s", path.c_str());

        fs::path rel;
        fs::relative_path(&args->base_path, &path, &rel);

        if (args->verbose)
            tprint("reading toc of archive %s\n", path.c_str());

        string var_prefix = ""_s;
        copy_string(rel.c_str(), &var_prefix);
        sanitize_name(&var_prefix);

        read(&reader, rel.c_str());

        format(&stream, "\n#define %s \"%s\"\n", var_prefix.data.data, rel.c_str());
        format(&stream, "#define %s_file_count %u\n", var_prefix.data.data, reader.toc->entry_count);
        format(&stream, "static const char *%s_files[] = {\n", var_prefix.data.data);

        size_t maxnamelen = 0;

        for (u64 i = 0; i < reader.toc->entry_count; ++i)
        {
            get_package_entry(&reader, i, &entry);
            format(&stream, "    \"%s\",\n", entry.name);

            auto len = string_length(entry.name);

            if (len > maxnamelen)
                maxnamelen = len;
        };

        format(&stream, "};\n\n");

        char entry_format_str[256] = {0};
        sprintf(entry_format_str, "#define %%s__%%-%lus %%u\n", maxnamelen);

        string varname = ""_s;

        for (u64 i = 0; i < reader.toc->entry_count; ++i)
        {
            get_package_entry(&reader, i, &entry);

            if (args->verbose)
                tprint("  adding entry %s\n", entry.name);

            copy_string(entry.name, &varname);
            sanitize_name(&varname);

            format(&stream, entry_format_str, var_prefix.data.data, varname.data.data, i);

        }

        free(&varname);

        free(&var_prefix);
        free(&reader);
    }

    close(&stream);
}

void extract_archive(arguments *args, package_reader *reader)
{
    file_stream stream;
    init(&stream);
    package_reader_entry entry;
    fs::path outp(args->out_path.c_str);

    bool always_overwrite = false;
    bool never_overwrite = false;

    if (!::is_or_make_directory(&outp))
        throw_error("output directory not a directory: %s", outp.c_str());

    for (int i = 0; i < reader->toc->entry_count; ++i)
    {
        get_package_entry(reader, i, &entry);

        if ((entry.flags & PACK_TOC_FLAG_FILE) != PACK_TOC_FLAG_FILE)
        {
            if (args->verbose)
                tprint("  skipping non-file entry %d: %s\n", i, entry.name);

            continue;
        }

        fs::path epath;
        fs::append_path(&outp, entry.name, &epath);

        fs::path parent;
        fs::parent_path(&epath, &parent);

        if (!fs::exists(&parent))
        {
            if (!fs::create_directories(&parent))
                throw_error("  could not create parent directory '%s' for file entry '%s'", parent.c_str(), epath.c_str());
        }

        if (fs::exists(&epath))
        {
            if (never_overwrite)
            {
                tprint("skipping existing file %s\n", epath.c_str());
                continue;
            }

            if (!fs::is_file(&epath))
                throw_error("entry output path exists but is not a file: %s", epath.c_str());

            if (!always_overwrite)
            {
                auto msg = tformat("output file %s already exists. overwrite? [y / n / (a)lways overwrite / n(e)ver overwrite]: ", epath.c_str());
                char choice = choice_prompt(msg.c_str, "ynae", args);

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

        open(&stream, epath.c_str(), MODE_WRITE, true, false);
        write(&stream, entry.content, entry.size);
    }

    close(&stream);
}

void extract(arguments *args)
{
    package_reader reader;

    for_array(path, &args->input_files)
    {
        fs::path p(path->c_str);

        if (!fs::is_file(&p))
            throw_error("not a file: '%s'", path->c_str);

        if (args->verbose)
            tprint("extracting archive %s\n", path);

        read(&reader, path->c_str);
        extract_archive(args, &reader);
        free(&reader);
    }
}

#define max(a, b) ((a) >= (b) ? (a) : (b))

void print_package_reader_entry_flags(file_stream *out, const package_reader_entry *entry)
{
    size_t count = 0;

    if ((entry->flags & PACK_TOC_FLAG_FILE) == PACK_TOC_FLAG_FILE)
    {
        put(out->handle, 'F');
        count++;
    }

    format(out, "%.*s", max(8 - count, 0), "        ");
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

void list(arguments *args)
{
    file_stream out;

    if (args->out_path.size > 0)
    {
        if (!open(&out, args->out_path.c_str, MODE_WRITE, false, false))
            throw_error("could not open ", args->out_path, " for writing");
    }
    else
    {
        out.handle = stdout;
    }

    for_array(input, &args->input_files)
    {
        format(&out, "contents of package %s:\n", input->c_str);

        package_reader reader;
        read(&reader, input->c_str);

        format(&out, "%u entries found\n", reader.toc->entry_count);
        package_reader_entry entry;

        size_t digits = dec_digits(reader.toc->entry_count);
        char digit_fmt[16] = {0};
        sprintf(digit_fmt, "  %%0%lud ", digits);

        if (args->verbose)
            format(&out, "\n  %.*s flags    offset   size     name\n", digits, "n               ");
        else
            format(&out, "\n  %.*s flags    name\n", digits, "n               ");

        for (int i = 0; i < reader.toc->entry_count; ++i)
        {
            get_package_entry(&reader, i, &entry);
            format(&out, digit_fmt, i);
            print_package_reader_entry_flags(&out, &entry);
            
            if (args->verbose)
                format(&out, " %08x %08x", reinterpret_cast<char*>(entry.content) - reader.memory.data, entry.size);

            format(&out, " %s\n", entry.name);
        }

        free(&reader);
        put("");
    }
}

void show_help_and_exit()
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
  -g            Generate a C header file for the entires a given package.
  -l            List the contents of the input files.
  -i            Treat index files as normal files.
  -o <path>     The output file / path.
  -b <path>     Specifies the base path, all file paths will be relative to it.
                Only used in packing, not extracting.

  <files>       The input files.
)");

    exit(0);
}

const char *next_arg(int argc, char **argv, int *i)
{
    if (*i >= argc - 1)
        throw_error("argument '%s' missing parameter", argv[*i]);

    *i = *i + 1;
    return argv[*i];
}

void parse_arguments(int argc, char **argv, arguments *args)
{
    for (int i = 1; i < argc; ++i)
    {
        const_string arg = to_const_string(argv[i]);

        if (arg == "-h"_cs)
        {
            show_help_and_exit();
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
            args->out_path = to_const_string(next_arg(argc, argv, &i));
            continue;
        }

        if (arg == "-b"_cs)
        {
            args->base_path = fs::path(next_arg(argc, argv, &i));
            fs::absolute_path(&args->base_path);
            continue;
        }

        if (compare_strings(arg, "-"_cs, 1) == 0)
            throw_error("unexpected argument '%s'", arg);

        add_at_end(&args->input_files, arg);
    }
}

int main(int argc, char **argv)
try
{
    arguments args = default_arguments;
    init(&args);

    fs::get_current_path(&args.base_path);
    fs::absolute_path(&args.base_path);

    parse_arguments(argc, argv, &args);

    if (args.input_files.size == 0)
        throw_error("no input files");

    if (!is_or_make_directory(&args.base_path))
        throw_error("not a directory: '%s'", args.base_path.c_str());

    fs::absolute_canonical_path(&args.base_path);

    size_t action_count = 0;
    action_count += args.list ? 1 : 0;
    action_count += args.extract ? 1 : 0;
    action_count += args.generate_header ? 1 : 0;

    if (action_count > 1)
        throw_error("can only do one of extract (-x), generate header (-g) or list (-l)");

    if (args.list)
    {
        list(&args);
        return 0;
    }
    else if (args.generate_header)
        generate_header(&args);
    else if (args.extract)
        extract(&args);
    else
        pack(&args);

    free(&args);

    put("done");
    return 0;
}
catch (error &e)
{
    tprint(stderr, "error: %s\n", e.what);
    return 1;
}
catch (...)
{
    tprint(stderr, "error: unknown error\n");
    return 1;
}
