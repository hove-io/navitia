"""shape in journey pattern point

Revision ID: 5812ecbcd76d
Revises: 52b017632678
Create Date: 2014-09-30 12:34:02.493910

"""

# revision identifiers, used by Alembic.
revision = '5812ecbcd76d'
down_revision = '52b017632678'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.add_column(
        'journey_pattern_point',
        sa.Column(
            'shape_from_prev',
            ga.Geography(geometry_type='LINESTRING', srid=4326, spatial_index=False),
            nullable=True,
        ),
        schema='navitia',
    )


def downgrade():
    op.drop_column('journey_pattern_point', 'shape_from_prev', schema='navitia')
