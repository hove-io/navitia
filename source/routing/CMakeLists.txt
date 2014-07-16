SET(BOOST_LIBS ${Boost_THREAD_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_REGEX_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_SERIALIZATION_LIBRARY} ${Boost_FILESYSTEM_LIBRARY})

SET(ROUTING_SRC routing.cpp raptor_solutions.cpp raptor_path.cpp raptor.cpp raptor_api.cpp best_stoptime.cpp dataraptor.cpp raptor_utils.cpp)

add_library(routing ${ROUTING_SRC})

add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark routing boost_program_options data fare routing
    georef utils autocomplete ${BOOST_LIBS} log4cplus)

add_executable(standalone single_run.cpp)
target_link_libraries(standalone routing boost_program_options data fare routing thermometer time_tables pb_lib
    georef utils autocomplete ${BOOST_LIBS} log4cplus pthread)
add_subdirectory(tests)
