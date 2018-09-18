install(FILES alembic.ini alembic/env.py DESTINATION /usr/share/navitia/ed/script)
install(FILES ed/02-migration.sql DESTINATION /usr/share/navitia/ed/sql)

install(FILES alembic.ini alembic/env.py DESTINATION /usr/share/navitia/ed/alembic)

FILE(GLOB
    alembic_ed
    alembic/versions/*.py
    )
install(FILES ${alembic_ed} DESTINATION /usr/share/navitia/ed/alembic/versions)
