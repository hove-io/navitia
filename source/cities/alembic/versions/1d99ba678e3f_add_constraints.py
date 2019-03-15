"""Add constraints

Revision ID: 1d99ba678e3f
Revises: c2373c7750f
Create Date: 2019-03-05 07:44:17.703306

"""

# revision identifiers, used by Alembic.
revision = '1d99ba678e3f'
down_revision = 'c2373c7750f'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.alter_column("administrative_regions", "level", nullable=False)
    op.alter_column("administrative_regions", "boundary", nullable=False)
    op.alter_column("administrative_regions", "coord", nullable=False)


def downgrade():
    op.alter_column("administrative_regions", "level", nullable=True)
    op.alter_column("administrative_regions", "boundary", nullable=True)
    op.alter_column("administrative_regions", "coord", nullable=True)
