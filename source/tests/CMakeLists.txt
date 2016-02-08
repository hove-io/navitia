SET(ALL_LIBS workers ed disruption_api calendar_api time_tables types autocomplete proximitylist
    ptreferential time_tables data routing fare georef utils SimpleAmqpClient
    rabbitmq-static log4cplus tcmalloc ${Boost_LIBRARIES} protobuf)

# every tests will produce an executable used in jormungandr integration tests

add_executable(main_routing_test main_routing_test.cpp)

add_executable(main_ptref_test main_ptref_test.cpp)

add_executable(basic_routing_test basic_routing_test.cpp)

add_executable(basic_schedule_test basic_schedule_test.cpp)

add_executable(departure_board_test departure_board_test.cpp)

add_executable(empty_routing_test empty_routing_test.cpp)

add_executable(main_autocomplete_test main_autocomplete_test.cpp)

add_executable(null_status_test null_status_test.cpp)

add_executable(main_routing_without_pt_test main_routing_without_pt_test.cpp)


target_link_libraries(main_routing_test ${ALL_LIBS})
target_link_libraries(main_ptref_test ${ALL_LIBS})
target_link_libraries(basic_routing_test ${ALL_LIBS})
target_link_libraries(basic_schedule_test ${ALL_LIBS})
target_link_libraries(departure_board_test ${ALL_LIBS})
target_link_libraries(empty_routing_test ${ALL_LIBS})
target_link_libraries(main_autocomplete_test ${ALL_LIBS})
target_link_libraries(null_status_test ${ALL_LIBS})
target_link_libraries(main_routing_without_pt_test ${ALL_LIBS})
