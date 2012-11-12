FIND_PACKAGE(Boost 1.44 COMPONENTS unit_test_framework REQUIRED)

add_executable(ptref_test ptret_test.cpp)
target_link_libraries(ptref_test ${Boost_LIBRARIES} ptreferential types)
ADD_TEST(ptref_test ${EXECUTABLE_OUTPUT_PATH}/ptref_test --report_level=no)
