SET(vptranslator_src
    vptranslator.h
    vptranslator.cpp)
add_library(vptranslator ${vptranslator_src})


add_executable (vptranslator_test test.cpp)
target_link_libraries(vptranslator_test ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} vptranslator)
ADD_BOOST_TEST(vptranslator_test)

