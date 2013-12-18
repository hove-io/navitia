#We use the BOOST_LIBS define is the parent
SET(BOOST_LIBS ${BOOST_LIBS} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

SET(SOURCE
    disruption_test.cpp
)

add_executable(disruption_test disruption_test.cpp)
target_link_libraries(disruption_test ${BOOST_LIBS} log4cplus)
ADD_TEST(disruption_test ${EXECUTABLE_OUTPUT_PATH}/disruption_test --report_level=no)
