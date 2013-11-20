add_executable (proximity_list_test tests/test.cpp proximity_list.h)

target_link_libraries(proximity_list_test types utils
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

ADD_TEST(test_proximity_list ${EXECUTABLE_OUTPUT_PATH}/proximity_list_test --report_level=no)

add_library(proximitylist proximity_list.h proximitylist_api.cpp)
