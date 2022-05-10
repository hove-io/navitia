
message(STATUS "clang-tools")

# Generates a compile_commands.json file containing the exact compiler calls for all translation units
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(COMPILE_COMMANDS_LINK "${CMAKE_SOURCE_DIR}/compile_commands.json")
set(COMPILE_COMMANDS_SRC "${CMAKE_BINARY_DIR}/compile_commands.json")
add_custom_command(
    OUTPUT ${COMPILE_COMMANDS_LINK}
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${COMPILE_COMMANDS_SRC} ${COMPILE_COMMANDS_LINK}
    COMMENT "Symlink clang compile commands file into source folder"
)
add_custom_target(
    NAME symlink_compile_command
    COMMENT "Symlink ${COMPILE_COMMANDS_LINK}"
    DEPENDS ${COMPILE_COMMANDS_LINK}
)

set(CLANG_TIDY_BINARY_NAME
    NAMES clang-tidy clang-tidy-11)
find_program(CLANG_TIDY_BIN ${CLANG_TIDY_BINARY_NAME})

set(RUN_CLANG_TIDY_BINARY_NAME
    NAMES run-clang-tidy run-clang-tidy.py run-clang-tidy-11.py)
find_program(RUN_CLANG_TIDY_BIN ${RUN_CLANG_TIDY_BINARY_NAME})

if( NOT CLANG_TIDY_BIN STREQUAL "CLANG_TIDY_BIN-NOTFOUND"
    AND NOT RUN_CLANG_TIDY_BIN STREQUAL "RUN_CLANG_TIDY_BIN-NOTFOUND")
    # Get all .cpp files in navitia/source/ and remove everything
    # From utils and /third_party
    file(GLOB_RECURSE FILES_TO_SCAN "${CMAKE_SOURCE_DIR}/*.cpp" "${CMAKE_SOURCE_DIR}/*.h" "${CMAKE_SOURCE_DIR}/*.hpp")
    list(FILTER FILES_TO_SCAN EXCLUDE REGEX "(third_party)")
    list(APPEND RUN_CLANG_TIDY_BIN_ARGS -header-filter=.* -clang-tidy-binary ${CLANG_TIDY_BIN} "${FILES_TO_SCAN}")

    add_custom_target(
        tidy
        COMMAND ${RUN_CLANG_TIDY_BIN} ${RUN_CLANG_TIDY_BIN_ARGS}
        COMMENT "running clang tidy -- ${CMAKE_SOURCE_DIR}"
        DEPENDS protobuf_files ${COMPILE_COMMANDS_LINK}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    add_custom_target(
        tidy_fix
        COMMAND ${RUN_CLANG_TIDY_BIN} -format -fix ${RUN_CLANG_TIDY_BIN_ARGS}
        COMMENT "running clang tidy -fix -- ${CMAKE_SOURCE_DIR}"
        DEPENDS protobuf_files ${COMPILE_COMMANDS_LINK}
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
