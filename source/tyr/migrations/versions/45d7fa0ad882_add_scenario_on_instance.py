"""add the scenario column on the instance table

Revision ID: 45d7fa0ad882
Revises: 12aa2de68f7
Create Date: 2014-10-08 14:34:46.038194

"""

# revision identifiers, used by Alembic.
revision = '45d7fa0ad882'
down_revision = '12aa2de68f7'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('instance', sa.Column('scenario', sa.Text(), nullable=False, server_default='default'))


def downgrade():
    op.drop_column('instance', 'scenario')
