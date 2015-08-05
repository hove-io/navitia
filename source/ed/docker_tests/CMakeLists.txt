
SET(DATA_NAV_NAME "data.nav.lz4")
SET(EITRI "${CMAKE_SOURCE_DIR}/eitri/eitri.py")

# TODO: if more data.nav have to be created, refactor this to be more generic

# == Nav generation ==
SET(DATA_DIR_TO_LOAD ${FIXTURES_DIR}/ed/ntfs/)

SET(DATA_NAV_TO_CREATE ${CMAKE_CURRENT_BINARY_DIR}/ntfs_${DATA_NAV_NAME})

add_custom_command(OUTPUT ${DATA_NAV_TO_CREATE}
    COMMAND python
    ARGS ${EITRI} "${FIXTURES_DIR}/ed/ntfs/" --output-file ${DATA_NAV_TO_CREATE} --ed-component-path "${CMAKE_BINARY_DIR}/ed" --add-pythonpath "${CMAKE_SOURCE_DIR}/navitiacommon"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/type" VERBATIM
)
set_source_files_properties(${DATA_NAV_TO_CREATE} PROPERTIES GENERATED TRUE)


# == create a target with all generated files ==
# we add dependencies to all data import components used during nav file creation
add_custom_target(datanav_files DEPENDS ${DATA_NAV_TO_CREATE} ${ED_TARGETS_TO_INSTALL})

# ==
# add all the tests
#
# Note: each tests should have their data set given with the environment variable 'INPUT_FILE_PATH'
# ==

SET(BOOST_LIBS ${BOOST_LIBS} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

add_executable(ed_integration_test ed_integration_tests.cpp)
target_link_libraries(ed_integration_test data georef types utils ${BOOST_LIBS} log4cplus)

# the test is not added with 'add_test', because we don't want 'make test' to handle it
# it is added as a post build command to 'make docker_test', called after all the build
add_custom_target(run_test DEPENDS datanav_files ed_integration_test)
add_custom_command(TARGET run_test
                   COMMAND ed_integration_test
                   ARGS --log_format=XML --log_sink=results_ed_integration_test.xml --log_level=all --report_level=no --ntfs_file=${DATA_NAV_TO_CREATE})


add_dependencies(ed_integration_test datanav_files)
add_dependencies(docker_test run_test)
