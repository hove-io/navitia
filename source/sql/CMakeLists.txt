INSTALL_FILES(/usr/share/navitia/ed/alembic FILES alembic.ini alembic/env.py)

FILE(GLOB
    alembic_ed
    alembic/versions/*.py
    )
INSTALL_FILES(/usr/share/navitia/ed/alembic/versions FILES ${alembic_ed})
