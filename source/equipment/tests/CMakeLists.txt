#We use the BOOST_LIBS define is the parent
#SET(BOOST_LIBS ${BOOST_LIB} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_FILESYSTEM_LIBRARY}
#    ${Boost_SYSTEM_LIBRARY} ${Boost_SERIALIZATION_LIBRARY}
#    ${Boost_DATE_TIME_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_REGEX_LIBRARY})

SET(BOOST_LIBS ${BOOST_LIB} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

add_executable(equipment_test equipment_api_tests.cpp)
target_link_libraries(equipment_test
    equipment_api ed data georef utils ${BOOST_LIBS} protobuf)
ADD_BOOST_TEST(equipment_test)
