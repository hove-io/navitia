SET(ROUTING_SRC routing.cpp time_dependent.cpp raptor.cpp raptor_api.cpp best_trip.cpp)

add_library(dataraptor raptor_utils.h dataraptor.cpp)
target_link_libraries(dataraptor types data)

add_library(routing ${ROUTING_SRC})
target_link_libraries(routing types data dataraptor)

add_executable(single_run single_run.cpp)
target_link_libraries(single_run routing boost_program_options dataraptor)

add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark routing boost_program_options dataraptor)

add_executable(testartemis test_artemis.cpp)
target_link_libraries(testartemis routing connectors dataraptor)

add_subdirectory(tests)
