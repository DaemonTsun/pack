
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <filesystem>
#include <stdexcept>
#include <vector>

#include "pack/package.hpp"
#include "pack/package_writer.hpp"
#include "pack/package_reader.hpp"
#include "pack/shl/filesystem.hpp"
#include "pack/shl/string.hpp"

#include "config.hpp"

namespace fs
{
using namespace std::filesystem;
}

struct arguments
{
    bool verbose; // -v

    bool extract; // -x
    bool list; // -l
    const char *out_path; // -o

    fs::path base_path; // -b, defaults to current working directory
    std::vector<const char *> input_files; // anything thats not an arg
};

const arguments default_arguments
{
    .verbose = false,
    .extract = false,
    .list = false,
    .out_path = nullptr,
};

bool is_or_make_directory(fs::path *path)
{
    if (!fs::exists(*path))
    {
        if (!fs::create_directories(*path))
            throw std::runtime_error(str("could not create directory: ", *path));

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

void add_path_files(fs::path *path, std::vector<packer_path> *paths, fs::path *base)
{
    if (!fs::exists(*path))
        throw std::runtime_error(str("can't pack path because path does not exist: ", *path));

    if (fs::is_regular_file(*path))
    {
        paths->push_back(packer_path{*path, path->lexically_relative(*base)});
    }
    else if (fs::is_directory(*path))
        for (const auto &entry : fs::directory_iterator{*path})
        {
            auto epath = actually_absolute(entry.path());
            add_path_files(&epath, paths, base);
        }
}

char y_n_prompt(const char *message)
{
    char c;
    unsigned int ret;

    do
    {
        printf("\n%s", message);

        ret = getchar();

        if (ret == EOF)
            return 'x';

        c = to_lower(static_cast<char>(ret));
    }
    while (c != 'y' && c != 'n');

    return c;
}

void pack(arguments *args)
{
    if (args->out_path == nullptr)
        throw std::runtime_error("no output file specified");

    fs::path outp = fs::absolute(args->out_path);

    if (fs::exists(outp))
    {
        if (!fs::is_regular_file(outp))
            throw std::runtime_error(str("no output file exists and is not a file: ", outp));

        auto msg = str("output file ", outp, " already exists. overwrite? [y / n]: ");
        char choice = y_n_prompt(msg.c_str());

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
        add_path_files(&epath, &paths, &args->base_path);
    }

    if (args->verbose)
    {
        puts("files:\n");

        for (const auto &path : paths)
            printf("  %s -> %s\n", path.input_path.c_str(), path.target_path.c_str());
    }

    package_writer writer;
    
    for (const auto &path : paths)
        add_file(&writer, path.input_path.c_str(), path.target_path.c_str());

    write(&writer, outp.c_str());
    free(&writer);
}

void extract(arguments *args)
{
    /*
    for (const char *path : args->input_files)
        if (!fs::is_regular_file(path))
            throw std::runtime_error(str("not a file: '", path, "'"));

    for (const char *path : args->input_files)
        extract_archive(args, path);
    */
}

#define max(a, b) (a >= b ? a : b)

void print_package_reader_entry_flags(const package_reader_entry *entry)
{
    size_t count = 0;

    if ((entry->flags & PACK_TOC_FLAG_FILE) == PACK_TOC_FLAG_FILE)
    {
        putchar('F');
        count++;
    }

    printf("%.*s", max(8 - count, 0), "        ");
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
    // TODO: output file / stdout
    for (const char *input : args->input_files)
    {
        printf("contents of package %s:\n", input);

        package_reader reader;
        read(&reader, input);

        printf("%u entries found\n", reader.toc->entry_count);
        package_reader_entry entry;

        size_t digits = dec_digits(reader.toc->entry_count);
        char digit_fmt[16] = {0};
        sprintf(digit_fmt, "  %%0%ud ", digits);

        if (args->verbose)
            printf("\n  %.*s flags    offset   size     name\n", digits, "n               ");
        else
            printf("\n  %.*s flags    name\n", digits, "n               ");

        for (int i = 0; i < reader.toc->entry_count; ++i)
        {
            get_package_entry(&reader, i, &entry);
            printf(digit_fmt, i);
            print_package_reader_entry_flags(&entry);
            
            if (args->verbose)
                printf(" %08x %08x", reinterpret_cast<char*>(entry.content) - reader.memory.data, entry.size);

            printf(" %s\n", entry.name);
        }

        free(&reader);
    }
}

void show_help_and_exit()
{
    puts(PACKER_NAME R"( [-h] [-v] [-x | -l] [-b <path>] -o <path> <files...>
  v)" PACKER_VERSION R"(
  by )" PACKER_AUTHOR R"(

packs or extracts archives

ARGUMENTS:
  -h            show this help and exit
  -v            show verbose output
  -x            extract instead of pack
  -l            list the contents of the input files
  -o <path>     the output file / path
  -b <path>     specifies the base path, all file paths will be relative to it.
                only used in packing, not extracting.

  <files>       the input files
)");

    exit(0);
}

const char *next_arg(int argc, char **argv, int *i)
{
    if (*i >= argc - 1)
        throw std::runtime_error(str("argument '", argv[*i], "' missing parameter"));

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

        if (strcmp(arg, "-x") == 0)
        {
            args->extract = true;
            continue;
        }

        if (strcmp(arg, "-l") == 0)
        {
            args->list = true;
            continue;
        }

        if (strcmp(arg, "-o") == 0)
        {
            args->out_path = next_arg(argc, argv, &i);
            continue;
        }

        if (strcmp(arg, "-b") == 0)
        {
            args->base_path = actually_absolute(next_arg(argc, argv, &i));
            continue;
        }

        if (strncmp(arg, "-", 1) == 0)
            throw std::runtime_error(str("unexpected argument '", arg, "'"));

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
        throw std::runtime_error("no input files");

    if (!is_or_make_directory(&args.base_path))
        throw std::runtime_error(str("not a directory: '", args.base_path, "'"));

    if (args.list && args.extract)
        throw std::runtime_error("cannot extract and list");

    if (args.list)
        list(&args);
    else if (args.extract)
        extract(&args);
    else
        pack(&args);

    return 0;
}
catch (std::exception &e)
{
    fprintf(stderr, "error: %s\n", e.what());
}