"""
    Settings for parallel calls to real-time proxies : realtime_pool_size
    - defines the number of greenlets to use

Revision ID: 242138b08c92
Revises: 3acdde6329db
Create Date: 2018-02-15 11:42:01.190624

"""

# revision identifiers, used by Alembic.
revision = '242138b08c92'
down_revision = '3acdde6329db'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('instance', sa.Column('realtime_pool_size', sa.Integer(), nullable=True))

def downgrade():
    op.drop_column('instance', 'realtime_pool_size')
