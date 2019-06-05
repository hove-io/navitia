SET(BOOST_LIBS ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY} )

add_executable(cities cities.cpp)
target_link_libraries(cities transportation_data_import connectors types ${PQXX_LIB} ${OSMPBF} pb_lib utils
    ${BOOST_LIBS} log4cplus z protobuf ${NAVITIA_ALLOCATOR})
install(TARGETS cities DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(FILES alembic.ini alembic/env.py DESTINATION ${CMAKE_INSTALL_PREFIX}/share/navitia/cities/alembic)

FILE(GLOB
    alembic_cities
    alembic/versions/*.py
    )
install(FILES ${alembic_cities} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/navitia/cities/alembic/versions)
