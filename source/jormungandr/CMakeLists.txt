add_test(jormungandr nosetests -v --where=${CMAKE_CURRENT_SOURCE_DIR})
SET_PROPERTY(TEST jormungandr PROPERTY
    ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}:${CMAKE_SOURCE_DIR}/navitiacommon")

