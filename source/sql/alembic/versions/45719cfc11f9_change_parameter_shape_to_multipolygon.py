"""change parameter.shape to MULTIPOLYGON

Revision ID: 45719cfc11f9
Revises: 40d9139ba0ed
Create Date: 2014-10-24 18:43:27.550538

"""

# revision identifiers, used by Alembic.
revision = '45719cfc11f9'
down_revision = '40d9139ba0ed'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga
from sqlalchemy.dialects import postgresql


def upgrade():
    op.execute(
        """
    ALTER TABLE navitia.parameters ALTER COLUMN shape
    SET DATA TYPE GEOGRAPHY(MULTIPOLYGON, 4326) USING ST_Multi(shape::geometry)
    """
    )


def downgrade():
    op.execute(
        """
    ALTER TABLE navitia.parameters ALTER COLUMN shape
    SET DATA TYPE GEOGRAPHY(POLYGON, 4326) USING ST_GeometryN(shape::geometry, 1)
    """
    )
