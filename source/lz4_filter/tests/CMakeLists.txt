add_executable (lz4_tests test.cpp "${CMAKE_SOURCE_DIR}/third_party/lz4/lz4.c")
target_link_libraries(lz4_tests
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${Boost_IOSTREAMS_LIBRARY}
)
ADD_BOOST_TEST(lz4_tests)

