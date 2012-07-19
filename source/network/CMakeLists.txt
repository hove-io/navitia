SET(NETWORK_SRC network.cpp)
add_library(network ${NETWORK_SRC})
target_link_libraries(network types)

add_executable(test_n test.cpp)
target_link_libraries(test_n network utils)
