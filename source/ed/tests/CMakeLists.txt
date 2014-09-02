#We use the BOOST_LIBS define is the parent
SET(BOOST_LIBS ${BOOST_LIBS} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

SET(SOURCE
    tests.cpp
)

add_executable(gtfs_parser_test gtfsparser_test.cpp)
target_link_libraries(gtfs_parser_test ed connectors data types utils ${BOOST_LIBS} log4cplus)
ADD_BOOST_TEST(gtfs_parser_test)

add_executable(adapted_test adapted_test.cpp)
target_link_libraries(adapted_test ed types connectors  pq pqxx data fare georef
    routing utils autocomplete ${BOOST_LIBS} log4cplus)

ADD_BOOST_TEST(adapted_test)

add_executable(fare_parser_test fare_parser_test.cpp)
target_link_libraries(fare_parser_test connectors fare utils ${BOOST_LIBS} log4cplus)
ADD_BOOST_TEST(fare_parser_test)

add_executable(conv_coord_test conv_coord_test.cpp)
target_link_libraries(conv_coord_test connectors types utils ${BOOST_LIBS} log4cplus)
ADD_BOOST_TEST(conv_coord_test)
