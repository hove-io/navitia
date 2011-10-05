FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework REQUIRED)
FIND_PACKAGE(WS_Commons)
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories ( ${Boost_INCLUDE_DIRS} )

MAKE_WS(trie trie.cpp)

add_executable (first_letter_test test.cpp first_letter.h)
target_link_libraries(first_letter_test ${Boost_LIBRARIES})
ADD_TEST(test_first_letter ${EXECUTABLE_OUTPUT_PATH}/first_letter_tests --report_level=no)
