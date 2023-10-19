"""Add obstacles in navitia service type

Revision ID: 1094ff8c5975
Revises: cab8323f71bd
Create Date: 2023-10-19 17:58:27.149354

"""

# revision identifiers, used by Alembic.
revision = '1094ff8c5975'
down_revision = 'cab8323f71bd'

from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects import postgresql

def upgrade():
    op.execute("COMMIT")  # See https://bitbucket.org/zzzeek/alembic/issue/123
    op.execute("ALTER TYPE navitia_service_type ADD VALUE 'obstacles'")


def downgrade():
    op.execute(
        "DELETE FROM associate_instance_external_service WHERE external_service_id in (SELECT ID FROM external_service WHERE navitia_service = 'obstacles')"
    )
    op.execute("DELETE FROM external_service WHERE navitia_service = 'obstacles'")
    op.execute("ALTER TABLE external_service ALTER COLUMN navitia_service TYPE text")
    op.execute("DROP TYPE navitia_service_type CASCADE")
    op.execute(
        "CREATE TYPE navitia_service_type AS ENUM ('free_floatings', 'vehicle_occupancies', 'realtime_proxies', 'vehicle_positions', 'bss_stations')"
    )
    op.execute(
        "ALTER TABLE external_service ALTER COLUMN navitia_service TYPE navitia_service_type USING navitia_service::navitia_service_type"
    )
