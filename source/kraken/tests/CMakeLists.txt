set(KRAKEN_TEST_LINK_LIBS
    workers
    ${NAVITIA_ALLOCATOR}
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
)

add_executable(data_manager_test data_manager_test.cpp)
target_link_libraries(data_manager_test types ${NAVITIA_ALLOCATOR} ${KRAKEN_TEST_LINK_LIBS})
ADD_BOOST_TEST(data_manager_test)

add_executable(data_manager_with_type_data_test data_manager_with_type_data_test.cpp)
target_link_libraries(data_manager_with_type_data_test make_disruption_from_chaos ${KRAKEN_TEST_LINK_LIBS})
ADD_BOOST_TEST(data_manager_with_type_data_test)

add_executable(disruption_reader_test disruption_reader_test.cpp)
target_link_libraries(disruption_reader_test ${KRAKEN_TEST_LINK_LIBS})
ADD_BOOST_TEST(disruption_reader_test)

add_executable(fill_disruption_from_chaos_tests fill_disruption_from_chaos_tests.cpp)
target_link_libraries(fill_disruption_from_chaos_tests make_disruption_from_chaos ed ${KRAKEN_TEST_LINK_LIBS})
ADD_BOOST_TEST(fill_disruption_from_chaos_tests)

add_executable(worker_test worker_test.cpp)
target_link_libraries(worker_test ed ${KRAKEN_TEST_LINK_LIBS})
ADD_BOOST_TEST(worker_test)

add_executable(realtime_test realtime_test.cpp)
target_link_libraries(realtime_test ed disruption_api rt_handling ${KRAKEN_TEST_LINK_LIBS})
ADD_BOOST_TEST(realtime_test)

add_executable(apply_disruption_test apply_disruption_test.cpp)
target_link_libraries(apply_disruption_test apply_disruption ed rt_handling ${KRAKEN_TEST_LINK_LIBS})
ADD_BOOST_TEST(apply_disruption_test)

add_executable(direct_path_test direct_path_test.cpp)
target_link_libraries(direct_path_test ed ${KRAKEN_TEST_LINK_LIBS})
ADD_BOOST_TEST(direct_path_test)

add_executable(disruption_periods_test disruption_periods_test.cpp)
target_link_libraries(disruption_periods_test apply_disruption ed ${KRAKEN_TEST_LINK_LIBS})
ADD_BOOST_TEST(disruption_periods_test)
