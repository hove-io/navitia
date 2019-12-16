"""add max_taxi_duration_to_pt

Revision ID: 6f2635ce00b4
Revises: 858b24967ff2
Create Date: 2019-12-04 12:12:42.130204

"""

# revision identifiers, used by Alembic.
revision = '6f2635ce00b4'
down_revision = '858b24967ff2'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column(
        'instance', sa.Column('max_taxi_duration_to_pt', sa.Integer(), nullable=False, server_default='1800')
    )


def downgrade():
    op.drop_column('instance', 'max_taxi_duration_to_pt')
