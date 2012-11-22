FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework REQUIRED)

add_executable(next_departures next_departures.cpp)
target_link_libraries(next_departures ${Boost_LIBRARIES} time_tables connectors types)
ADD_TEST(next_departures ${EXECUTABLE_OUTPUT_PATH}/next_departures --report_level=no)

add_executable(thermometer thermometer.cpp)
target_link_libraries(thermometer ${Boost_LIBRARIES} time_tables connectors types)
ADD_TEST(thermometer ${EXECUTABLE_OUTPUT_PATH}/thermometer --report_level=no)
