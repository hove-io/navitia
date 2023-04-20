"""Add new column: poi_access_points

Revision ID: c7010efdcf62
Revises: 04f9b89e3ef5
Create Date: 2023-04-20 15:47:51.298091

"""

# revision identifiers, used by Alembic.
revision = 'c7010efdcf62'
down_revision = '04f9b89e3ef5'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('instance', sa.Column('poi_access_points', sa.Boolean(), server_default='False', nullable=False))


def downgrade():
    op.drop_column('instance', 'poi_access_points')
