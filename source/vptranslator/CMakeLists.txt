SET(vptranslator_src
    vptranslator.h
    vptranslator.cpp)
add_library(vptranslator ${vptranslator_src})


FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework  REQUIRED)
add_executable (vptranslator_test test.cpp)
target_link_libraries(vptranslator_test ${Boost_LIBRARIES} vptranslator)
ADD_TEST(vptranslator_test ${EXECUTABLE_OUTPUT_PATH}/vptranslator_test --report_level=no)

