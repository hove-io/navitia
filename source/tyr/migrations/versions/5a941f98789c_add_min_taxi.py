""" Adding min_taxi

Revision ID: 5a941f98789c
Revises: 35d86bbbe642
Create Date: 2019-05-23 14:14:51.103593

"""

# revision identifiers, used by Alembic.
revision = '5a941f98789c'
down_revision = '35d86bbbe642'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('instance', sa.Column('min_taxi', sa.Integer(), nullable=False))


def downgrade():
    op.drop_column('instance', 'min_taxi')
