add_test(jormungandr nosetests --with-doctest -v --where=${CMAKE_CURRENT_SOURCE_DIR}/jormungandr)
SET_PROPERTY(TEST jormungandr PROPERTY
    ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}:${CMAKE_SOURCE_DIR}/navitiacommon")

add_test(jormungandr_integration_tests nosetests -v --where=${CMAKE_CURRENT_SOURCE_DIR}/tests)
SET_PROPERTY(TEST jormungandr_integration_tests PROPERTY
    ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}:${CMAKE_SOURCE_DIR}/navitiacommon")

