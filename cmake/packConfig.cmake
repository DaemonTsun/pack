
# pack cmake package
# defines functions for integrating pack into cmake projects

macro(add_package OUT_PATH)
    find_program(PACKER_EXEC packer)

    if (NOT PACKER_EXEC)
        message(FATAL_ERROR "could not find packer executable, install packer first.")
    endif()

    set(_OPTIONS)
    # TODO: add single val arg for inc file
    set(_SINGLE_VAL_ARGS BASE)
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
        DEPENDS "${ADD_PACKAGE_FILES}" "${_INDEX_FILE}"
    )
endmacro()
