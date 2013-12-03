add_executable (lz4_tests test.cpp ${CMAKE_SOURCE_DIR}/third_party/lz4/lz4.c)
target_link_libraries(lz4_tests ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${Boost_IOSTREAMS_LIBRARY})

ADD_TEST(test_lz4filter ${EXECUTABLE_OUTPUT_PATH}/lz4_tests --report_level=no)

