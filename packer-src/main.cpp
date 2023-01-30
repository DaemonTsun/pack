
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <algorithm> // fuc
#include <filesystem> // guess i failed fs
#include <vector>

#include "fs/path.hpp"
#include "shl/file_stream.hpp"
#include "shl/string.hpp"
#include "shl/string_manip.hpp"
#include "shl/error.hpp"
#include "pack/package.hpp"
#include "pack/package_writer.hpp"
#include "pack/package_reader.hpp"

#include "config.hpp"

#define PACK_INDEX_EXTENSION "_index"

struct arguments
{
    bool verbose; // -v

    bool force; // -f
    bool extract; // -x
    bool generate_header; // -g
    bool list; // -l
    bool treat_index_as_file; // -i
    const char *out_path; // -o

    fs::path base_path; // -b, defaults to current working directory
    std::vector<const char *> input_files; // anything thats not an arg
};

const arguments default_arguments
{
    .verbose = false,
    .force = false,
    .extract = false,
    .generate_header = false,
    .list = false,
    .treat_index_as_file = false,
    .out_path = nullptr,
};

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

void add_path_files(fs::path *path, std::vector<packer_path> *paths, arguments *args)
{
    if (args->verbose)
        printf(" adding path '%s'\n", path->c_str());

    if (!fs::exists(path))
        throw_error("can't pack path because path does not exist: %s", path->c_str());

    if (fs::is_file(path))
    {
        if (args->treat_index_as_file)
        {
            fs::path rel;
            fs::relative_path(&args->base_path, path, &rel);
            paths->push_back(packer_path{*path, rel});
            return;
        }

        // check for index file, then get files from that
        const char *ext = fs::extension(path);

        if (!ends_with(ext, PACK_INDEX_EXTENSION))
        {
            fs::path rel;
            fs::relative_path(&args->base_path, path, &rel);
            paths->push_back(packer_path{*path, rel});
            return;
        }

        if (args->verbose)
            printf(" adding entries of index file %s\n", path->c_str());

        std::vector<fs::path> index_paths;

        file_stream findex;
        open(&findex, path->c_str());

        char* line = nullptr;
        size_t len = 0;

        while ((getline(&line, &len, findex.handle)) != -1)
        {
            if (len == 0)
                continue;

            std::string strline = std::string(line, string_length(line) - 1);

            if (is_blank(strline.c_str()))
                continue;

            if (begins_with(strline.c_str(), "##"))
                continue;

            fs::path *pth = &index_paths.emplace_back(strline.c_str());
            fs::absolute_canonical_path(pth, pth);
        }

        close(&findex);

        if (line != nullptr)
            free(line);

        for (auto &ipath : index_paths)
            add_path_files(&ipath, paths, args);
    }
    else if (fs::is_directory(path))
        for (const auto &entry : std::filesystem::directory_iterator(path->c_str()))
        {
            fs::path epath(entry.path().c_str());
            fs::absolute_canonical_path(&epath, &epath);
            add_path_files(&epath, paths, args);
        }
    else
        throw_error("cannot add unknown path ", *path);
}

char choice_prompt(const char *message, const char *choices, arguments *args)
{
    if (args->force)
        return *choices;

    char c;
    unsigned int ret;

    do
    {
        printf("\n%s", message);
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
    if (args->out_path == nullptr)
        throw_error("no output file specified");

    fs::path outp(args->out_path);
    fs::absolute_path(&outp, &outp);

    if (fs::exists(&outp))
    {
        if (!fs::is_file(&outp))
            throw_error("output file exists but is not a file: ", outp);

        std::string msg = to_string("output file ", outp, " already exists. overwrite? [y / n]: ");
        char choice = choice_prompt(msg.c_str(), "yn", args);

        if (choice != 'y')
        {
            puts("aborting");
            exit(0);
        }
    }

    std::vector<packer_path> paths;

    for (const char *path : args->input_files)
    {
        fs::path epath(path);
        fs::absolute_canonical_path(&epath, &epath);
        add_path_files(&epath, &paths, args);
    }

    if (args->verbose)
    {
        puts("\nfiles:");

        for (const auto &path : paths)
            printf("  %s -> %s\n", path.input_path.c_str(), path.target_path.c_str());
    }

    package_writer writer;
    
    for (const auto &path : paths)
        add_file(&writer, path.input_path.c_str(), path.target_path.c_str());

    write(&writer, outp.c_str());
    free(&writer);
}

std::string sanitize_name(std::string s)
{
    std::replace(s.begin(), s.end(), '.', '_');
    std::replace(s.begin(), s.end(), '/', '_');

    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](auto ch)
    {
        return ch != '_';
    }));

    for (unsigned i = 0; i < s.length() - 1;)
    {
        if (s[i] == s[i + 1])
            s.erase(i, 2);
        else ++i;
    }

    return s;
}

