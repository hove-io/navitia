FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework regex system REQUIRED)
FIND_PACKAGE(WS_Commons)

add_executable (autocomplete_test tests/test.cpp)
target_link_libraries(autocomplete_test ${Boost_LIBRARIES})
ADD_TEST(test_autocomplete ${EXECUTABLE_OUTPUT_PATH}/autocomplete_test --report_level=no)

add_library(autocomplete autocomplete.cpp autocomplete_api.cpp)
target_link_libraries(autocomplete pb_lib)
