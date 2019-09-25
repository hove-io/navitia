install(FILES alembic.ini alembic/env.py DESTINATION ${CMAKE_INSTALL_PREFIX}/share/navitia/ed/script)

FILE(GLOB
    alembic_ed
    alembic/versions/*.py
    )
install(FILES ${alembic_ed} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/navitia/ed/alembic/versions)
