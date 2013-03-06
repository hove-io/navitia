FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework date_time REQUIRED)
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories ( ${Boost_INCLUDE_DIRS} 
                      ${CMAKE_SOURCE_DIR}
                      ${CMAKE_BINARY_DIR}
)

SET(SOURCE
    tests.cpp
)


add_executable(navimake_test ${SOURCE})
target_link_libraries(navimake_test connectors types utils ${Boost_LIBRARIES})

ADD_TEST(navimake_test ${EXECUTABLE_OUTPUT_PATH}/navimake_test --report_level=no)

add_executable(gtfs_parser_test gtfsparser_test.cpp)
target_link_libraries(gtfs_parser_test connectors types utils ${Boost_LIBRARIES})

ADD_TEST(gtfs_parser_test ${EXECUTABLE_OUTPUT_PATH}/gtfs_parser_test --report_level=no)
