
# pack cmake package
# defines functions for integrating pack into cmake projects

# used internally
macro(sanitize_path NAME_VAR PATH)
    string(REGEX REPLACE "[/.]" "_" ${NAME_VAR} ${PATH})
    string(REGEX REPLACE "__+" "_" ${NAME_VAR} ${${NAME_VAR}})
    string(REGEX REPLACE "^_+" "" ${NAME_VAR} ${${NAME_VAR}})
endmacro()

# used internally
macro(generate_header OUT_PATH)
    set(_OPTIONS)
    set(_SINGLE_VAL_ARGS BASE PACKAGE)
    set(_MULTI_VAL_ARGS FILES)

    cmake_parse_arguments(GENERATE_HEADER "${_OPTIONS}" "${_SINGLE_VAL_ARGS}" "${_MULTI_VAL_ARGS}" ${ARGN})

    if (NOT DEFINED GENERATE_HEADER_FILES)
        message(FATAL_ERROR "generate_header: missing FILES")
    endif()

    if (NOT DEFINED GENERATE_HEADER_PACKAGE)
        message(FATAL_ERROR "generate_header: missing PACKAGE path, required to generate header file (package does not need to exist)")
    endif()

    if (NOT DEFINED GENERATE_HEADER_BASE)
        message(FATAL_ERROR "generate_header: missing BASE path")
    endif()

    cmake_path(RELATIVE_PATH GENERATE_HEADER_PACKAGE BASE_DIRECTORY "${GENERATE_HEADER_BASE}" OUTPUT_VARIABLE REL_PACKAGE)

    sanitize_path(SAFE_REL_PACKAGE "${REL_PACKAGE}")

    set(_HEADER "// this file was generated by CMake pack\n\n#pragma once\n")
    set(_HEADER "${_HEADER}\n#define ${SAFE_REL_PACKAGE} \"${REL_PACKAGE}\"\n")

    list(LENGTH GENERATE_HEADER_FILES ENTRY_COUNT)

    set(_HEADER "${_HEADER}#define ${SAFE_REL_PACKAGE}_file_count ${ENTRY_COUNT}\n")
    set(_HEADER "${_HEADER}static const char *${SAFE_REL_PACKAGE}_files[] = {\n")

    set(_HEADER_DEFS "")

    set(_I 0)
    foreach(_FILE ${GENERATE_HEADER_FILES})
        cmake_path(RELATIVE_PATH _FILE BASE_DIRECTORY "${GENERATE_HEADER_BASE}" OUTPUT_VARIABLE REL_FILE)
        sanitize_path(SAFE_FILE "${REL_FILE}")
        set(_HEADER "${_HEADER}    \"${REL_FILE}\",\n")
        set(_HEADER_DEFS "${_HEADER_DEFS}#define ${SAFE_REL_PACKAGE}__${SAFE_FILE} ${_I}\n")
        math(EXPR _I "${_I}+1")
    endforeach()

    set(_HEADER "${_HEADER}};\n\n${_HEADER_DEFS}\n")

    file(WRITE "${OUT_PATH}" "${_HEADER}")

    unset(_HEADER)
endmacro()

# TODO: docs
macro(add_files OUT_FILES_VAR OUT_PATH)
    set(_OPTIONS)
    set(_SINGLE_VAL_ARGS BASE PACKAGE GEN_HEADER)
    set(_MULTI_VAL_ARGS FILES)

    cmake_parse_arguments(ADD_FILES "${_OPTIONS}" "${_SINGLE_VAL_ARGS}" "${_MULTI_VAL_ARGS}" ${ARGN})

    if (NOT DEFINED ADD_FILES_FILES)
        message(FATAL_ERROR "add_files: missing FILES")
    endif()

    if (NOT DEFINED ADD_FILES_BASE)
        message(FATAL_ERROR "add_files: missing BASE path")
    endif()

    foreach(_FILE ${ADD_FILES_FILES})
        cmake_path(RELATIVE_PATH _FILE BASE_DIRECTORY "${ADD_FILES_BASE}" OUTPUT_VARIABLE _REL_FILE)
        set(_OUT_FILE "${OUT_PATH}/${_REL_FILE}")

        cmake_path(GET _OUT_FILE PARENT_PATH _PARENT)
        file(MAKE_DIRECTORY "${_PARENT}")

        message(DEBUG "adding target to copy ${_FILE} to ${_OUT_FILE}")

        add_custom_command(
            OUTPUT "${_OUT_FILE}"
            COMMAND ${CMAKE_COMMAND} -E copy "${_FILE}" "${_OUT_FILE}"
            DEPENDS "${_FILE}")

        list(APPEND ${OUT_FILES_VAR} "${_OUT_FILE}")
    endforeach()

    if (DEFINED ADD_FILES_GEN_HEADER)
        if (NOT DEFINED ADD_FILES_PACKAGE)
            message(FATAL_ERROR "add_files: missing PACKAGE path, required to generate header file (package does not need to exist)")
        endif()

        generate_header(
            "${ADD_FILES_GEN_HEADER}"
            BASE "${OUT_PATH}"
            PACKAGE "${ADD_FILES_PACKAGE}"
            FILES ${${OUT_FILES_VAR}})
    endif()
