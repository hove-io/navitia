set(RAPTOR_LINK_LIBS
    routing
    ed
    workers
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${NAVITIA_ALLOCATOR}
)
add_executable(raptor_test raptor_test.cpp)
target_link_libraries(raptor_test ${RAPTOR_LINK_LIBS})
ADD_BOOST_TEST(raptor_test)

add_executable(reverse_raptor_test reverse_raptor_test.cpp)
target_link_libraries(reverse_raptor_test ${RAPTOR_LINK_LIBS})
ADD_BOOST_TEST(reverse_raptor_test)

add_executable(frequency_raptor_test frequency_raptor_test.cpp)
target_link_libraries(frequency_raptor_test ${RAPTOR_LINK_LIBS})
ADD_BOOST_TEST(frequency_raptor_test)

add_executable(routing_api_test routing_api_test.cpp)
target_link_libraries(routing_api_test ${RAPTOR_LINK_LIBS})
ADD_BOOST_TEST(routing_api_test)

add_executable(next_stop_time_test next_stop_time_test.cpp)
target_link_libraries(next_stop_time_test ${RAPTOR_LINK_LIBS})
ADD_BOOST_TEST(next_stop_time_test)

add_executable(disruptions_test disruptions_test.cpp)
target_link_libraries(disruptions_test ${RAPTOR_LINK_LIBS} ${Boost_PROGRAM_OPTIONS_LIBRARY})
ADD_BOOST_TEST(disruptions_test)

add_executable(co2_emission_test co2_emission_test.cpp)
target_link_libraries(co2_emission_test ${RAPTOR_LINK_LIBS})
ADD_BOOST_TEST(co2_emission_test)

add_executable(journey_pattern_container_test journey_pattern_container_test.cpp)
target_link_libraries(journey_pattern_container_test ${RAPTOR_LINK_LIBS})
ADD_BOOST_TEST(journey_pattern_container_test)

add_executable(get_stop_times_test get_stop_times_test.cpp)
target_link_libraries(get_stop_times_test ${RAPTOR_LINK_LIBS})
ADD_BOOST_TEST(get_stop_times_test)

add_executable(isochrone_test isochrone_test.cpp)
target_link_libraries(isochrone_test ${RAPTOR_LINK_LIBS})
ADD_BOOST_TEST(isochrone_test)

add_executable(heat_map_test heat_map_test.cpp)
target_link_libraries(heat_map_test ${RAPTOR_LINK_LIBS})
ADD_BOOST_TEST(heat_map_test)

add_executable(journey_test journey_test.cpp)
target_link_libraries(journey_test ${RAPTOR_LINK_LIBS})
ADD_BOOST_TEST(journey_test)
