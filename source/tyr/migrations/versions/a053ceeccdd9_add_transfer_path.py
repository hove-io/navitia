"""

Add transfer_path to enable omputing pathways using the street_network engine for transfers between surface physical modes

Revision ID: a053ceeccdd9
Revises: 7f757939ace8
Create Date: 2021-07-30 15:05:45.751107

"""

# revision identifiers, used by Alembic.
revision = 'a053ceeccdd9'
down_revision = '7f757939ace8'

from alembic import op
import sqlalchemy as sa

from navitiacommon import default_values


def upgrade():
    op.add_column(
        'instance',
        sa.Column('transfer_path', sa.Boolean(), server_default=default_values.transfer_path, nullable=False),
    )


def downgrade():
    op.drop_column('instance', 'transfer_path')
