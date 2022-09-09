
#pragma once

#include "pack/shl/number_types.hpp"

#define PACK_HEADER_MAGIC "pack"
#define PACK_TOC_MAGIC "toc0"

/* pack structure:
 * [header
 *   4 bytes magic "pack"
 *   4 bytes version
 *   8 bytes flags
 *   8 bytes toc offset position
 *   8 bytes entry name table offset position
 *   8 bytes entry name table size
 * ]
 * [entries]
 * [name table (aligned at 8 bytes)
 *   arbitrary length names seperated by \0
 * ]
 * [table of contents (aligned at 8 bytes)
 *   4 bytes toc magic "toc0"
 *   4 bytes padding
 *   8 bytes number of toc entries
 * ]
 * [toc entries
 *   [entry 1
 *     8 bytes content offset
 *     8 bytes content size
 *     8 bytes name offset
 *   ]
 *   [entry 2 ...]
 * ]
 */

#define PACK_VERSION  0x00000001
#define PACK_NO_FLAGS 0

struct package_header
{
    char magic[4];
    u32 version;
    u64 flags;
    u64 toc_offset;
    u64 names_offset;
    u64 names_size;
};

struct package_toc
{
    char magic[4];
    u32 _padding;
    u64 entry_count;
};

#define PACK_TOC_NO_FLAGS  0x00
#define PACK_TOC_FLAG_FILE 0x01

struct package_toc_entry
{
    u64 offset;
    u64 size;
    u64 name_offset;
    u64 flags;
};
