"""Add car_parkings in navitia_service_type

Revision ID: 3f42a8407b93
Revises: 1094ff8c5975
Create Date: 2023-12-06 15:43:34.661720

"""

# revision identifiers, used by Alembic.
revision = '3f42a8407b93'
down_revision = '1094ff8c5975'

from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects import postgresql

def upgrade():
    op.execute("COMMIT")  # See https://bitbucket.org/zzzeek/alembic/issue/123
    op.execute("ALTER TYPE navitia_service_type ADD VALUE 'car_parkings'")


def downgrade():
    op.execute(
        "DELETE FROM associate_instance_external_service WHERE external_service_id in (SELECT ID FROM external_service WHERE navitia_service = 'car_parkings')"
    )
    op.execute("DELETE FROM external_service WHERE navitia_service = 'car_parkings'")
    op.execute("ALTER TABLE external_service ALTER COLUMN navitia_service TYPE text")
    op.execute("DROP TYPE navitia_service_type CASCADE")
    op.execute(
        "CREATE TYPE navitia_service_type AS ENUM ('free_floatings', 'vehicle_occupancies', 'realtime_proxies', 'vehicle_positions', 'bss_stations', 'obstacles')"
    )
    op.execute(
        "ALTER TABLE external_service ALTER COLUMN navitia_service TYPE navitia_service_type USING navitia_service::navitia_service_type"
    )
