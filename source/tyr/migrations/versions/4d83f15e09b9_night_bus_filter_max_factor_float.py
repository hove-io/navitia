"""Fix default type of night_bus_filter_max_factor

Revision ID: 4d83f15e09b9
Revises: cf2becf2dc1
Create Date: 2016-02-23 16:22:59.544317

"""

# revision identifiers, used by Alembic.
revision = '4d83f15e09b9'
down_revision = 'cf2becf2dc1'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.alter_column('instance', 'night_bus_filter_max_factor',
               existing_type=sa.Integer(), type_=sa.Float())


def downgrade():
    op.alter_column('instance', 'night_bus_filter_max_factor',
               existing_type=sa.Float(), type_=sa.Integer())
