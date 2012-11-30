FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework date_time thread serialization REQUIRED)

add_executable(get_stop_times get_stop_times.cpp)
target_link_libraries(get_stop_times time_tables connectors ptreferential data georef routing utils ${Boost_LIBRARIES})
ADD_TEST(get_stop_times ${EXECUTABLE_OUTPUT_PATH}/get_stop_times --report_level=no)

add_executable(thermometer thermometer.cpp)
target_link_libraries(thermometer time_tables connectors ptreferential data georef routing utils ${Boost_LIBRARIES})
ADD_TEST(thermometer ${EXECUTABLE_OUTPUT_PATH}/thermometer --report_level=no)
