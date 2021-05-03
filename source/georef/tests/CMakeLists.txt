SET(GEOREF_SRC_TEST
    builder.h
    builder.cpp
)

add_library(georef_test_utils ${GEOREF_SRC_TEST})
target_link_libraries(georef_test_utils georef)

add_executable(georef_test georef_test.cpp)
target_link_libraries(georef_test georef_test_utils ed data ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} )
ADD_BOOST_TEST(georef_test)

add_executable(street_network_test street_network_test.cpp)
target_link_libraries(street_network_test georef_test_utils data ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} )
ADD_BOOST_TEST(street_network_test)

add_executable(path_finder_test path_finder_test.cpp)
target_link_libraries(path_finder_test georef_test_utils data ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} )
ADD_BOOST_TEST(path_finder_test)
