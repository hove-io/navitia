FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework  REQUIRED)
FIND_PACKAGE(WS_Commons)

add_executable (first_letter_test tests/test.cpp)
target_link_libraries(first_letter_test ${Boost_LIBRARIES})
ADD_TEST(test_first_letter ${EXECUTABLE_OUTPUT_PATH}/first_letter_test --report_level=no)

MAKE_WS(first_letter first_letter.h firstletter_ws.cpp LIBS types)
