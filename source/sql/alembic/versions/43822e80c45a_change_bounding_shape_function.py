"""

Remove the upsert for the bounding shape computation, since the computation was done even if the
shape_computed was false


Revision ID: 43822e80c45a
Revises: 4a47bcf84927
Create Date: 2014-11-25 11:18:52.755484

"""

# revision identifiers, used by Alembic.
revision = '43822e80c45a'
down_revision = '4a47bcf84927'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.execute(
        """
    CREATE OR REPLACE FUNCTION georef.update_bounding_shape() RETURNS VOID AS $$
    INSERT INTO navitia.parameters (shape) SELECT NULL WHERE NOT EXISTS (SELECT * FROM navitia.parameters);

    UPDATE navitia.parameters SET shape = (SELECT georef.compute_bounding_shape()) WHERE shape_computed;
    $$
    LANGUAGE SQL;
    """
    )


def downgrade():
    # we put back the old function

    op.execute(
        """
    CREATE OR REPLACE FUNCTION georef.update_bounding_shape() RETURNS VOID AS $$
    WITH upsert as (UPDATE navitia.parameters SET shape = (SELECT georef.compute_bounding_shape()) WHERE shape_computed RETURNING *)
    INSERT INTO navitia.parameters (shape) SELECT (SELECT georef.compute_bounding_shape()) WHERE NOT EXISTS (SELECT * FROM upsert);
    $$
    LANGUAGE SQL;
    """
    )
