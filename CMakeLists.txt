cmake_minimum_required(VERSION 3.20)
find_package(better REQUIRED NO_DEFAULT_PATH PATHS ext/better-cmake/cmake)

project(pack VERSION 0.8)
get_version_target(pack_TARGET ${PROJECT_NAME})

if(TARGET ${pack_TARGET})
    return()
endif()

set(pack_SOURCES_DIR "${CMAKE_SOURCE_DIR}/src")
set(packer_SOURCES_DIR "${CMAKE_SOURCE_DIR}/packer-src")

# sources
find_sources(pack_SOURCES "${pack_SOURCES_DIR}")
find_headers(pack_HEADERS "${pack_SOURCES_DIR}")
find_sources(packer_SOURCES "${packer_SOURCES_DIR}")
find_headers(packer_HEADERS "${packer_SOURCES_DIR}")
set(SOURCE_CMAKE_CONFIG_FILE "${CMAKE_SOURCE_DIR}/cmake/packConfig.cmake")

# settings
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(ext/shl)
add_subdirectory(ext/fs)

# target lib
add_library(${pack_TARGET} SHARED)
target_sources(${pack_TARGET} PUBLIC ${pack_HEADERS} ${pack_SOURCES})
set_property(TARGET ${pack_TARGET} PROPERTY POSITION_INDEPENDENT_CODE ON)

# includes
target_include_directories(${pack_TARGET} PUBLIC "${pack_SOURCES_DIR}" "${shl_SOURCES_DIR}" "${fs_SOURCES_DIR}")
target_link_libraries(${pack_TARGET} ${shl_TARGET} ${fs_TARGET})

# packer
add_executable(packer)
target_sources(packer PRIVATE ${packer_HEADERS} ${packer_SOURCES})
target_link_libraries(packer ${pack_TARGET})
target_include_directories(packer PRIVATE "${pack_SOURCES_DIR}" "${packer_SOURCES_DIR}" "${shl_SOURCES_DIR}" "${fs_SOURCES_DIR}")

# demo
find_program(packer_INSTALLED packer)

if (packer_INSTALLED)
    message(DEBUG "found packer: ${packer_INSTALLED}")
    add_subdirectory(demo)
endif()

# tests
set(TEST_SOURCES_DIR "${CMAKE_SOURCE_DIR}/tests")
set(TEST_BIN_DIR "${CMAKE_BINARY_DIR}/tests")

find_package(t1 QUIET)
if(NOT DEFINED t1_DIR OR t1_DIR)
    add_test_directory(${TEST_SOURCES_DIR} INCLUDE_DIRS "${pack_SOURCES_DIR}" LIBRARIES ${pack_TARGET})
    register_tests()

    configure_file(${TEST_SOURCES_DIR}/test_file.txt
                   ${TEST_BIN_DIR}/test_file.txt
                   COPYONLY)

    configure_file(${TEST_SOURCES_DIR}/testpack.pack
                   ${TEST_BIN_DIR}/testpack.pack
                   COPYONLY)
endif()

# install
install_library(TARGET ${pack_TARGET} HEADERS ${pack_HEADERS})
install_executable(TARGET packer)
