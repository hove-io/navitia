FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework date_time REQUIRED)
#link_directories ( ${Boost_LIBRARY_DIRS} )
add_executable(time_dep_test time_dep_test.cpp)
target_link_libraries(time_dep_test connectors utils data georef routing types utils ${Boost_LIBRARIES})
ADD_TEST(time_dep_test ${EXECUTABLE_OUTPUT_PATH}/time_dep_test --report_level=no)


add_executable(raptor_test raptor_test.cpp)
target_link_libraries(raptor_test ${Boost_LIBRARIES} connectors data routing georef utils ${Boost_LIBRARIES})
ADD_TEST(raptor ${EXECUTABLE_OUTPUT_PATH}/raptor_test --report_level=no)


#add_executable(time_expanded_test time_exp_test.cpp)
#target_link_libraries(time_expanded_test ${Boost_LIBRARIES} routing connectors)
#ADD_TEST(time_expanded ${EXECUTABLE_OUTPUT_PATH}/time_expanded_test --report_level=no)

add_executable(reverse_raptor_test reverse_raptor_test.cpp)
target_link_libraries(reverse_raptor_test connectors data routing georef utils ${Boost_LIBRARIES} )
ADD_TEST(reverse_raptor ${EXECUTABLE_OUTPUT_PATH}/reverse_raptor_test --report_level=no)


add_executable(rab_raptor_test test_rabattement.cpp)
target_link_libraries(rab_raptor_test connectors data georef utils  routing ${Boost_LIBRARIES} )
ADD_TEST(rab_raptor_test ${EXECUTABLE_OUTPUT_PATH}/rab_raptor_test --report_level=no)

add_executable(routing_api_test routing_api_test.cpp)
target_link_libraries(routing_api_test connectors data types routing pb_lib interface georef utils  ${Boost_LIBRARIES})
ADD_TEST(routing_api_test ${EXECUTABLE_OUTPUT_PATH}/routing_api_test --report_level=no)
