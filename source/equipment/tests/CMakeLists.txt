SET(BOOST_LIBS ${BOOST_LIB} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

add_executable(equipment_test equipment_api_tests.cpp)
target_link_libraries(equipment_test
    equipment_api ed data georef utils ${BOOST_LIBS} protobuf)
ADD_BOOST_TEST(equipment_test)
