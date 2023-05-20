
# pack-demo
set(DEMODIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DEMOBIN ${CMAKE_CURRENT_BINARY_DIR})

# i cannot put into words how god awful cmake is
list(APPEND CMAKE_MODULE_PATH "${DEMODIR}/cmake")
set(RES ${DEMODIR}/res)
set(SRC ${DEMODIR}/src)
file(GLOB_RECURSE DEMO_HEADERS "${SRC}/*.h" "${SRC}/*.hpp")
file(GLOB_RECURSE DEMO_SOURCES "${SRC}/*.c" "${SRC}/*.cpp")

find_package(pack REQUIRED NO_DEFAULT_PATH PATHS ../cmake/)

file(GLOB_RECURSE PACKAGE_FILES "${RES}/*")
set(TEST_PACK "${DEMOBIN}/testpack.pack")
set(TEST_PACK_H "${SRC}/gen/testpack.h")

pack(OUTPUT_FILES
     BASE "${DEMODIR}"
     COPY_FILE_DESTINATION "${DEMOBIN}"
     GEN_HEADER "${TEST_PACK_H}"
     PACKAGE "${TEST_PACK}"
     FILES ${PACKAGE_FILES})

# if only package is required:
# add_package(OUTPUT_FILES "${TEST_PACK}"
#             BASE ${DEMODIR} 
#             GEN_HEADER "${TEST_PACK_H}"
#             FILES ${PACKAGE_FILES})

# if files copies are required:
# add_files(OUTPUT_FILES "${DEMOBIN}"
#           BASE "${DEMODIR}"
#           GEN_HEADER "${TEST_PACK_H}"
#           PACKAGE "${TEST_PACK}"
#           FILES ${PACKAGE_FILES})

# generate_header("${TEST_PACK_H}"
#                 BASE ${DEMOBIN} 
#                 PACKAGE "${TEST_PACK}"
#                 FILES ${PACKAGE_FILES})

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)

add_executable(pack-demo)
target_sources(pack-demo PUBLIC ${DEMO_HEADERS} ${DEMO_SOURCES} ${OUTPUT_FILES} "${TEST_PACK_H}")
target_include_directories(pack-demo PUBLIC "${pack_SOURCES_DIR}" ${pack_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS})
add_custom_target(run COMMAND "${DEMOBIN}/pack-demo")

# link
target_link_libraries(pack-demo ${pack_TARGET} ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})
