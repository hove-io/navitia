#We use the BOOST_LIBS define is the parent: routing
SET(BOOST_LIBS ${BOOST_LIBS} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
add_executable(raptor_test raptor_test.cpp)
target_link_libraries(raptor_test ${Boost_LIBRARIES} ed data fare routing georef
    autocomplete utils ${BOOST_LIBS} log4cplus)
ADD_TEST(raptor ${EXECUTABLE_OUTPUT_PATH}/raptor_test --report_level=no)

add_executable(reverse_raptor_test reverse_raptor_test.cpp)
target_link_libraries(reverse_raptor_test ed data fare routing georef autocomplete
    utils ${BOOST_LIBS} log4cplus)
ADD_TEST(reverse_raptor ${EXECUTABLE_OUTPUT_PATH}/reverse_raptor_test --report_level=no)

add_executable(routing_api_test routing_api_test.cpp)
target_link_libraries(routing_api_test ed data types routing fare pb_lib georef
    autocomplete utils  ${BOOST_LIBS} log4cplus)
ADD_TEST(routing_api_test ${EXECUTABLE_OUTPUT_PATH}/routing_api_test --report_level=no)

add_executable(best_stoptime_test best_stoptime.cpp)
target_link_libraries(best_stoptime_test ed connectors data fare types routing
    autocomplete pb_lib autocomplete georef utils ${BOOST_LIBS} log4cplus)
ADD_TEST(best_stoptime_test ${EXECUTABLE_OUTPUT_PATH}/best_stoptime_test --report_level=no)
