SET(vptranslator_src
    vptranslator.h
    vptranslator.cpp)
add_library(vptranslator ${vptranslator_src})
target_link_libraries(vptranslator types ${Boost_DATE_TIME_LIBRARY} ${Boost_SERIALIZATION_LIBRARY} data)


add_executable (vptranslator_test test.cpp)
target_link_libraries(vptranslator_test types ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} vptranslator)
ADD_BOOST_TEST(vptranslator_test)

