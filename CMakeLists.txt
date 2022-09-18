cmake_minimum_required(VERSION 3.20)

if(CMAKE_BUILD_TYPE)
    if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        # the fact that this isnt default behavior is baffling
        set(CMAKE_MESSAGE_LOG_LEVEL DEBUG)
    endif()
endif()

project(pack)

set(PACK_SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(PACKER_SRC_DIR "${CMAKE_SOURCE_DIR}/packer-src")
set(TEST_SRC_DIR "${CMAKE_SOURCE_DIR}/tests")
set(TEST_BIN_DIR "${CMAKE_BINARY_DIR}/tests")

# sources
file(GLOB_RECURSE PACK_HEADERS "${PACK_SRC_DIR}/*.h" "${PACK_SRC_DIR}/*.hpp")
file(GLOB_RECURSE PACK_SOURCES "${PACK_SRC_DIR}/*.c" "${PACK_SRC_DIR}/*.cpp")
file(GLOB_RECURSE PACKER_HEADERS "${PACKER_SRC_DIR}/*.h" "${PACKER_SRC_DIR}/*.hpp")
file(GLOB_RECURSE PACKER_SOURCES "${PACKER_SRC_DIR}/*.c" "${PACKER_SRC_DIR}/*.cpp")
set(SOURCE_CMAKE_CONFIG_FILE "${CMAKE_SOURCE_DIR}/cmake/packConfig.cmake")

# settings
set(CMAKE_CXX_STANDARD 17)

# target lib
add_library(pack SHARED)
target_sources(pack PUBLIC ${PACK_HEADERS} ${PACK_SOURCES})
set_property(TARGET pack PROPERTY POSITION_INDEPENDENT_CODE ON)

# includes
target_include_directories(pack PUBLIC "${PACK_SRC_DIR}")

# packer
add_executable(packer)
target_sources(packer PRIVATE ${PACKER_HEADERS} ${PACKER_SOURCES})
target_link_libraries(packer pack)
target_include_directories(packer PRIVATE "${PACK_SRC_DIR}" "${PACKER_SRC_DIR}")

# demo
find_program(PACKER_INSTALLED packer)

if (PACKER_INSTALLED)
    message(DEBUG "found packer: ${PACKER_INSTALLED}")
    add_subdirectory(demo)
endif()

# tests
find_package(t1 QUIET)
if(NOT DEFINED t1_DIR OR t1_DIR)
    add_test_directory(${TEST_SRC_DIR})
    register_tests()
    link_tests(pack)

    configure_file(${TEST_SRC_DIR}/test_file.txt
                   ${TEST_BIN_DIR}/test_file.txt
                   COPYONLY)
endif()

# install
install(FILES ${PACK_HEADERS} DESTINATION "include/${PROJECT_NAME}")
install(FILES "${SOURCE_CMAKE_CONFIG_FILE}" DESTINATION "share/${PROJECT_NAME}/cmake")
install(TARGETS "${PROJECT_NAME}"
        RUNTIME DESTINATION "bin"
        LIBRARY DESTINATION "lib"
        ARCHIVE DESTINATION "lib/${PROJECT_NAME}")
install(TARGETS packer DESTINATION bin)
