FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework REQUIRED)
link_directories ( ${Boost_LIBRARY_DIRS} )

SET(SOURCE_TD
    time_dep_test.cpp
)

add_executable(time_dep_test ${SOURCE_TD})
target_link_libraries(time_dep_test ${Boost_LIBRARIES} routing connectors)

ADD_TEST(time_dep_test ${EXECUTABLE_OUTPUT_PATH}/time_dep_test --report_level=no)


SET(SOURCE_RA
    raptor_test.cpp
)
add_executable(raptor_test ${SOURCE_RA})
target_link_libraries(raptor_test ${Boost_LIBRARIES} routing connectors)

ADD_TEST(raptor ${EXECUTABLE_OUTPUT_PATH}/raptor_test --report_level=no)


SET(SOURCE_TE
    time_exp_test.cpp
)
add_executable(time_expanded_test ${SOURCE_TE})
target_link_libraries(time_expanded_test ${Boost_LIBRARIES} routing connectors)

ADD_TEST(time_expanded ${EXECUTABLE_OUTPUT_PATH}/time_expanded_test --report_level=no)
