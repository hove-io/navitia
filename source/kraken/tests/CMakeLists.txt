#We use the BOOST_LIBS define is the parent
SET(BOOST_LIBS ${BOOST_LIBS} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
add_executable(data_manager_test data_manager_test.cpp)
target_link_libraries(data_manager_test types utils log4cplus ${NAVITIA_ALLOCATOR} ${Boost_LIBRARIES})
ADD_BOOST_TEST(data_manager_test)

add_executable(data_manager_with_type_data_test data_manager_with_type_data_test.cpp)
target_link_libraries(data_manager_with_type_data_test make_disruption_from_chaos workers log4cplus ${NAVITIA_ALLOCATOR} ${Boost_LIBRARIES})
ADD_BOOST_TEST(data_manager_with_type_data_test)

add_executable(disruption_reader_test disruption_reader_test.cpp)
target_link_libraries(disruption_reader_test workers data types pb_lib utils log4cplus ${NAVITIA_ALLOCATOR} ${Boost_LIBRARIES} ${Boost_DATE_TIME_LIBRARY} protobuf)
ADD_BOOST_TEST(disruption_reader_test)

add_executable(fill_disruption_from_chaos_tests fill_disruption_from_chaos_tests.cpp)
target_link_libraries(fill_disruption_from_chaos_tests make_disruption_from_chaos ed workers data types pb_lib utils log4cplus ${NAVITIA_ALLOCATOR} ${Boost_LIBRARIES} ${Boost_DATE_TIME_LIBRARY} protobuf)
ADD_BOOST_TEST(fill_disruption_from_chaos_tests)

add_executable(worker_test worker_test.cpp)
target_link_libraries(worker_test make_disruption_from_chaos ed workers data types pb_lib utils log4cplus ${NAVITIA_ALLOCATOR} ${Boost_LIBRARIES} ${Boost_DATE_TIME_LIBRARY} protobuf)
ADD_BOOST_TEST(worker_test)

add_executable(realtime_test realtime_test.cpp)
target_link_libraries(realtime_test disruption_api ed workers data types pb_lib utils log4cplus ${NAVITIA_ALLOCATOR} ${Boost_LIBRARIES} ${Boost_DATE_TIME_LIBRARY} protobuf)
ADD_BOOST_TEST(realtime_test)

add_executable(apply_disruption_test apply_disruption_test.cpp)
target_link_libraries(apply_disruption_test make_disruption_from_chaos apply_disruption ed workers data types pb_lib utils log4cplus ${NAVITIA_ALLOCATOR} ${Boost_LIBRARIES} ${Boost_DATE_TIME_LIBRARY} protobuf)
ADD_BOOST_TEST(apply_disruption_test)

add_executable(direct_path_test direct_path_test.cpp)
target_link_libraries(direct_path_test workers make_disruption_from_chaos apply_disruption
  ed data types routing fare pb_lib thermometer georef
  autocomplete utils  ${BOOST_LIBS} log4cplus pthread protobuf ${NAVITIA_ALLOCATOR})
ADD_BOOST_TEST(direct_path_test)

add_executable(disruption_periods_test disruption_periods_test.cpp)
target_link_libraries(disruption_periods_test workers data ed types pb_lib utils log4cplus ${NAVITIA_ALLOCATOR} ${Boost_LIBRARIES} ${Boost_DATE_TIME_LIBRARY} protobuf)
ADD_BOOST_TEST(disruption_periods_test)
