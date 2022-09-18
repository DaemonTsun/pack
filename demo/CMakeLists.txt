
# pack-demo
set(DEMODIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DEMOBIN ${CMAKE_CURRENT_BINARY_DIR})
set(RES ${DEMODIR}/res)
set(SRC ${DEMODIR}/src)
file(GLOB_RECURSE DEMO_HEADERS "${SRC}/*.h" "${SRC}/*.hpp")
file(GLOB_RECURSE DEMO_SOURCES "${SRC}/*.c" "${SRC}/*.cpp")

find_package(pack REQUIRED NO_DEFAULT_PATH PATHS ../cmake/)

file(GLOB_RECURSE PACKAGE_FILES "${RES}/*")
set(TEST_PACK "${DEMOBIN}/testpack.pack")
add_package("${TEST_PACK}"
            BASE ${DEMODIR} 
            GEN_HEADER "${SRC}/gen/testpack.h"
            FILES ${PACKAGE_FILES})

add_executable(pack-demo)
target_sources(pack-demo PUBLIC ${DEMO_HEADERS} ${DEMO_SOURCES} "${TEST_PACK}")
target_include_directories(pack-demo PUBLIC "${PACK_SRC_DIR}")

# link
target_link_libraries(pack-demo ${PROJECT_NAME})