void generate_header(arguments *args)
{
    if (args->out_path == nullptr)
        throw_error("no output file specified");

    fs::path opath = args->out_path;

    if (fs::exists(&opath))
    {
        if (!fs::is_file(&opath))
            throw_error("not a writable file: ", opath);

        std::string msg = to_string("generated header file ", opath.c_str(), " exists, overwrite? [y / n]: ");
        char choice = choice_prompt(msg.c_str(), "yn", args);

        if (choice != 'y')
        {
            puts("aborting");
            exit(0);
        }
    }

    file_stream stream;
    
    if (!open(&stream, opath.c_str(), MODE_WRITE, false, false))
        throw_error("could not open ", opath, " for writing");

    format(&stream, R"(// this file was generated by pack packer v%s

#pragma once
)", PACKER_VERSION);

    package_reader reader;
    package_reader_entry entry;

    for (const char *path_ : args->input_files)
    {
        fs::path path(path_);
        fs::absolute_canonical_path(&path, &path);

        if (!fs::is_file(&path))
            throw_error("not a file: ", path);

        fs::path rel;
        fs::relative_path(&args->base_path, &path, &rel);

        if (args->verbose)
            printf("reading toc of archive %s\n", path.c_str());

        std::string var_prefix = sanitize_name(rel.c_str());
        read(&reader, rel.c_str());

        format(&stream, "\n#define %s \"%s\"\n", var_prefix.c_str(), rel.c_str());
        format(&stream, "#define %s_file_count %u\n", var_prefix.c_str(), reader.toc->entry_count);
        format(&stream, "static const char *%s_files[] = {\n", var_prefix.c_str());

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
        sprintf(entry_format_str, "#define %%s__%%-%us %%u\n", maxnamelen);

        for (u64 i = 0; i < reader.toc->entry_count; ++i)
        {
            get_package_entry(&reader, i, &entry);

            if (args->verbose)
                printf("  adding entry %s\n", entry.name);

            std::string varname = sanitize_name(entry.name);


            format(&stream, entry_format_str, var_prefix.c_str(), varname.c_str(), i);
        }

        free(&reader);
    }

    close(&stream);
}

void extract_archive(arguments *args, package_reader *reader)
{
    file_stream stream;
    init(&stream);
    package_reader_entry entry;
    fs::path outp = args->out_path;

    bool always_overwrite = false;
    bool never_overwrite = false;

    if (!is_or_make_directory(&outp))
        throw_error("output directory not a directory: ", outp);

    for (int i = 0; i < reader->toc->entry_count; ++i)
    {
        get_package_entry(reader, i, &entry);

        if ((entry.flags & PACK_TOC_FLAG_FILE) != PACK_TOC_FLAG_FILE)
        {
            if (args->verbose)
                printf("  skipping non-file entry %d: %s\n", i, entry.name);

            continue;
        }

        fs::path epath;
        fs::append_path(&outp, entry.name, &epath);

        fs::path parent;
        fs::parent_path(&epath, &parent);

        if (!fs::exists(&parent))
        {
            if (!fs::create_directories(&parent))
                throw_error("  could not create parent directory ", parent, " for file entry ", epath);
        }

        if (fs::exists(&epath))
        {
            if (never_overwrite)
            {
                printf("skipping existing file %s\n", epath.c_str());
                continue;
            }

            if (!fs::is_file(&epath))
                throw_error("entry output path exists but is not a file: ", epath);

            if (!always_overwrite)
            {
                auto msg = to_string("output file ", epath, " already exists. overwrite? [y / n / (a)lways overwrite / n(e)ver overwrite]: ");
                char choice = choice_prompt(msg.c_str(), "ynae", args);

                if (choice == 'n')
                {
                    puts("skipping");
                    continue;
                }
                else if (choice == 'a')
                {
                    puts("always overwriting");
                    always_overwrite = true;
                }
                else if (choice == 'e')
                {
                    puts("never overwriting");
                    printf("skipping existing file %s\n", epath.c_str());
                    never_overwrite = true;
                    continue;
                }
                else if (choice == 'x')
                {
                    puts("aborting");
                    exit(0);
                }
            }
        }

        if (args->verbose)
            printf("  %08x bytes %s\n", entry.size, epath.c_str());

        open(&stream, epath.c_str(), MODE_WRITE, true, false);
        write(&stream, entry.content, entry.size);
    }

    close(&stream);
}

