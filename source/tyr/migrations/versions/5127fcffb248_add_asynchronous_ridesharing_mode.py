"""add asynchronous ridesharing option

Revision ID: 5127fcffb248
Revises: b413ec1e3bbc
Create Date: 2020-09-15 15:16:26.651648

"""

# revision identifiers, used by Alembic.
revision = '5127fcffb248'
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
