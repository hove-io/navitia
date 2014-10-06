INSTALL_FILES(/usr/share/navitia/ed/script FILES alembic.ini alembic/env.py)
INSTALL_FILES(/usr/share/inavitia/ed/sql FILES ed/02-migration.sql)

INSTALL_FILES(/usr/share/navitia/ed/alembic FILES alembic.ini alembic/env.py)

FILE(GLOB
    alembic_ed
    alembic/versions/*.py
    )
INSTALL_FILES(/usr/share/navitia/ed/alembic/versions FILES ${alembic_ed})
