SET(BOOST_LIBS ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY} ${Boost_SERIALIZATION_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_REGEX_LIBRARY}
    ${Boost_IOSTREAMS_LIBRARY})

add_executable(get_stop_times get_stop_times.cpp)
target_link_libraries(get_stop_times time_tables ed data georef routing types utils ${BOOST_LIBS} log4cplus)
ADD_TEST(get_stop_times ${EXECUTABLE_OUTPUT_PATH}/get_stop_times --report_level=no)


add_executable(thermometer thermometer.cpp)
target_link_libraries(thermometer time_tables ptreferential ed data routing utils pb_lib georef ${BOOST_LIBS} log4cplus)
ADD_TEST(thermometer ${EXECUTABLE_OUTPUT_PATH}/thermometer --report_level=no)


add_executable(departure_boards_test departure_boards_test.cpp)
target_link_libraries(departure_boards_test time_tables ptreferential ed data routing utils pb_lib georef ${BOOST_LIBS} log4cplus)
ADD_TEST(departure_boards_test ${EXECUTABLE_OUTPUT_PATH}/departure_boards_test --report_level=no)
