set(PTREF_TEST_LINK_LIBS
    ed
    data
    ptreferential
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
)
add_executable(ptref_test ptref_test.cpp)
target_link_libraries(ptref_test ${PTREF_TEST_LINK_LIBS})
ADD_BOOST_TEST(ptref_test)

add_executable(ptref_ng_test ptref_ng_test.cpp)
target_link_libraries(ptref_ng_test ${PTREF_TEST_LINK_LIBS})
ADD_BOOST_TEST(ptref_ng_test)

add_executable(ptref_odt_level_test ptref_odt_level_test.cpp)
target_link_libraries(ptref_odt_level_test ${PTREF_TEST_LINK_LIBS})
ADD_BOOST_TEST(ptref_odt_level_test)

add_executable(ptref_companies_test ptref_companies_test.cpp)
target_link_libraries(ptref_companies_test ${PTREF_TEST_LINK_LIBS})
ADD_BOOST_TEST(ptref_companies_test)

add_executable(vehicle_journey_test vehicle_journey_test.cpp)
target_link_libraries(vehicle_journey_test ${PTREF_TEST_LINK_LIBS})
ADD_BOOST_TEST(vehicle_journey_test)
