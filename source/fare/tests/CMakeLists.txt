add_executable(fare_test fare_test.cpp)
target_link_libraries(fare_test fare connectors data georef routing types autocomplete utils
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_DATE_TIME_LIBRARY} ${Boost_SYSTEM_LIBRARY}
    ${Boost_REGEX_LIBRARY} ${Boost_SERIALIZATION_LIBRARY} log4cplus)
ADD_BOOST_TEST(fare_test)

add_executable(fare_integration_test fare_integration_test.cpp)
target_link_libraries(fare_integration_test fare ed connectors data georef routing types pb_lib autocomplete utils
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY}
    ${Boost_REGEX_LIBRARY} ${Boost_SERIALIZATION_LIBRARY} ${Boost_DATE_TIME_LIBRARY} log4cplus pthread)
ADD_BOOST_TEST(fare_integration_test)

