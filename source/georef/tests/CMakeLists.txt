FIND_PACKAGE(Boost 1.44 COMPONENTS unit_test_framework thread REQUIRED)


SET(GEOREF_SRC_TEST
    georef_test.cpp
    builder.h
    builder.cpp
)
add_executable(georef_test ${GEOREF_SRC_TEST})
target_link_libraries(georef_test  georef utils ${Boost_LIBRARIES})

ADD_TEST(georef_test ${EXECUTABLE_OUTPUT_PATH}/georef_test --report_level=no)
