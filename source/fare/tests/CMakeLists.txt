add_executable(fare_test fare_test.cpp)
target_link_libraries(fare_test fare connectors utils
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_SYSTEM_LIBRARY}
    ${Boost_REGEX_LIBRARY} ${Boost_SERIALIZATION_LIBRARY} log4cplus)
ADD_TEST(fare_test ${EXECUTABLE_OUTPUT_PATH}/fare_test --report_level=no)

