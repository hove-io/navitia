set(ALL_LIBS workers ed disruption_api calendar_api time_tables types autocomplete proximitylist
    ptreferential time_tables data routing fare types georef utils SimpleAmqpClient
    rabbitmq-static log4cplus ${Boost_LIBRARIES} protobuf ${NAVITIA_ALLOCATOR})

# every tests will produce an executable used in jormungandr integration tests

add_executable(main_routing_test main_routing_test.cpp)
add_executable(main_ptref_test main_ptref_test.cpp)
add_executable(min_nb_journeys_test min_nb_journeys_test.cpp)
add_executable(basic_routing_test basic_routing_test.cpp)
add_executable(basic_schedule_test basic_schedule_test.cpp)
add_executable(departure_board_test departure_board_test.cpp)
add_executable(empty_routing_test empty_routing_test.cpp)
add_executable(main_autocomplete_test main_autocomplete_test.cpp)
add_executable(main_stif_test main_stif_test.cpp)
add_executable(null_status_test null_status_test.cpp)
add_executable(main_routing_without_pt_test main_routing_without_pt_test.cpp)
add_executable(multiple_schedules multiple_schedules.cpp)
add_executable(line_sections_test line_sections_test.cpp)
add_executable(timezone_cape_verde_test timezone_cape_verde_test.cpp)
add_executable(timezone_hong_kong_test timezone_hong_kong_test.cpp)

#every test must be added to this target
add_custom_target(integration_tests_bin DEPENDS main_routing_test
                                                main_ptref_test
                                                min_nb_journeys_test
                                                basic_routing_test
                                                basic_schedule_test
                                                departure_board_test
                                                empty_routing_test
                                                main_autocomplete_test
                                                main_stif_test
                                                null_status_test
                                                main_routing_without_pt_test
                                                multiple_schedules
                                                line_sections_test
                                                timezone_cape_verde_test
                                                timezone_hong_kong_test)


target_link_libraries(main_routing_test ${ALL_LIBS})
target_link_libraries(main_ptref_test ${ALL_LIBS})
target_link_libraries(min_nb_journeys_test ${ALL_LIBS})
target_link_libraries(basic_routing_test ${ALL_LIBS})
target_link_libraries(basic_schedule_test ${ALL_LIBS})
target_link_libraries(departure_board_test ${ALL_LIBS})
target_link_libraries(empty_routing_test ${ALL_LIBS})
target_link_libraries(main_autocomplete_test ${ALL_LIBS})
target_link_libraries(main_stif_test ${ALL_LIBS})
target_link_libraries(null_status_test ${ALL_LIBS})
target_link_libraries(main_routing_without_pt_test ${ALL_LIBS})
target_link_libraries(multiple_schedules ${ALL_LIBS})
target_link_libraries(line_sections_test ${ALL_LIBS})
target_link_libraries(timezone_cape_verde_test ${ALL_LIBS})
target_link_libraries(timezone_hong_kong_test ${ALL_LIBS})
