#
## Setup the BOOST tests that will exercise Chaos database
#
add_executable(chaos_db_tests chaos_db_tests.cpp)
target_link_libraries(chaos_db_tests
    make_disruption_from_chaos
    ed
    workers
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
)

target_link_libraries(chaos_db_tests ${ALL_LIBS})

#
## Setup the Pytest that will use 'chaos_db_tests'
#
set(CHAOS_TESTS_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
set(CHAOS_DB_TESTS_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}")
add_custom_target(
    chaos_tests
    DEPENDS chaos_db_tests
)
add_custom_command(
    TARGET chaos_tests
    WORKING_DIRECTORY ${CHAOS_TESTS_PATH}
    COMMAND PYTHONPATH=${CMAKE_SOURCE_DIR}/navitiacommon CHAOS_DB_TESTS_BUILD_DIR=${CHAOS_DB_TESTS_BUILD_DIR} py.test ${CHAOS_TESTS_PATH}
    COMMENT "Running Chaos's disruptions tests"
)
add_dependencies(docker_test chaos_tests)
