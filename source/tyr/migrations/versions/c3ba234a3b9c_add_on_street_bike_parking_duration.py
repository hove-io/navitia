"""empty message

Revision ID: c3ba234a3b9c
Revises: fd13bb348665
Create Date: 2024-11-29 11:45:56.806718

"""

# revision identifiers, used by Alembic.
revision = 'c3ba234a3b9c'
down_revision = 'fd13bb348665'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column(
        'instance',
        sa.Column('on_street_bike_parking_duration', sa.Integer(), nullable=False, server_default='300'),
    )


def downgrade():
    op.drop_column('instance', 'on_street_bike_parking_duration')
