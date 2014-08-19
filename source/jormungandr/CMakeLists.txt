set(JORMUN_PATH "${CMAKE_CURRENT_SOURCE_DIR}/jormungandr")
add_test(jormungandr nosetests --with-doctest -v --with-xunit  --xunit-file=${CMAKE_BINARY_DIR}/nosetest_jormungandr.xml "--where=${JORMUN_PATH}")
SET_PROPERTY(TEST jormungandr PROPERTY
    ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}:${CMAKE_SOURCE_DIR}/navitiacommon")

#there are import problems (the settings are not loaded)
#when doctest is run in the same time as nose
#I whould have prefered not to set the settings file, but it is not working, so for the moment
#we do it the weak way.
set(JORMUN_TEST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tests")
add_test(jormungandr_integration_tests nosetests --with-doctest -v
        --with-xunit --xunit-file=${CMAKE_BINARY_DIR}/nosetest_jormungandr_integration_tests.xml --doctest-tests "--where=${JORMUN_TEST_PATH}")
SET_PROPERTY(TEST jormungandr_integration_tests PROPERTY
    ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}:${CMAKE_SOURCE_DIR}/navitiacommon")
SET_PROPERTY(TEST jormungandr_integration_tests APPEND PROPERTY
    ENVIRONMENT "KRAKEN_BUILD_DIR=${CMAKE_BINARY_DIR}")
