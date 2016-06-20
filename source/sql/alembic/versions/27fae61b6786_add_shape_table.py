"""add a table for shape betwenn stop_time

Revision ID: 27fae61b6786
Revises: 4f4ee492129f
Create Date: 2016-06-20 11:45:07.092619

"""

# revision identifiers, used by Alembic.
revision = '27fae61b6786'
down_revision = '4f4ee492129f'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.create_table('shape',
    sa.Column('id', sa.BIGINT(), nullable=False),
    sa.Column('geom', ga.Geography(geometry_type='LINESTRING', srid=4326, spatial_index=False), nullable=True),
    sa.PrimaryKeyConstraint('id'),
    schema='navitia'
    )
    op.drop_column('stop_time', 'shape_from_prev', schema='navitia')
    op.add_column('stop_time', sa.Column('shape_from_prev_id', sa.BIGINT(), nullable=True), schema='navitia')
    op.create_foreign_key('fk_stop_time_shape', 'stop_time', 'shape', ['shape_from_prev_id'], ['id'], referent_schema='navitia', source_schema='navitia')


def downgrade():
    op.drop_column('stop_time', 'shape_from_prev_id', schema='navitia')
    op.add_column('stop_time', sa.Column('shape_from_prev', ga.Geography(geometry_type='LINESTRING', srid=4326, spatial_index=False), nullable=True), schema='navitia')
    op.drop_table('shape', schema='navitia')
