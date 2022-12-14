
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <filesystem>
#include <stdexcept>
#include <vector>

#include "shl/filesystem.hpp"
#include "shl/file_stream.hpp"
#include "shl/string.hpp"
#include "pack/package.hpp"
#include "pack/package_writer.hpp"
#include "pack/package_reader.hpp"

#include "config.hpp"

#define PACK_INDEX_EXTENSION "_index"

#define throw_error(...)\
    throw std::runtime_error(str(__FILE__, ":", __LINE__, ' ', __VA_ARGS__));

namespace fs
{
using namespace std::filesystem;
}

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
    if (!fs::exists(*path))
    {
        if (!fs::create_directories(*path))
            throw_error("could not create directory: ", *path);

        return true;
    }
    else
    {
        if (!fs::is_directory(*path))
            return false;

        return true;
    }
}

fs::path actually_absolute(fs::path path)
{
    return fs::absolute(fs::canonical(path));
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

    if (!fs::exists(*path))
        throw_error("can't pack path because path does not exist: ", *path);

    if (fs::is_regular_file(*path))
    {
        if (args->treat_index_as_file)
        {
            paths->push_back(packer_path{*path, path->lexically_relative(args->base_path)});
            return;
        }

        // check for index file, then get files from that
        std::string ext = path->extension();

        if (!ends_with(ext, str(PACK_INDEX_EXTENSION)))
        {
            paths->push_back(packer_path{*path, path->lexically_relative(args->base_path)});
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

            auto strline = std::string(line, strlen(line) - 1);

            if (is_blank(strline))
                continue;

            if (begins_with(strline, str("##")))
                continue;

            index_paths.push_back(actually_absolute(strline));
        }

        close(&findex);

        if (line != nullptr)
            free(line);

        for (auto &ipath : index_paths)
            add_path_files(&ipath, paths, args);
    }
    else if (fs::is_directory(*path))
        for (const auto &entry : fs::directory_iterator{*path})
        {
            auto epath = actually_absolute(entry.path());
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

        if (is_space(ret))
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

    fs::path outp = fs::absolute(args->out_path);

    if (fs::exists(outp))
    {
        if (!fs::is_regular_file(outp))
            throw_error("output file exists but is not a file: ", outp);

        auto msg = str("output file ", outp, " already exists. overwrite? [y / n]: ");
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
        auto epath = actually_absolute(path);
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

    if (fs::exists(opath))
    {
        if (!fs::is_regular_file(opath))
            throw_error("not a writable file: ", opath);

        std::string msg = str("generated header file ", opath, " exists, overwrite? [y / n]: ");
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
        fs::path path = actually_absolute(path_);

        if (!fs::is_regular_file(path))
            throw_error("not a file: ", path);

        fs::path rel = path.lexically_relative(args->base_path);

        if (args->verbose)
            printf("reading toc of archive %s\n", path.c_str());

        std::string var_prefix = sanitize_name(rel);
        read(&reader, rel.c_str());

        format(&stream, "\n#define %s \"%s\"\n", var_prefix.c_str(), rel.c_str());
        format(&stream, "#define %s_file_count %u\n", var_prefix.c_str(), reader.toc->entry_count);
        format(&stream, "static const char *%s_files[] = {\n", var_prefix.c_str());

        size_t maxnamelen = 0;

        for (u64 i = 0; i < reader.toc->entry_count; ++i)
        {
            get_package_entry(&reader, i, &entry);
            format(&stream, "    \"%s\",\n", entry.name);

            auto len = strlen(entry.name);

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

        fs::path epath = outp / entry.name;
        fs::path parent = epath.parent_path();

        if (!fs::exists(parent))
        {
            if (!fs::create_directories(parent))
                throw_error("  could not create parent directory ", parent, " for file entry ", epath);
        }

        if (fs::exists(epath))
        {
            if (never_overwrite)
            {
                printf("skipping existing file %s\n", epath.c_str());
                continue;
            }

            if (!fs::is_regular_file(epath))
                throw_error("entry output path exists but is not a file: ", epath);

            if (!always_overwrite)
            {
                auto msg = str("output file ", epath, " already exists. overwrite? [y / n / (a)lways overwrite / n(e)ver overwrite]: ");
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
        if (!fs::is_regular_file(path))
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

        if (strcmp(arg, "-h") == 0)
        {
            show_help_and_exit();
            continue;
        }

        if (strcmp(arg, "-v") == 0)
        {
            args->verbose = true;
            continue;
        }

        if (strcmp(arg, "-f") == 0)
        {
            args->force = true;
            continue;
        }

        if (strcmp(arg, "-x") == 0)
        {
            args->extract = true;
            continue;
        }

        if (strcmp(arg, "-g") == 0)
        {
            args->generate_header = true;
            continue;
        }

        if (strcmp(arg, "-l") == 0)
        {
            args->list = true;
            continue;
        }

        if (strcmp(arg, "-i") == 0)
        {
            args->treat_index_as_file = true;
            continue;
        }

        if (strcmp(arg, "-o") == 0)
        {
            args->out_path = next_arg(argc, argv, &i);
            continue;
        }

        if (strcmp(arg, "-b") == 0)
        {
            args->base_path = fs::absolute(next_arg(argc, argv, &i));
            continue;
        }

        if (strncmp(arg, "-", 1) == 0)
            throw_error("unexpected argument '", arg, "'");

        args->input_files.push_back(arg);
    }
}

int main(int argc, char **argv)
try
{
    arguments args = default_arguments;
    args.base_path = fs::absolute(fs::current_path());
    parse_arguments(argc, argv, &args);

    if (args.input_files.empty())
        throw_error("no input files");

    if (!is_or_make_directory(&args.base_path))
        throw_error("not a directory: '", args.base_path, "'");

    args.base_path = actually_absolute(args.base_path);

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
