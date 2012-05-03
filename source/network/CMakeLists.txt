FIND_PACKAGE(Boost 1.40.0 COMPONENTS serialization date_time regex thread iostreams REQUIRED)

SET(NETWORK_SRC network.cpp)
add_library(network ${NETWORK_SRC})
target_link_libraries(network ${Boost_LIBRARIES} types)

add_executable(test_n test.cpp)
target_link_libraries(test_n ${Boost_LIBRARIES} types network)
