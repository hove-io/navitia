"""Add areas in stop points

Revision ID: 29fc422c56cb
Revises: 23dd34bfaeaf
Create Date: 2015-04-13 17:07:36.827493

"""

# revision identifiers, used by Alembic.
revision = '29fc422c56cb'
down_revision = '23dd34bfaeaf'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.add_column(
        'stop_point',
        sa.Column('is_zonal', sa.BOOLEAN(), primary_key=False, nullable=False, server_default='false'),
        schema='navitia',
    )
    op.add_column(
        'stop_point',
        sa.Column(
            'area',
            ga.Geography(geometry_type='MULTIPOLYGON', srid=4326, spatial_index=False),
            primary_key=False,
            nullable=True,
        ),
        schema='navitia',
    )


def downgrade():
    op.drop_column('stop_point', 'is_zonal', schema='navitia')
    op.drop_column('stop_point', 'area', schema='navitia')
