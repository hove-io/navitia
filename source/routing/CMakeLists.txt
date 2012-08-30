SET(ROUTING_SRC routing.cpp astar.cpp time_dependent.cpp raptor.cpp benchmark.cpp raptor_api.cpp)

add_library(routing ${ROUTING_SRC})
target_link_libraries(routing types data)

add_executable(single_run single_run.cpp)
target_link_libraries(single_run routing boost_program_options)

add_executable(testbench test_benchmark.cpp)
target_link_libraries(testbench routing)

add_executable(testartemis test_artemis.cpp)
target_link_libraries(testartemis routing connectors)

add_subdirectory(tests)
