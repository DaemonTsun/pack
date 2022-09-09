
// v1.0
#pragma once

#include <limits.h>
#include <string>

// out pointer must point to memory at least MAX_PATH / PATH_MAX
// in size.
long get_executable_path(char *out);
std::string get_executable_path();
