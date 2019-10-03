"""add station entrances

Revision ID: 73f0662786c7
Revises: 25b7f3ace052
Create Date: 2019-10-02 14:42:15.195754

"""

# revision identifiers, used by Alembic.
revision = '73f0662786c7'
down_revision = '25b7f3ace052'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():

    op.create_table(
        'entrance',
        sa.Column('id', sa.BIGINT(), sa.Sequence('entrance_id_seq'), nullable=False, primary_key=True),
        sa.Column('uri', sa.TEXT(), nullable=False),
        sa.Column('code', sa.TEXT(), nullable=True),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.Column('stop_area_id', sa.BIGINT(), nullable=False),
        sa.Column('coord', ga.Geography(geometry_type='POINT', srid=4326, spatial_index=False), nullable=True),
        sa.ForeignKeyConstraint(['stop_area_id'], [u'navitia.stop_area.id'], name=u'entrance_stop_area_id_fkey'),
        schema='navitia',
    )


def downgrade():
    op.drop_table('entrance', schema='navitia')
