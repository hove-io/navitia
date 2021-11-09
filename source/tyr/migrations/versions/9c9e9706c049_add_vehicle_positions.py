"""Add vehicle_positions in external_service table

Revision ID: 9c9e9706c049
Revises: a053ceeccdd9
Create Date: 2021-09-16 10:56:49.026201

"""

# revision identifiers, used by Alembic.
revision = '9c9e9706c049'
down_revision = 'a053ceeccdd9'

from alembic import op


def upgrade():
    op.execute("COMMIT")  # See https://bitbucket.org/zzzeek/alembic/issue/123
    op.execute("ALTER TYPE navitia_service_type ADD VALUE 'vehicle_positions'")


def downgrade():
    op.execute("ALTER TABLE external_service ALTER COLUMN navitia_service TYPE text")
    op.execute("DROP TYPE navitia_service_type CASCADE")
    op.execute(
        "CREATE TYPE navitia_service_type AS ENUM ( 'free_floatings', 'vehicle_occupancies', 'realtime_proxies')"
    )
    op.execute(
        "ALTER TABLE external_service ALTER COLUMN navitia_service TYPE navitia_service_type USING navitia_service::navitia_service_type"
    )
