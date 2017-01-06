
SET(DATA_NAV_NAME "data.nav.lz4")
SET(EITRI "${CMAKE_SOURCE_DIR}/eitri/eitri.py")

# TODO: if more data.nav have to be created, refactor this to be more generic

# == Nav generation ==
# this function calls eitri to build a data.nav.lz4 file for the DATASET_NAME
macro(generate_nav DATASET_NAME)
    SET(DATA_NAV_TO_CREATE ${CMAKE_CURRENT_BINARY_DIR}/${DATASET_NAME}_${DATA_NAV_NAME})

    add_custom_command(OUTPUT ${DATA_NAV_TO_CREATE}
        DEPENDS ${ED_TARGETS_TO_INSTALL}
        COMMAND python
        ARGS ${EITRI} "${FIXTURES_DIR}/ed/${DATASET_NAME}/" --output-file ${DATA_NAV_TO_CREATE} --ed-component-path "${CMAKE_BINARY_DIR}/ed" --add-pythonpath "${CMAKE_SOURCE_DIR}/navitiacommon"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/type" VERBATIM
    )
    set_source_files_properties(${DATA_NAV_TO_CREATE} PROPERTIES GENERATED TRUE)

    LIST(APPEND FILES_CREATED ${DATA_NAV_TO_CREATE})

    # we fill the TEST_CLI_PARAMS with the name of the dataset and the path of the data.nav.lz4 files
    # since it is a macro it will be available outside of this function
    SET(TEST_CLI_PARAMS ${TEST_CLI_PARAMS} --${DATASET_NAME}_file=${DATA_NAV_TO_CREATE})
endmacro()

generate_nav("ntfs")
generate_nav("gtfs_google_example")
generate_nav("ntfs_v5")

# == create a target with all generated files ==
# we add dependencies to all data import components used during nav file creation
add_custom_target(datanav_files DEPENDS ${FILES_CREATED})

# ==
# add all the tests
#
# Note: each tests should have their data set given with a command line parameter
# ==

SET(BOOST_LIBS ${BOOST_LIBS} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

add_executable(ed_integration_test ed_integration_tests.cpp)
target_link_libraries(ed_integration_test data georef types utils ${BOOST_LIBS} log4cplus)

# the test is not added with 'add_test', because we don't want 'make test' to handle it
# it is added as a post build command to 'make docker_test', called after all the build
add_custom_target(run_test DEPENDS datanav_files ed_integration_test)
if(${Boost_VERSION} LESS 106000) #if boost version strictly older than 1.60
add_custom_command(TARGET run_test
                   COMMAND ed_integration_test
                   ARGS --log_format=XML --log_sink=results_ed_integration_test.xml --log_level=all --report_level=no ${TEST_CLI_PARAMS})
else() #if using newer version of boost, passing cli_params new way
add_custom_command(TARGET run_test
                   COMMAND ed_integration_test
                   ARGS --log_format=XML --log_sink=results_ed_integration_test.xml --log_level=all --report_level=no -- ${TEST_CLI_PARAMS})
endif()

add_dependencies(ed_integration_test datanav_files)
add_dependencies(docker_test run_test)
