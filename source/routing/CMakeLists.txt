SET(ROUTING_SRC routing.cpp time_dependent.cpp raptor.cpp raptor_api.cpp best_trip.cpp dataraptor.cpp)

add_library(routing ${ROUTING_SRC})

add_executable(single_run single_run.cpp)
target_link_libraries(single_run routing boost_program_options routing)

add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark routing boost_program_options routing)

add_executable(testartemis test_artemis.cpp)
target_link_libraries(testartemis routing connectors routing)

add_subdirectory(tests)
