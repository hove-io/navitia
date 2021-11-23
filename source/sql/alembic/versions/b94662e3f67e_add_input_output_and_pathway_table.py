"""add input_output and pathway table

Revision ID: b94662e3f67e
Revises: 2c69b03d20fc
Create Date: 2021-11-22 11:03:42.036716

"""

# revision identifiers, used by Alembic.
revision = 'b94662e3f67e'
down_revision = '2c69b03d20fc'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.create_table(
        'input_output',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('uri', sa.TEXT(), nullable=False),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.Column('coord', ga.Geography(geometry_type='POINT', srid=4326, spatial_index=False), nullable=True),
        sa.Column('stop_code', sa.TEXT(), nullable=True),
        sa.Column('parent_station', sa.TEXT(), nullable=True),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'pathway',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('uri', sa.TEXT(), nullable=False),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.Column('from_stop_id', sa.TEXT(), nullable=False),
        sa.Column('to_stop_id', sa.TEXT(), nullable=False),
        sa.Column('pathway_mode', sa.INTEGER(), nullable=False),
        sa.Column('is_bidirectional', sa.BOOLEAN(), nullable=False),
        sa.Column('length', sa.INTEGER(), nullable=True),
        sa.Column('traversal_time', sa.INTEGER(), nullable=True),
        sa.Column('stair_count', sa.INTEGER(), nullable=True),
        sa.Column('max_slope', sa.INTEGER(), nullable=True),
        sa.Column('min_width', sa.INTEGER(), nullable=True),
        sa.Column('signposted_as', sa.TEXT(), nullable=True),
        sa.Column('reversed_signposted_as', sa.TEXT(), nullable=True),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )


def downgrade():
    op.drop_table('input_output', schema='navitia')
    op.drop_table('pathway', schema='navitia')
