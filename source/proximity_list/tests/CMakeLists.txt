add_executable (proximity_list_test test.cpp)

target_link_libraries(proximity_list_test proximitylist ptreferential routing georef pb_lib data fare types routing autocomplete utils ${Boost_LIBRARIES} log4cplus)

ADD_BOOST_TEST(proximity_list_test)
