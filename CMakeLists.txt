cmake_minimum_required(VERSION 3.20)
find_package(better REQUIRED PATHS ext/better-cmake/cmake)

project(pack)
project_author("DaemonTsun")

set(TEST_SOURCE_DIR ${ROOT}/tests)
set(TEST_BINARY_DIR ${ROOT}/tests)

set(SOURCE_CMAKE_CONFIG_FILE "${CMAKE_SOURCE_DIR}/cmake/packConfig.cmake")

add_lib(pack STATIC
    VERSION 0.8.1
    SOURCES_DIR "${ROOT}/src/"
    GENERATE_TARGET_HEADER "${ROOT}/src/pack/pack_info.hpp"
    CPP_VERSION 20
    CPP_WARNINGS ALL SANE
    TESTS "${TEST_SOURCE_DIR}"
    EXT
        LIB shl 0.6 "${ROOT}/ext/shl" INCLUDE LINK
        LIB fs  0.6 "${ROOT}/ext/fs"  INCLUDE LINK
    )

add_exe(packer
    VERSION 0.8.1
    SOURCES_DIR "${ROOT}/packer-src"
    GENERATE_TARGET_HEADER "${ROOT}/packer-src/packer_info.hpp"
    CPP_VERSION 20
    CPP_WARNINGS ALL SANE
    LIBRARIES ${pack_TARGET}
    INCLUDE_DIRS "${pack_SOURCES_DIR}" "${shl-0.6.0_SOURCES_DIR}" "${fs-0.6.0_SOURCES_DIR}"
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

