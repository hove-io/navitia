add_executable(fare_test fare_test.cpp)
target_link_libraries(fare_test fare config connectors ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
ADD_BOOST_TEST(fare_test)

add_executable(fare_integration_test fare_integration_test.cpp)
target_link_libraries(fare_integration_test pb_converter ed ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} )
ADD_BOOST_TEST(fare_integration_test)

