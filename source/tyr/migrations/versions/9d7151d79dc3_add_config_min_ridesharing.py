"""
Add attribute min_ridesharing in instance

Revision ID: 9d7151d79dc3
Revises: 3cf7f605e9ac
Create Date: 2020-11-04 10:45:18.768646

"""

# revision identifiers, used by Alembic.
revision = '9d7151d79dc3'
down_revision = '3cf7f605e9ac'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('instance', sa.Column('min_ridesharing', sa.Integer(), nullable=False, server_default='600'))


def downgrade():
    op.drop_column('instance', 'min_ridesharing')
