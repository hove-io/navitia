add_executable(position_test position_api_tests.cpp)
target_link_libraries(position_test position_api rt_handling ed ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY})
ADD_BOOST_TEST(position_test)
