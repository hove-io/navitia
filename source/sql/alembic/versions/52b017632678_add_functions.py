"""Add functions

Revision ID: 52b017632678
Revises: 90f4275525
Create Date: 2014-09-24 17:30:36.923867

"""

# revision identifiers, used by Alembic.
revision = '52b017632678'
down_revision = '90f4275525'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.execute(
        """
    CREATE OR REPLACE FUNCTION georef.compute_bounding_shape() RETURNS GEOMETRY AS $$
        SELECT ST_ConvexHull(ST_Collect(ARRAY(select coord::geometry from georef.node)))
    $$
    LANGUAGE SQL;
    """
    )
    op.execute(
        """
    CREATE OR REPLACE FUNCTION georef.update_bounding_shape() RETURNS VOID AS $$
    WITH upsert as (UPDATE navitia.parameters SET shape = (SELECT georef.compute_bounding_shape()) WHERE shape_computed RETURNING *)
    INSERT INTO navitia.parameters (shape) SELECT (SELECT georef.compute_bounding_shape()) WHERE NOT EXISTS (SELECT * FROM upsert);
    $$
    LANGUAGE SQL;
    """
    )


def downgrade():
    op.execute("DROP FUNCTION georef.update_bounding_shape();")
    op.execute("DROP FUNCTION georef.compute_bounding_shape();")
