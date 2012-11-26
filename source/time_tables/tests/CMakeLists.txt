FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework REQUIRED)

add_executable(get_stop_times get_stop_times.cpp)
target_link_libraries(get_stop_times ${Boost_LIBRARIES} time_tables connectors)
ADD_TEST(get_stop_times ${EXECUTABLE_OUTPUT_PATH}/get_stop_times --report_level=no)

add_executable(thermometer thermometer.cpp)
target_link_libraries(thermometer ${Boost_LIBRARIES} time_tables connectors data types)
ADD_TEST(thermometer ${EXECUTABLE_OUTPUT_PATH}/thermometer --report_level=no)
