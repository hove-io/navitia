"""add full_sn_geometries on instances for managing geometries in ed2nav

Revision ID: 3131d5bf5570
Revises: 162b4fa5836a
Create Date: 2016-08-22 13:55:00.870197

"""

# revision identifiers, used by Alembic.
revision = '3131d5bf5570'
down_revision = '162b4fa5836a'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('instance', sa.Column('full_sn_geometries', sa.Boolean(), server_default='false', nullable=False))


def downgrade():
    op.drop_column('instance', 'full_sn_geometries')
