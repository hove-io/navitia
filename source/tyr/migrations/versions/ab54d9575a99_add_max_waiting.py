"""max_waiting added in instance

Revision ID: ab54d9575a99
Revises: 43b19d690da2
Create Date: 2021-04-02 16:27:20.489654

"""

# revision identifiers, used by Alembic.
revision = 'ab54d9575a99'
down_revision = '43b19d690da2'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('instance', sa.Column('max_waiting', sa.Integer(), server_default='14400', nullable=False))


def downgrade():
    op.drop_column('instance', 'max_waiting')
