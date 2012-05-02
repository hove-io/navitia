FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework REQUIRED)
add_executable (proximity_list_test test.cpp proximity_list.h ../type/type.cpp)
target_link_libraries(proximity_list_test ${Boost_LIBRARIES} proj)
#ADD_TEST(test_proximity_list ${EXECUTABLE_OUTPUT_PATH}/proximity_list_test --report_level=no)
