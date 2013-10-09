FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework date_time REQUIRED)

add_executable(raptor_test raptor_test.cpp)
target_link_libraries(raptor_test ${Boost_LIBRARIES} ed data routing georef utils ${Boost_LIBRARIES} log4cplus)
ADD_TEST(raptor ${EXECUTABLE_OUTPUT_PATH}/raptor_test --report_level=no)

add_executable(reverse_raptor_test reverse_raptor_test.cpp)
target_link_libraries(reverse_raptor_test ed data routing georef utils ${Boost_LIBRARIES} log4cplus)
ADD_TEST(reverse_raptor ${EXECUTABLE_OUTPUT_PATH}/reverse_raptor_test --report_level=no)

add_executable(routing_api_test routing_api_test.cpp)
target_link_libraries(routing_api_test ed data types routing pb_lib georef utils  ${Boost_LIBRARIES} log4cplus)
ADD_TEST(routing_api_test ${EXECUTABLE_OUTPUT_PATH}/routing_api_test --report_level=no)

add_executable(best_stoptime_test best_stoptime.cpp)
target_link_libraries(best_stoptime_test connectors data types routing autocomplete pb_lib autocomplete georef utils  ${Boost_LIBRARIES} log4cplus)
ADD_TEST(best_stoptime_test ${EXECUTABLE_OUTPUT_PATH}/best_stoptime_test --report_level=no)
