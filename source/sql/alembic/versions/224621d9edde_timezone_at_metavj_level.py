"""timezone at metavj level

Revision ID: 224621d9edde
Revises: 14346346596e
Create Date: 2015-12-21 16:52:30.275508

"""

# revision identifiers, used by Alembic.
revision = '224621d9edde'
down_revision = '5a590ae95255'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.create_table(
        'timezone',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'tz_dst',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('tz_id', sa.BIGINT(), nullable=False),
        sa.Column('beginning_date', sa.DATE(), nullable=False),
        sa.Column('end_date', sa.DATE(), nullable=False),
        sa.Column('utc_offset', sa.INTEGER(), nullable=False),
        sa.ForeignKeyConstraint(['tz_id'], [u'navitia.timezone.id'], name=u'associated_tz_dst_fkey'),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.add_column(u'meta_vj', sa.Column('timezone', sa.BIGINT(), nullable=True), schema=u'navitia')
    op.drop_column(u'vehicle_journey', 'utc_to_local_offset', schema=u'navitia')


def downgrade():
    op.drop_column(u'meta_vj', 'timezone', schema=u'navitia')
    op.drop_table('tz_dst', schema='navitia')
    op.drop_table('timezone', schema='navitia')
    op.add_column(
        u'vehicle_journey', sa.Column('utc_to_local_offset', sa.BIGINT(), nullable=True), schema=u'navitia'
    )
