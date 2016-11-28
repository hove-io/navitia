#We use the BOOST_LIBS define is the parent
SET(BOOST_LIBS ${BOOST_LIBS} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

add_executable(jpp_shape_test jpp_shape_test.cpp)
target_link_libraries(jpp_shape_test ed data types ${BOOST_LIBS})
ADD_BOOST_TEST(jpp_shape_test)

add_executable(gtfs_parser_test gtfsparser_test.cpp)
target_link_libraries(gtfs_parser_test connectors data ed types utils ${BOOST_LIBS} log4cplus)
ADD_BOOST_TEST(gtfs_parser_test)

add_executable(fusio_parser_test fusioparser_test.cpp)
target_link_libraries(fusio_parser_test connectors data ed types utils ${BOOST_LIBS} log4cplus)
ADD_BOOST_TEST(fusio_parser_test)

add_executable(osm_tags_reader_test osm_tags_reader_test.cpp)
target_link_libraries(osm_tags_reader_test connectors data ed types utils ${BOOST_LIBS} log4cplus)
ADD_BOOST_TEST(osm_tags_reader_test)


add_executable(associated_calendar_test associated_calendar_test.cpp)
target_link_libraries(associated_calendar_test ed data types routing fare georef autocomplete utils ${BOOST_LIBS} log4cplus pb_lib protobuf)
ADD_BOOST_TEST(associated_calendar_test)

add_executable(fare_parser_test fare_parser_test.cpp)
target_link_libraries(fare_parser_test connectors fare utils ${BOOST_LIBS} log4cplus)
ADD_BOOST_TEST(fare_parser_test)

add_executable(conv_coord_test conv_coord_test.cpp)
target_link_libraries(conv_coord_test connectors types utils ${BOOST_LIBS} log4cplus)
ADD_BOOST_TEST(conv_coord_test)

add_executable(shift_stop_times_test shift_stop_times.cpp)
target_link_libraries(shift_stop_times_test ed types utils ${BOOST_LIBS} log4cplus)
ADD_BOOST_TEST(shift_stop_times_test)


add_executable(route_main_destination_test route_main_destination_test.cpp)
target_link_libraries(route_main_destination_test ed types utils ${BOOST_LIBS} log4cplus)
ADD_BOOST_TEST(route_main_destination_test)