endmacro()

# TODO: docs
macro(add_package OUT_FILES_VAR OUT_PATH)
    find_program(PACKER_EXEC packer)

    if (NOT PACKER_EXEC)
        message(FATAL_ERROR "could not find packer executable, install packer first.")
    endif()

    set(_OPTIONS)
    set(_SINGLE_VAL_ARGS BASE GEN_HEADER)
    set(_MULTI_VAL_ARGS FILES)

    cmake_parse_arguments(ADD_PACKAGE "${_OPTIONS}" "${_SINGLE_VAL_ARGS}" "${_MULTI_VAL_ARGS}" ${ARGN})

    if (NOT DEFINED ADD_PACKAGE_FILES)
        message(FATAL_ERROR "add_package: missing FILES")
    endif()

    if (NOT DEFINED ADD_PACKAGE_BASE)
        message(FATAL_ERROR "add_package: missing BASE path")
    endif()

    message(DEBUG " pack: package ${OUT_PATH}")
    message(DEBUG "       base path ${ADD_PACKAGE_BASE}")
    message(DEBUG "  files:")

    set(_INDEX "## this file was generated by CMake pack\n\n")

    foreach(_FILE ${ADD_PACKAGE_FILES})
        message(DEBUG "  ${_FILE}")
        set(_INDEX "${_INDEX}${_FILE}\n")
    endforeach()

    set(_INDEX_FILE "${OUT_PATH}_index")
    file(WRITE "${_INDEX_FILE}" "${_INDEX}")

    message(DEBUG "  command:\n" "${PACKER_EXEC} -f -b ${ADD_PACKAGE_BASE} -o ${OUT_PATH} ${_INDEX_FILE}")

    add_custom_command(
        OUTPUT "${OUT_PATH}"
        COMMAND "${PACKER_EXEC}" "-f" "-b" "${ADD_PACKAGE_BASE}" "-o" "${OUT_PATH}" "${_INDEX_FILE}"
        MAIN_DEPENDENCY "${_INDEX_FILE}"
        DEPENDS "${ADD_PACKAGE_FILES}" "${_INDEX_FILE}")

    list(APPEND ${OUT_FILES_VAR} "${OUT_PATH}")

    if (DEFINED ADD_PACKAGE_GEN_HEADER)
        add_custom_command(
            OUTPUT "${ADD_PACKAGE_GEN_HEADER}"
            COMMAND "${PACKER_EXEC}" "-f" "-g" "-o" "${ADD_PACKAGE_GEN_HEADER}" "${OUT_PATH}"
            MAIN_DEPENDENCY "${OUT_PATH}"
            DEPENDS "${OUT_PATH}")
    endif()

    unset(_INDEX)
    unset(_INDEX_FILE)
endmacro()

# TODO: docs
macro(pack OUT_FILES_VAR)
    set(_OPTIONS COPY_FILES)
    set(_SINGLE_VAL_ARGS COPY_FILE_DESTINATION BASE PACKAGE GEN_HEADER)
    set(_MULTI_VAL_ARGS FILES)

    cmake_parse_arguments(_PACK "${_OPTIONS}" "${_SINGLE_VAL_ARGS}" "${_MULTI_VAL_ARGS}" ${ARGN})

    if (NOT DEFINED _PACK_FILES)
        message(FATAL_ERROR "pack: missing FILES")
    endif()

    if (NOT DEFINED _PACK_BASE)
        message(FATAL_ERROR "pack: missing BASE path")
    endif()

    if (NOT DEFINED _PACK_PACKAGE)
        message(FATAL_ERROR "pack: missing PACKAGE path")
    endif()

    if (_PACK_COPY_FILES OR "${CMAKE_BUILD_TYPE}" STREQUAL Debug)
        message(STATUS "pack: copying files")

        if (NOT DEFINED _PACK_COPY_FILE_DESTINATION)
            message(FATAL_ERROR "pack: missing COPY_FILE_DESTINATION path for copying files")
        endif()

        if (DEFINED _PACK_GEN_HEADER)
            add_files(${OUT_FILES_VAR} "${_PACK_COPY_FILE_DESTINATION}" BASE "${_PACK_BASE}" PACKAGE "${_PACK_PACKAGE}" GEN_HEADER "${_PACK_GEN_HEADER}" FILES ${_PACK_FILES})
        else()
            add_files(${OUT_FILES_VAR} "${_PACK_COPY_FILE_DESTINATION}" BASE "${_PACK_BASE}" "${_PACK_GEN_HEADER}" FILES ${_PACK_FILES})
        endif()
    else()
        message(STATUS "pack: packing files")

        if (DEFINED _PACK_GEN_HEADER)
            add_package(${OUT_FILES_VAR} "${_PACK_PACKAGE}" BASE "${_PACK_BASE}" GEN_HEADER "${_PACK_GEN_HEADER}" FILES ${_PACK_FILES})
        else()
            add_package(${OUT_FILES_VAR} "${_PACK_PACKAGE}" BASE "${_PACK_BASE}" FILES ${_PACK_FILES})
        endif()
    endif()
endmacro()
