set(TYPES_TEST_LINK_LIBS
    ed
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
)

add_executable(fill_pb_placemark_test fill_pb_placemark_test.cpp)
target_link_libraries(fill_pb_placemark_test
    data
    routing
    thermometer
    pb_converter
    ${TYPES_TEST_LINK_LIBS}
)
ADD_BOOST_TEST(fill_pb_placemark_test)

add_executable(type_test test.cpp)
target_link_libraries(type_test ${TYPES_TEST_LINK_LIBS})
ADD_BOOST_TEST(type_test)

add_executable(datetime_test datetime.cpp)
target_link_libraries(datetime_test ${TYPES_TEST_LINK_LIBS})
ADD_BOOST_TEST(datetime_test)

add_executable(accessible_test accessible.cpp)
target_link_libraries(accessible_test ${TYPES_TEST_LINK_LIBS})
ADD_BOOST_TEST(accessible_test)

add_executable(multi_polygon_map_test multi_polygon_map_test.cpp)
target_link_libraries(multi_polygon_map_test ${TYPES_TEST_LINK_LIBS})
ADD_BOOST_TEST(multi_polygon_map_test)

add_executable(fill_pb_object_tests fill_pb_object_tests.cpp)
target_link_libraries(fill_pb_object_tests pb_converter ${TYPES_TEST_LINK_LIBS})
ADD_BOOST_TEST(fill_pb_object_tests)

add_executable(aggregation_odt_test aggregation_odt_test.cpp)
target_link_libraries(aggregation_odt_test ${TYPES_TEST_LINK_LIBS})
ADD_BOOST_TEST(aggregation_odt_test)

add_executable(comments_test comments_test.cpp)
target_link_libraries(comments_test ${TYPES_TEST_LINK_LIBS})
ADD_BOOST_TEST(comments_test)

add_executable(data_test data_test.cpp)
target_link_libraries(data_test ${TYPES_TEST_LINK_LIBS})
ADD_BOOST_TEST(data_test)

add_executable(code_container_test code_container_test.cpp)
target_link_libraries(code_container_test ${TYPES_TEST_LINK_LIBS})
ADD_BOOST_TEST(code_container_test)

add_executable(headsign_test headsign_test.cpp)
target_link_libraries(headsign_test ${TYPES_TEST_LINK_LIBS})
ADD_BOOST_TEST(headsign_test)

add_executable(create_vj_test create_vj_test.cpp)
target_link_libraries(create_vj_test ${TYPES_TEST_LINK_LIBS})
ADD_BOOST_TEST(create_vj_test)
