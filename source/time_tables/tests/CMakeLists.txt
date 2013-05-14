FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework iostreams serialization date_time thread regex REQUIRED)

add_executable(get_stop_times get_stop_times.cpp)
target_link_libraries(get_stop_times time_tables connectors data georef routing types utils ${Boost_LIBRARIES} log4cplus)
ADD_TEST(get_stop_times ${EXECUTABLE_OUTPUT_PATH}/get_stop_times --report_level=no)


add_executable(thermometer thermometer.cpp)
target_link_libraries(thermometer time_tables ptreferential connectors data routing types utils pb_lib georef ${Boost_LIBRARIES} log4cplus)
ADD_TEST(thermometer ${EXECUTABLE_OUTPUT_PATH}/thermometer --report_level=no)

