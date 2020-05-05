
message(STATUS "cland-tools")

# Generates a compile_commands.json file containing the exact compiler calls for all translation units
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CLANG_TIDY_BINARY_NAME
    NAMES clang-tidy clang-tidy-6.0)
find_program(CLANG_TIDY_BIN ${CLANG_TIDY_BINARY_NAME})

set(RUN_CLANG_TIDY_BINARY_NAME
    NAMES run-clang-tidy.py run-clang-tidy-6.0.py)
find_program(RUN_CLANG_TIDY_BIN ${RUN_CLANG_TIDY_BINARY_NAME})

if( NOT CLANG_TIDY_BIN STREQUAL "CLANG_TIDY_BIN-NOTFOUND"
    AND NOT RUN_CLANG_TIDY_BIN STREQUAL "RUN_CLANG_TIDY_BIN-NOTFOUND")
    # Get all .cpp files in navitia/source/ and remove everything
    # From utils and /third_party
    file(GLOB_RECURSE FILES_TO_SCAN "${CMAKE_SOURCE_DIR}/*.cpp")
    list(FILTER FILES_TO_SCAN EXCLUDE REGEX "(third_party|utils)")
    list(APPEND RUN_CLANG_TIDY_BIN_ARGS -clang-tidy-binary ${CLANG_TIDY_BIN} "${FILES_TO_SCAN}")

    add_custom_target(
        tidy
        COMMAND ${RUN_CLANG_TIDY_BIN} ${RUN_CLANG_TIDY_BIN_ARGS}
        COMMENT "running clang tidy -- ${CMAKE_SOURCE_DIR}"
        DEPENDS protobuf_files
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    add_custom_target(
        tidy_fix
        COMMAND ${RUN_CLANG_TIDY_BIN} -fix ${RUN_CLANG_TIDY_BIN_ARGS}
        COMMENT "running clang tidy -fix -- ${CMAKE_SOURCE_DIR}"
        DEPENDS protobuf_files
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    message(STATUS "found")
endif()

unset(missing_bin)
if(CLANG_TIDY_BIN STREQUAL "CLANG_TIDY_BIN-NOTFOUND")
    list(APPEND missing_bin "clang-tidy")
endif()

if(RUN_CLANG_TIDY_BIN STREQUAL "RUN_CLANG_TIDY_BIN-NOTFOUND")
    list(APPEND missing_bin "clang-tools")
endif()

if(missing_bin)
    message(WARNING "couldn't find packages : ${missing_bin}")
endif()
