"""Adding argument taxi_speed for journey

Revision ID: 16d8bca1453b
Revises: 52d628e535db
Create Date: 2019-03-26 17:18:53.868067

"""

# revision identifiers, used by Alembic.
revision = '16d8bca1453b'
down_revision = '52d628e535db'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('instance', sa.Column('taxi_speed', sa.Float(), nullable=False, server_default='11.11'))


def downgrade():
    op.drop_column('instance', 'taxi_speed')
