"""Add bss_stations in navitia_service_type

Revision ID: cab8323f71bd
Revises: 20d8caa23b26
Create Date: 2023-09-06 14:19:03.579738

"""

# revision identifiers, used by Alembic.
revision = 'cab8323f71bd'
down_revision = '20d8caa23b26'

from alembic import op


def upgrade():
    op.execute("COMMIT")  # See https://bitbucket.org/zzzeek/alembic/issue/123
    op.execute("ALTER TYPE navitia_service_type ADD VALUE 'bss_stations'")


def downgrade():
    op.execute(
        "DELETE FROM associate_instance_external_service WHERE external_service_id in (SELECT ID FROM external_service WHERE navitia_service = 'bss_stations')"
    )
    op.execute("DELETE FROM external_service WHERE navitia_service = 'bss_stations'")
    op.execute("ALTER TABLE external_service ALTER COLUMN navitia_service TYPE text")
    op.execute("DROP TYPE navitia_service_type CASCADE")
    op.execute(
        "CREATE TYPE navitia_service_type AS ENUM ('free_floatings', 'vehicle_occupancies', 'realtime_proxies', 'vehicle_positions')"
    )
    op.execute(
        "ALTER TABLE external_service ALTER COLUMN navitia_service TYPE navitia_service_type USING navitia_service::navitia_service_type"
    )
