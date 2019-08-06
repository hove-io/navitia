include_directories("..")

add_executable(chaos_db_tests chaos_db_tests.cpp)

target_link_libraries(chaos_db_tests make_disruption_from_chaos apply_disruption ed workers data types pb_lib utils log4cplus ${NAVITIA_ALLOCATOR} ${Boost_LIBRARIES} ${Boost_DATE_TIME_LIBRARY} protobuf)
ADD_BOOST_TEST(apply_disruption_test)

target_link_libraries(chaos_db_tests ${ALL_LIBS})
