SET(ROUTING_SRC routing.cpp time_dependent.cpp)
add_library(routing ${ROUTING_SRC})
target_link_libraries(routing types)

add_executable(test_time_dependent time_dependent_test.cpp)
target_link_libraries(test_time_dependent routing)

add_subdirectory(tests)
