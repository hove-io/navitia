FIND_PACKAGE(Boost 1.40.0 COMPONENTS graph serialization unit_test_framework REQUIRED)

SET(RAPTOR_SRC raptor.cpp)
add_library(raptor ${RAPTOR_SRC})
target_link_libraries(raptor types)

add_executable (test_raptor test.cpp)
target_link_libraries(test_raptor ${Boost_LIBRARIES} raptor utils  types)
