
add_library(autocomplete autocomplete.cpp autocomplete_api.cpp utils.cpp)
target_link_libraries(autocomplete pb_lib)
add_dependencies(autocomplete protobuf_files)

# Add tests
if(NOT SKIP_TESTS)
    add_executable(autocomplete_test tests/test.cpp tests/test_utils.cpp)
    target_link_libraries(autocomplete_test autocomplete ed ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_REGEX_LIBRARY})
    ADD_BOOST_TEST(autocomplete_test)
endif(NOT SKIP_TESTS)
