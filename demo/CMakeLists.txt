
# pack-demo
set(DEMODIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DEMOBIN ${CMAKE_CURRENT_BINARY_DIR})
set(RES ${DEMODIR}/res)
set(SRC ${DEMODIR}/src)
file(GLOB_RECURSE DEMO_HEADERS "${SRC}/*.h" "${SRC}/*.hpp")
file(GLOB_RECURSE DEMO_SOURCES "${SRC}/*.c" "${SRC}/*.cpp")
add_executable(pack-demo)

find_package(pack REQUIRED PATHS ../cmake/)

file(GLOB_RECURSE PACKAGE_FILES "${RES}/*")
add_package(testpack ${DEMOBIN}/testpack.pack BASE ${RES} FILES ${PACKAGE_FILES})

target_sources(pack-demo PUBLIC ${DEMO_HEADERS} ${DEMO_SOURCES})
target_include_directories(pack-demo PUBLIC "${PACK_SRC_DIR}")

# link
target_link_libraries(pack-demo ${PROJECT_NAME})
