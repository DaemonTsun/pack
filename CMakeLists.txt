cmake_minimum_required(VERSION 3.18)

project(pack)

set(PACK_SRC_DIR "src")
set(PACKER_SRC_DIR "packer-src")

# sources
file(GLOB_RECURSE PACK_HEADERS "${PACK_SRC_DIR}/*.h" "${PACK_SRC_DIR}/*.hpp")
file(GLOB_RECURSE PACK_SOURCES "${PACK_SRC_DIR}/*.c" "${PACK_SRC_DIR}/*.cpp")
file(GLOB_RECURSE PACKER_HEADERS "${PACKER_SRC_DIR}/*.h" "${PACKER_SRC_DIR}/*.hpp")
file(GLOB_RECURSE PACKER_SOURCES "${PACKER_SRC_DIR}/*.c" "${PACKER_SRC_DIR}/*.cpp")

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

