#We use the BOOST_LIBS define is the parent
SET(BOOST_LIBS ${BOOST_LIBS} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

SET(SOURCE
    tests.cpp
)

add_executable(gtfs_parser_test gtfsparser_test.cpp)
target_link_libraries(gtfs_parser_test ed connectors types utils ${BOOST_LIBS} log4cplus)
ADD_TEST(gtfs_parser_test ${EXECUTABLE_OUTPUT_PATH}/gtfs_parser_test --report_level=no)

add_executable(adapted_test adapted_test.cpp)
target_link_libraries(adapted_test ed types connectors pq pqxx data georef
    routing utils autocomplete ${BOOST_LIBS} log4cplus)

ADD_TEST(adapted_test ${EXECUTABLE_OUTPUT_PATH}/adapted_test --report_level=no)

add_executable(extcode2uri_test extcode2uri_test.cpp)
target_link_libraries(extcode2uri_test ed connectors types utils hiredis ${BOOST_LIBS} log4cplus)
ADD_TEST(extcode2uri_test ${EXECUTABLE_OUTPUT_PATH}/extcode2uri_test --report_level=no)
