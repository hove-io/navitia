add_executable (proximity_list_test test.cpp)

SET(BOOST_LIBS ${Boost_THREAD_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_REGEX_LIBRARY}
    ${Boost_SERIALIZATION_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

target_link_libraries(proximity_list_test proximitylist ptreferential routing georef pb_lib data fare types routing autocomplete utils ${BOOST_LIBS} log4cplus pthread)

ADD_BOOST_TEST(proximity_list_test)
