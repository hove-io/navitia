SET(BOOST_LIBS ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY} )

add_executable(cities cities.cpp)
target_link_libraries(cities transportation_data_import connectors types ${PQXX_LIB} ${OSMPBF} pb_lib utils
    ${BOOST_LIBS} log4cplus z protobuf)
INSTALL_TARGETS(/usr/bin/ cities)

INSTALL_FILES(/usr/share/navitia/cities/alembic FILES alembic.ini alembic/env.py)

FILE(GLOB
    alembic_cities
    alembic/versions/*.py
    )
INSTALL_FILES(/usr/share/navitia/cities/alembic/versions FILES ${alembic_cities})