void extract(arguments *args)
{
    package_reader reader;

    for (const char *path : args->input_files)
    {
        fs::path p(path);

        if (!fs::is_file(&p))
            throw_error("not a file: '", path, "'");

        if (args->verbose)
            printf("extracting archive %s\n", path);

        read(&reader, path);
        extract_archive(args, &reader);
        free(&reader);
    }
}

#define max(a, b) (a >= b ? a : b)

void print_package_reader_entry_flags(file_stream *out, const package_reader_entry *entry)
{
    size_t count = 0;

    if ((entry->flags & PACK_TOC_FLAG_FILE) == PACK_TOC_FLAG_FILE)
    {
        putc('F', out->handle);
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

    if (args->out_path != nullptr)
    {
        if (!open(&out, args->out_path, MODE_WRITE, false, false))
            throw_error("could not open ", args->out_path, " for writing");
    }
    else
    {
        out.handle = stdout;
    }

    for (const char *input : args->input_files)
    {
        format(&out, "contents of package %s:\n", input);

        package_reader reader;
        read(&reader, input);

        format(&out, "%u entries found\n", reader.toc->entry_count);
        package_reader_entry entry;

        size_t digits = dec_digits(reader.toc->entry_count);
        char digit_fmt[16] = {0};
        sprintf(digit_fmt, "  %%0%ud ", digits);

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
        puts("");
    }
}

void show_help_and_exit()
{
    puts(PACKER_NAME R"( [-h] [-v] [-x | -g | -l] [-i] [-b <path>] -o <path> <files...>
  v)" PACKER_VERSION R"(
  by )" PACKER_AUTHOR R"(

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
        throw_error("argument '", argv[*i], "' missing parameter");

    *i = *i + 1;
    return argv[*i];
}

void parse_arguments(int argc, char **argv, arguments *args)
{
    for (int i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];

        if (compare_strings(arg, "-h") == 0)
        {
            show_help_and_exit();
            continue;
        }

        if (compare_strings(arg, "-v") == 0)
        {
            args->verbose = true;
            continue;
        }

        if (compare_strings(arg, "-f") == 0)
        {
            args->force = true;
            continue;
        }

        if (compare_strings(arg, "-x") == 0)
        {
            args->extract = true;
            continue;
        }

        if (compare_strings(arg, "-g") == 0)
        {
            args->generate_header = true;
            continue;
        }

        if (compare_strings(arg, "-l") == 0)
        {
            args->list = true;
            continue;
        }

        if (compare_strings(arg, "-i") == 0)
        {
            args->treat_index_as_file = true;
            continue;
        }

        if (compare_strings(arg, "-o") == 0)
        {
            args->out_path = next_arg(argc, argv, &i);
            continue;
        }

        if (compare_strings(arg, "-b") == 0)
        {
            args->base_path = fs::path(next_arg(argc, argv, &i));
            fs::absolute_path(&args->base_path, &args->base_path);
            continue;
        }

        if (compare_strings(arg, "-", 1) == 0)
            throw_error("unexpected argument '", arg, "'");

        args->input_files.push_back(arg);
    }
}

int main(int argc, char **argv)
try
{
    arguments args = default_arguments;
    fs::get_current_path(&args.base_path);
    fs::absolute_path(&args.base_path, &args.base_path);

    parse_arguments(argc, argv, &args);

    if (args.input_files.empty())
        throw_error("no input files");

    if (!is_or_make_directory(&args.base_path))
        throw_error("not a directory: '", args.base_path, "'");

    fs::absolute_canonical_path(&args.base_path, &args.base_path);

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

    puts("done");
    return 0;
}
catch (std::exception &e)
{
    fprintf(stderr, "error: %s\n", e.what());
    return 1;
}
