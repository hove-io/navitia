SET(BOOST_LIBS ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY} ${Boost_SERIALIZATION_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_REGEX_LIBRARY}
    ${Boost_IOSTREAMS_LIBRARY})

add_executable(get_stop_times get_stop_times.cpp)
target_link_libraries(get_stop_times time_tables ed data fare georef routing types utils ${BOOST_LIBS} log4cplus pb_lib protobuf)
ADD_BOOST_TEST(get_stop_times)


add_executable(thermometer_test thermometer.cpp)
target_link_libraries(thermometer_test time_tables ptreferential ed data fare routing utils pb_lib thermometer georef ${BOOST_LIBS} log4cplus protobuf)
ADD_BOOST_TEST(thermometer_test)


add_executable(departure_boards_test departure_boards_test.cpp)
target_link_libraries(departure_boards_test time_tables ptreferential ed data fare routing utils pb_lib thermometer georef ${BOOST_LIBS} log4cplus pthread protobuf)
ADD_BOOST_TEST(departure_boards_test)



add_executable(route_schedules_test route_schedules_test.cpp)
target_link_libraries(route_schedules_test time_tables ptreferential ed data fare routing utils pb_lib thermometer georef ${BOOST_LIBS} log4cplus pthread protobuf)
ADD_BOOST_TEST(route_schedules_test)
