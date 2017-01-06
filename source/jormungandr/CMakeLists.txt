#add python file to also handle version number in python modules
configure_file("${CMAKE_SOURCE_DIR}/navitia_version.py.cmake" "${CMAKE_CURRENT_SOURCE_DIR}/jormungandr/_version.py")


#tests 
set(JORMUN_PATH "${CMAKE_CURRENT_SOURCE_DIR}/jormungandr")
add_test(jormungandr py.test --doctest-modules --junit-xml=${CMAKE_BINARY_DIR}/nosetest_jormungandr.xml
    --cov-report=xml:${CMAKE_BINARY_DIR}/jormungandr_coverage.xml --cov=jormungandr ${JORMUN_PATH})
SET_PROPERTY(TEST jormungandr PROPERTY
    ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}:${CMAKE_SOURCE_DIR}/navitiacommon")

add_test(jormungandr_serpy py.test --doctest-modules --junit-xml=${CMAKE_BINARY_DIR}/nosetest_jormungandr_serpy.xml
    --cov-report=xml:${CMAKE_BINARY_DIR}/jormungandr_coverage_serpy.xml --cov=jormungandr ${JORMUN_PATH})
SET_PROPERTY(TEST jormungandr_serpy PROPERTY
    ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}:${CMAKE_SOURCE_DIR}/navitiacommon" "JORMUNGANDR_CONFIG_FILE=${CMAKE_CURRENT_SOURCE_DIR}/tests/serpy_settings.py")

#there are import problems (the settings are not loaded)
#when doctest is run in the same time as nose
#I whould have prefered not to set the settings file, but it is not working, so for the moment
#we do it the weak way.
set(JORMUN_TEST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tests")
add_test(jormungandr_integration_tests nosetests --with-doctest -v
        --with-xunit --xunit-file=${CMAKE_BINARY_DIR}/nosetest_jormungandr_integration_tests.xml --doctest-tests "--where=${JORMUN_TEST_PATH}"
        --with-coverage --cover-inclusive --cover-xml --cover-package=jormungandr
        --cover-xml-file=${CMAKE_BINARY_DIR}/jormungandr_integration_coverage.xml)
SET_PROPERTY(TEST jormungandr_integration_tests PROPERTY
    ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}:${CMAKE_SOURCE_DIR}/navitiacommon")
SET_PROPERTY(TEST jormungandr_integration_tests APPEND PROPERTY
    ENVIRONMENT "KRAKEN_BUILD_DIR=${CMAKE_BINARY_DIR}")

add_test(jormungandr_serpy_integration_tests nosetests --with-doctest -v
        --with-xunit --xunit-file=${CMAKE_BINARY_DIR}/nosetest_jormungandr_serpy_integration_tests.xml --doctest-tests "--where=${JORMUN_TEST_PATH}"
        --with-coverage --cover-inclusive --cover-xml --cover-package=jormungandr
        --cover-xml-file=${CMAKE_BINARY_DIR}/jormungandr_serpy_integration_coverage.xml)
SET_PROPERTY(TEST jormungandr_serpy_integration_tests PROPERTY
    ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}:${CMAKE_SOURCE_DIR}/navitiacommon" "JORMUNGANDR_CONFIG_FILE=${CMAKE_CURRENT_SOURCE_DIR}/tests/integration_tests_serpy_settings.py")
SET_PROPERTY(TEST jormungandr_serpy_integration_tests APPEND PROPERTY
    ENVIRONMENT "KRAKEN_BUILD_DIR=${CMAKE_BINARY_DIR}")

SET(CUSTOM_TIMEOUT $ENV{CUSTOM_INSTANCE_TIMEOUT})
IF(CUSTOM_TIMEOUT)
SET_PROPERTY(TEST jormungandr_integration_tests APPEND PROPERTY
    ENVIRONMENT "CUSTOM_INSTANCE_TIMEOUT=${CUSTOM_TIMEOUT}")
SET_PROPERTY(TEST jormungandr_serpy_integration_tests APPEND PROPERTY
    ENVIRONMENT "CUSTOM_INSTANCE_TIMEOUT=${CUSTOM_TIMEOUT}")
ENDIF()

