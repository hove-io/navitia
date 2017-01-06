"""add default to import stops

Revision ID: 132cbd5a61a4
Revises: 361f3993960e
Create Date: 2017-01-05 11:51:08.188833

"""

# revision identifiers, used by Alembic.
revision = '132cbd5a61a4'
down_revision = '361f3993960e'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.alter_column('instance', 'import_stops_in_mimir', server_default='false')


def downgrade():
    op.alter_column('instance', 'import_stops_in_mimir', server_default=None)
