cmake_minimum_required(VERSION 3.18)

project(pack)

set(PACK_SRC_DIR "src/pack")

# sources
file(GLOB_RECURSE HEADERS "${PACK_SRC_DIR}/*.h" "${PACK_SRC_DIR}/*.hpp")
file(GLOB_RECURSE SOURCES "${PACK_SRC_DIR}/*.c" "${PACK_SRC_DIR}/*.cpp")

# settings
set(CMAKE_CXX_STANDARD 17)

# target lib
add_library(${PROJECT_NAME} SHARED ${HEADERS} ${SOURCES})
set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

# includes
target_include_directories(${PROJECT_NAME} PUBLIC "${PACK_SRC_DIR}")

