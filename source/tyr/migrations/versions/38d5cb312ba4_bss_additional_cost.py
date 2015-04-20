""" add bss_additional_cost column

Revision ID: 38d5cb312ba4
Revises: 2a77596bdf40
Create Date: 2015-04-20 17:59:30.209730

"""

# revision identifiers, used by Alembic.
revision = '38d5cb312ba4'
down_revision = '2a77596bdf40'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('instance', sa.Column('bss_additional_cost', sa.Integer(), nullable=True))


def downgrade():
    op.drop_column('instance', 'bss_additional_cost')
