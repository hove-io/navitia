if(${ENABLE_CLANG_TIDY})

    message("Enable clang-tidy")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    set(CLANG_TIDY_BINARY_NAME clang-tidy-6.0)
    set(RUN_CLANG_TIDY_BINARY_NAME run-clang-tidy-6.0.py)
    find_program(CLANG_TIDY_BIN ${CLANG_TIDY_BINARY_NAME})
    find_program(RUN_CLANG_TIDY_BIN ${RUN_CLANG_TIDY_BINARY_NAME})

    if(CLANG_TIDY_BIN STREQUAL "CLANG_TIDY_BIN-NOTFOUND")
        message(FATAL_ERROR "unable to locate ${CLANG_TIDY_BINARY_NAME}")
    endif()

    if(RUN_CLANG_TIDY_BIN STREQUAL "RUN_CLANG_TIDY_BIN-NOTFOUND")
        message(FATAL_ERROR "unable to locate ${RUN_CLANG_TIDY_BINARY_NAME}")
    endif()

    # Get all .cpp files in navitia/source/ and remove everything
    # From /tests (including unit tests) and /third_party
    file(GLOB_RECURSE FILES_TO_SCAN "${CMAKE_SOURCE_DIR}/*.cpp")
    list(FILTER FILES_TO_SCAN EXCLUDE REGEX "(tests|third_party|utils)")

    list(APPEND RUN_CLANG_TIDY_BIN_ARGS
        -clang-tidy-binary ${CLANG_TIDY_BIN}
        "${FILES_TO_SCAN}"
    )

    add_custom_target(
        tidy
        COMMAND ${RUN_CLANG_TIDY_BIN} ${RUN_CLANG_TIDY_BIN_ARGS}
        COMMENT "running clang tidy"
    )

    add_custom_target(
        tidy_fix
        COMMAND ${RUN_CLANG_TIDY_BIN} ${RUN_CLANG_TIDY_BIN_ARGS} -fix
        COMMENT "running clang tidy -fix"
    )

endif()
