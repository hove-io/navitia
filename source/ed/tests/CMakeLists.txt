#We use the BOOST_LIBS define is the parent
SET(BOOST_LIBS ${BOOST_LIBS} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

SET(SOURCE
    tests.cpp
)


#add_executable(navimake_test ${SOURCE})
#target_link_libraries(navimake_test connectors types utils ${Boost_LIBRARIES} log4cplus)

#ADD_TEST(navimake_test ${EXECUTABLE_OUTPUT_PATH}/navimake_test --report_level=no)

add_executable(gtfs_parser_test gtfsparser_test.cpp)
target_link_libraries(gtfs_parser_test ed connectors types utils ${BOOST_LIBS} log4cplus)
ADD_TEST(gtfs_parser_test ${EXECUTABLE_OUTPUT_PATH}/gtfs_parser_test --report_level=no)

add_executable(adapted_test adapted_test.cpp)
target_link_libraries(adapted_test ed types connectors pq pqxx data georef
    routing utils autocomplete ${BOOST_LIBS} log4cplus)

ADD_TEST(adapted_test ${EXECUTABLE_OUTPUT_PATH}/adapted_test --report_level=no)

add_executable(osm_test osm_test.cpp)
target_link_libraries(osm_test ed connectors osmpbf types data georef pb_lib
    utils routing ${BOOST_LIBS} log4cplus)

