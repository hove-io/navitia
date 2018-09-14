"""create table

Revision ID: 15e13307464d
Revises: None
Create Date: 2014-09-11 15:26:23.309779

"""

# revision identifiers, used by Alembic.
revision = '15e13307464d'
down_revision = None

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.create_table(
        'administrative_regions',
        sa.Column('id', sa.BIGINT, primary_key=True),
        sa.Column('name', sa.TEXT, nullable=False),
        sa.Column('uri', sa.TEXT, nullable=False),
        sa.Column('post_code', sa.TEXT),
        sa.Column('insee', sa.TEXT),
        sa.Column('level', sa.INTEGER),
        sa.Column('coord', ga.Geography(geometry_type='POINT', srid=4326, spatial_index=False)),
        sa.Column('boundary', ga.Geography(geometry_type='MULTIPOLYGON', srid=4326, spatial_index=False)),
    )


def downgrade():
    op.drop_table('administrative_regions')
