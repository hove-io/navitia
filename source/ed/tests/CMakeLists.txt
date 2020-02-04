set(ED_TESTS_LINK_LIBS
    connectors
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
)


add_executable(jpp_shape_test jpp_shape_test.cpp)
target_link_libraries(jpp_shape_test ed ${ED_TESTS_LINK_LIBS})
ADD_BOOST_TEST(jpp_shape_test)

add_executable(gtfs_parser_test gtfsparser_test.cpp)
target_link_libraries(gtfs_parser_test ${ED_TESTS_LINK_LIBS})
ADD_BOOST_TEST(gtfs_parser_test)

add_executable(fusio_parser_test fusioparser_test.cpp)
target_link_libraries(fusio_parser_test ${ED_TESTS_LINK_LIBS})
ADD_BOOST_TEST(fusio_parser_test)

add_executable(osm_tags_reader_test osm_tags_reader_test.cpp)
target_link_libraries(osm_tags_reader_test ${ED_TESTS_LINK_LIBS})
ADD_BOOST_TEST(osm_tags_reader_test)

add_executable(associated_calendar_test associated_calendar_test.cpp)
target_link_libraries(associated_calendar_test ed ${ED_TESTS_LINK_LIBS})
ADD_BOOST_TEST(associated_calendar_test)

add_executable(fare_parser_test fare_parser_test.cpp)
target_link_libraries(fare_parser_test ${ED_TESTS_LINK_LIBS})
ADD_BOOST_TEST(fare_parser_test)

add_executable(conv_coord_test conv_coord_test.cpp)
target_link_libraries(conv_coord_test ${ED_TESTS_LINK_LIBS})
ADD_BOOST_TEST(conv_coord_test)

add_executable(shift_stop_times_test shift_stop_times.cpp)
target_link_libraries(shift_stop_times_test ed ${ED_TESTS_LINK_LIBS})
ADD_BOOST_TEST(shift_stop_times_test)

add_executable(osm2ed_test osm2ed_test.cpp)
target_link_libraries(osm2ed_test osm2ed_lib ${ED_TESTS_LINK_LIBS})
ADD_BOOST_TEST(osm2ed_test)

add_executable(poi2ed_test poi2ed_test.cpp)
target_link_libraries(poi2ed_test ${ED_TESTS_LINK_LIBS})
ADD_BOOST_TEST(poi2ed_test "${FIXTURES_DIR}/ed/poi/poi")

add_executable(fare2ed_test fare2ed_test.cpp)
target_link_libraries(fare2ed_test fare2ed_lib ${ED_TESTS_LINK_LIBS})

add_executable(ed2nav_test ed2nav_test.cpp)
target_link_libraries(ed2nav_test ed2nav_lib ${ED_TESTS_LINK_LIBS})
ADD_BOOST_TEST(ed2nav_test)

add_executable(route_main_destination_test route_main_destination_test.cpp)
target_link_libraries(route_main_destination_test ed ${ED_TESTS_LINK_LIBS})
ADD_BOOST_TEST(route_main_destination_test)


add_executable(speed_parser_test speed_parser_test.cpp)
target_link_libraries(speed_parser_test ${ED_TESTS_LINK_LIBS})
ADD_BOOST_TEST(speed_parser_test)
