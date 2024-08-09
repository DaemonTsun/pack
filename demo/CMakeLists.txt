
find_package(pack REQUIRED NO_DEFAULT_PATH PATHS ../cmake/)

# pack-demo
set(DEMODIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DEMOBIN ${CMAKE_CURRENT_BINARY_DIR})

set(RES "${DEMODIR}/res")
set(SRC "${DEMODIR}/src")

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

add_exe(pack-demo
    VERSION 1.0
    SOURCES_DIR "${DEMODIR}/src"
    INCLUDE_DIRS "${pack_SOURCES_DIR}" "${pack_INCLUDE_DIRS}"
    CPP_VERSION 20
    CPP_WARNINGS ALL SANE FATAL
    LIBRARIES ${pack_TARGET}
    )
