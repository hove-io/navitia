SET(NETWORK_SRC network.cpp)
add_library(network ${NETWORK_SRC})
target_link_libraries(network types)

add_executable(test_n test.cpp)
target_link_libraries(test_n network utils)

add_executable(test_time_dependent time_dependent_test.cpp time_dependent.cpp)
target_link_libraries(test_time_dependent types)
