cmake_minimum_required(VERSION 3.20)
if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ext/better-cmake/cmake/betterConfig.cmake")
    execute_process(COMMAND git submodule update --init "${CMAKE_CURRENT_SOURCE_DIR}/ext/better-cmake" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}") 
endif()
find_package(better REQUIRED PATHS ext/better-cmake/cmake)

project(pack)
project_author("DaemonTsun")

set(TEST_SOURCE_DIR ${ROOT}/tests)
set(TEST_BINARY_DIR ${ROOT}/tests)

set(SOURCE_CMAKE_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/packConfig.cmake")

add_lib(pack STATIC
    VERSION 0.8.1
    SOURCES_DIR "${ROOT}/src/"
    GENERATE_TARGET_HEADER "${ROOT}/src/pack/pack_info.hpp"
    CPP_VERSION 17
    CPP_WARNINGS ALL SANE FATAL
    TESTS "${TEST_SOURCE_DIR}"
    EXT
        LIB shl 0.7.0 "${ROOT}/ext/shl" INCLUDE LINK GIT_SUBMODULE
        LIB fs  0.6.3 "${ROOT}/ext/fs"  INCLUDE LINK GIT_SUBMODULE
    )

add_exe(packer
    VERSION 0.8.1
    SOURCES_DIR "${ROOT}/packer-src"
    GENERATE_TARGET_HEADER "${ROOT}/packer-src/packer_info.hpp"
    CPP_VERSION 17
    CPP_WARNINGS ALL SANE FATAL
    LIBRARIES ${pack_TARGET}
    INCLUDE_DIRS "${pack_SOURCES_DIR}" ${pack_INCLUDE_DIRS}
    )

exit_if_included()

install(FILES "${SOURCE_CMAKE_CONFIG_FILE}" DESTINATION "share/${PROJECT_NAME}/cmake")

# demo
find_program(packer_INSTALLED packer)

if (packer_INSTALLED)
    message(DEBUG "found packer: ${packer_INSTALLED}")
    add_subdirectory(demo)
endif()

find_package(t1 QUIET)
if (t1_FOUND)
    configure_file(${TEST_SOURCE_DIR}/test_file.txt
                   ${TEST_BINARY_DIR}/test_file.txt
                   COPYONLY)

    configure_file(${TEST_SOURCE_DIR}/testpack.pack
                   ${TEST_BINARY_DIR}/testpack.pack
                   COPYONLY)
endif()

