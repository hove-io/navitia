FIND_PACKAGE(Boost 1.40.0 COMPONENTS program_options filesystem system serialization date_time thread regex REQUIRED)

SET(ROUTING_SRC routing.cpp raptor_init.cpp raptor_path.cpp raptor.cpp raptor_api.cpp best_trip.cpp dataraptor.cpp raptor_utils.cpp)

add_library(routing ${ROUTING_SRC})

add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark routing boost_program_options data routing georef utils ${Boost_LIBRARIES})

add_subdirectory(tests)
