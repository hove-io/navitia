add_executable (autocomplete_test tests/test.cpp)
target_link_libraries(autocomplete_test ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${Boost_REGEX_LIBRARY} ${Boost_SYSTEM_LIBRARY})

ADD_BOOST_TEST(autocomplete_test)

add_library(autocomplete autocomplete.cpp autocomplete_api.cpp)
target_link_libraries(autocomplete pb_lib)
