cmake_minimum_required(VERSION 3.20)
if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ext/better-cmake/cmake/betterConfig.cmake")
    execute_process(COMMAND git submodule update --init "${CMAKE_CURRENT_SOURCE_DIR}/ext/better-cmake" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}") 
endif()
find_package(better REQUIRED PATHS ext/better-cmake/cmake)

project(pack)
project_author("DaemonTsun")

set(SOURCE_CMAKE_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/packConfig.cmake")

set(fs_COMPILE_DEFINITIONS @Windows)

add_lib(pack STATIC
    VERSION 0.8.2
    SOURCES_DIR "${ROOT}/src/"
    GENERATE_TARGET_HEADER "${ROOT}/src/pack/pack_info.hpp"
    CPP_VERSION 20
    CPP_WARNINGS ALL SANE FATAL
    COMPILE_DEFINITIONS ${fs_COMPILE_DEFINITIONS}
    TESTS "${ROOT}/tests"
    EXT
        LIB shl 0.10 "${ROOT}/ext/shl" INCLUDE LINK GIT_SUBMODULE
        LIB fs  0.9  "${ROOT}/ext/fs"  INCLUDE LINK GIT_SUBMODULE
    )

add_exe(packer
    VERSION 0.8.2
    SOURCES_DIR "${ROOT}/packer-src"
    GENERATE_TARGET_HEADER "${ROOT}/packer-src/packer_info.hpp"
    CPP_VERSION 20
    CPP_WARNINGS ALL SANE FATAL
        @MSVC /wd4129 /wd4477 # false printf warnings, these DO NOT apply to format
    LIBRARIES ${pack_TARGET}
    COMPILE_DEFINITIONS ${fs_COMPILE_DEFINITIONS}
    INCLUDE_DIRS "${pack_SOURCES_DIR}" ${pack_INCLUDE_DIRS}
    )

exit_if_included()

install(FILES "${SOURCE_CMAKE_CONFIG_FILE}" DESTINATION "share/${PROJECT_NAME}/cmake")

# demo
add_subdirectory(demo)

if (DEFINED Tests)
    copy_files(FILES ${ROOT}/tests/test_file.txt
                     ${ROOT}/tests/testpack.pack
               DESTINATION "${ROOT_BIN}"
               BASE "${ROOT}")
endif()

