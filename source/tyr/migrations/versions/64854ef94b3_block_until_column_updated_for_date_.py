"""'block_until' column updated for date format.

Revision ID: 64854ef94b3
Revises: 2320ba18ce1
Create Date: 2015-12-22 15:15:21.597541

"""

# revision identifiers, used by Alembic.
revision = '64854ef94b3'
down_revision = '2320ba18ce1'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.drop_column('user', 'block_until')
    op.add_column('user', sa.Column('block_until', sa.DateTime(), nullable=True))


def downgrade():
    op.drop_column('user', 'block_until')
    op.add_column('user', sa.Column('block_until', sa.DateTime(timezone=True), nullable=True))
