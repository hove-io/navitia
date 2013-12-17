add_executable (proximity_list_test test.cpp)

target_link_libraries(proximity_list_test proximitylist ptreferential routing georef pb_lib data types routing autocomplete utils ${Boost_LIBRARIES} log4cplus)

ADD_TEST(proximity_list_test ${EXECUTABLE_OUTPUT_PATH}/proximity_list_test --report_level=no)
