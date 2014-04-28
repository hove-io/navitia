#We use the BOOST_LIBS define is the parent
SET(BOOST_LIBS ${BOOST_LIB} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY} ${Boost_SERIALIZATION_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_REGEX_LIBRARY})

add_executable(calendar_api_test calendar_api_test.cpp)

target_link_libraries(calendar_api_test calendar_api types ptreferential pb_lib data fare routing ed georef autocomplete utils ${BOOST_LIBS} pthread)

ADD_TEST(calendar_api_test ${EXECUTABLE_OUTPUT_PATH}/calendar_api_test --report_level=no)
