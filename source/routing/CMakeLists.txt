SET(ROUTING_SRC routing.cpp astar.cpp time_dependent.cpp time_expanded.cpp raptor.cpp)
add_library(routing ${ROUTING_SRC})
target_link_libraries(routing types)

add_executable(test_time_dependent time_dependent_test.cpp)
target_link_libraries(test_time_dependent routing)

add_executable(test_time_expanded time_expanded_test.cpp)
target_link_libraries(test_time_expanded routing)

add_executable(testraptor test_raptor.cpp)
target_link_libraries(testraptor routing)


add_subdirectory(tests)
