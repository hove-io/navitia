"""add a default coord in the user to be used in the bragi autocomplete service

Revision ID: 4ec9c897fc03
Revises: 4af6f969bfa8
Create Date: 2017-07-07 09:38:59.021358

"""

# revision identifiers, used by Alembic.
revision = '4ec9c897fc03'
down_revision = '4af6f969bfa8'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('user', sa.Column('default_coord', sa.Text(), nullable=True))


def downgrade():
    op.drop_column('user', 'default_coord')
