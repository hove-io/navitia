SET(vptranslator_src
    vptranslator.h
    vptranslator.cpp)
add_library(vptranslator ${vptranslator_src})
target_link_libraries(vptranslator types)

# Add tests
if(NOT SKIP_TESTS)
    add_executable (vptranslator_test test.cpp)
    target_link_libraries(vptranslator_test types ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
    ADD_BOOST_TEST(vptranslator_test)
endif(NOT SKIP_TESTS)
