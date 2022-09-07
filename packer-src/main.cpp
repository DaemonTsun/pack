
#include <iostream>
#include "pack/package_writer.hpp"
#include "pack/filesystem.hpp"


int main(int argc, char **argv)
{
    std::string rtpath = get_executable_path();
    rtpath += "_out";

    package_writer writer;
    
    add_entry(&writer, 8u, "number 8");
    add_entry(&writer, "hello world", "the hello");

    write(&writer, rtpath.c_str());

    return 0;
}
