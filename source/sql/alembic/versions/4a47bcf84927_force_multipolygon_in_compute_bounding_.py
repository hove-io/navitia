"""Force multipolygon in compute_bounding_shape

Revision ID: 4a47bcf84927
Revises: 45719cfc11f9
Create Date: 2014-10-29 17:52:59.309392

"""

# revision identifiers, used by Alembic.
revision = '4a47bcf84927'
down_revision = '45719cfc11f9'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.execute(
        """CREATE OR REPLACE FUNCTION georef.compute_bounding_shape() RETURNS GEOMETRY AS $$
                SELECT ST_Multi(ST_ConvexHull(ST_Collect(ARRAY(select coord::geometry from georef.node))))
                $$
                LANGUAGE SQL;"""
    )


def downgrade():
    op.execute(
        """CREATE OR REPLACE FUNCTION georef.compute_bounding_shape() RETURNS GEOMETRY AS $$
                SELECT ST_ConvexHull(ST_Collect(ARRAY(select coord::geometry from georef.node)))
                $$
                LANGUAGE SQL;"""
    )
