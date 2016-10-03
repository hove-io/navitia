"""add a shape for the user

Revision ID: 44dab62dfff7
Revises: 162b4fa5836a
Create Date: 2016-09-21 15:30:43.609986

"""

# revision identifiers, used by Alembic.
revision = '44dab62dfff7'
down_revision = '3131d5bf5570'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('user', sa.Column('shape', sa.TEXT(), nullable=True))


def downgrade():
    op.drop_column('user', 'shape')
