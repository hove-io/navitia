SET(GEOREF_SRC_TEST
    georef_test.cpp
    builder.h
    builder.cpp
)
add_executable(georef_test ${GEOREF_SRC_TEST})
target_link_libraries(georef_test georef utils
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_THREAD_LIBRARY}
    ${Boost_REGEX_LIBRARY} ${Boost_SERIALIZATION_LIBRARY} log4cplus)

ADD_TEST(georef_test ${EXECUTABLE_OUTPUT_PATH}/georef_test --report_level=no)
