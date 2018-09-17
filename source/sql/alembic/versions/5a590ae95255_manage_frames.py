"""manage frames

Revision ID: 5a590ae95255
Revises: 59f4456a029
Create Date: 2015-11-25 16:43:10.104442

"""

# revision identifiers, used by Alembic.
revision = '5a590ae95255'
down_revision = '14346346596e'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.create_table(
        'frame',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('uri', sa.TEXT(), nullable=False),
        sa.Column('description', sa.TEXT(), nullable=True),
        sa.Column('system', sa.TEXT(), nullable=True),
        sa.Column('start_date', sa.DATE(), nullable=False),
        sa.Column('end_date', sa.DATE(), nullable=False),
        sa.Column('contributor_id', sa.BIGINT(), nullable=False),
        sa.ForeignKeyConstraint(['contributor_id'], [u'navitia.contributor.id'], name=u'contributor_frame_fkey'),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.add_column('vehicle_journey', sa.Column('frame_id', sa.BIGINT(), nullable=True), schema='navitia')


def downgrade():
    op.drop_column('vehicle_journey', 'frame_id', schema='navitia')
    op.drop_table('frame', schema='navitia')
