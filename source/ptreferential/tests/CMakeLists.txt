
add_executable(ptref_test ptref_test.cpp)
target_link_libraries(ptref_test ed ptreferential data ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

ADD_BOOST_TEST(ptref_test)

add_executable(ptref_legacy_test ptref_legacy_test.cpp)
target_link_libraries(ptref_legacy_test ed data ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

ADD_BOOST_TEST(ptref_legacy_test)

add_executable(ptref_ng_test ptref_ng_test.cpp)
target_link_libraries(ptref_ng_test ed data ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

ADD_BOOST_TEST(ptref_ng_test)

add_executable(ptref_odt_level_test ptref_odt_level_test.cpp)
target_link_libraries(ptref_odt_level_test ed data  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

ADD_BOOST_TEST(ptref_odt_level_test)


add_executable(ptref_companies_test ptref_companies_test.cpp)
target_link_libraries(ptref_companies_test ed data ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

ADD_BOOST_TEST(ptref_companies_test)
