SET(ROUTING_SRC routing.cpp astar.cpp time_dependent.cpp time_expanded.cpp raptor.cpp benchmark.cpp mcraptor.cpp raptor_api.cpp)

add_library(routing ${ROUTING_SRC})
target_link_libraries(routing types data)

add_executable(test_time_dependent time_dependent_test.cpp)
target_link_libraries(test_time_dependent routing)

add_executable(test_time_expanded time_expanded_test.cpp)
target_link_libraries(test_time_expanded routing)

add_executable(testraptor test_raptor.cpp)
target_link_libraries(testraptor routing connectors)

add_executable(testbench test_benchmark.cpp)
target_link_libraries(testbench routing)


add_executable(testartemis test_artemis.cpp)
target_link_libraries(testartemis routing connectors)

add_subdirectory(tests)
