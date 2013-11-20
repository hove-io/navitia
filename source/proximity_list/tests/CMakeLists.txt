FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework filesystem system serialization date_time thread regex REQUIRED)

add_executable (proximity_list_test test.cpp)

target_link_libraries(proximity_list_test proximitylist ptreferential routing georef pb_lib data types routing autocomplete utils ${Boost_LIBRARIES} log4cplus)

ADD_TEST(proximity_list_test ${EXECUTABLE_OUTPUT_PATH}/proximity_list_test --report_level=no)
