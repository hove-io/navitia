# Wrap Tyr tests in cmake
# since the tests need docker to run we put them in the docker_test target
# This is also the reason why we don't use cmake's `add_test`
# (because we don't want `make test` to run them)

set(TYR_TEST_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

add_custom_target(tyr_tests)

add_custom_command(TARGET tyr_tests
                   COMMAND PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}:${CMAKE_SOURCE_DIR}/navitiacommon py.test
                        --doctest-modules
                        --junit-xml=${CMAKE_BINARY_DIR}/tyr_tests.xml
                        --ignore=${CMAKE_CURRENT_SOURCE_DIR}/migrations/
                        --cov=tyr
                        --cov-report=xml:${CMAKE_BINARY_DIR}/tyr_integration_coverage.xml
                        ${TYR_TEST_PATH})

add_dependencies(docker_test tyr_tests)
