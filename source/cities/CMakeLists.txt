SET(BOOST_LIBS ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY} ${Boost_SERIALIZATION_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_REGEX_LIBRARY})

add_executable(cities cities.cpp)
target_link_libraries(cities transportation_data_import connectors types pq pqxx ${OSMPBF} pb_lib utils
    ${BOOST_LIBS} log4cplus z)
