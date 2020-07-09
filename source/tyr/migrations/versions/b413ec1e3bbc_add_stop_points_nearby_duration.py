"""empty message

Revision ID: b413ec1e3bbc
Revises: c7e1cd821c6c
Create Date: 2020-07-06 16:52:54.086090

"""

# revision identifiers, used by Alembic.
revision = 'b413ec1e3bbc'
down_revision = 'c7e1cd821c6c'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column(
        'instance', sa.Column('stop_points_nearby_duration', sa.Integer(), server_default='300', nullable=False)
    )


def downgrade():
    op.drop_column('instance', 'stop_points_nearby_duration')
