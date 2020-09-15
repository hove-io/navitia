"""add asynchronous ridesharing

Revision ID: 0ccb5413baa9
Revises: b413ec1e3bbc
Create Date: 2020-09-04 11:23:13.637259

"""

# revision identifiers, used by Alembic.
revision = '0ccb5413baa9'
down_revision = 'b413ec1e3bbc'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column(
        'instance',
        sa.Column('asynchronous_ridesharing', sa.Boolean(), nullable=False, server_default=sa.false()),
    )


def downgrade():
    op.drop_column('instance', 'asynchronous_ridesharing')
