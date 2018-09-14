"""'block_until' column added for restriction access before this date.

Revision ID: 962cfcce76f
Revises: 11eae2b0e6b0
Create Date: 2015-12-16 11:46:05.960382

"""

# revision identifiers, used by Alembic.
revision = '962cfcce76f'
down_revision = '11eae2b0e6b0'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('user', sa.Column('block_until', sa.DateTime(timezone=True), nullable=True))


def downgrade():
    op.drop_column('user', 'block_until')
