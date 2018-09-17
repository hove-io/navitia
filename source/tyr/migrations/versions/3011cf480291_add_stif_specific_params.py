"""add stif specific params

Revision ID: 3011cf480291
Revises: 3fde52b5e132
Create Date: 2015-12-09 12:07:22.812467

"""

# revision identifiers, used by Alembic.
revision = '3011cf480291'
down_revision = '3fde52b5e132'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('instance', sa.Column('max_duration', sa.Integer(), server_default='14400', nullable=False))
    op.add_column(
        'instance',
        sa.Column('night_bus_filter_base_factor', sa.Integer(), server_default='3600', nullable=False),
    )
    op.add_column(
        'instance', sa.Column('night_bus_filter_max_factor', sa.Integer(), server_default='3', nullable=False)
    )
    op.add_column(
        'instance', sa.Column('walking_transfer_penalty', sa.Integer(), server_default='2', nullable=False)
    )


def downgrade():
    op.drop_column('instance', 'walking_transfer_penalty')
    op.drop_column('instance', 'night_bus_filter_max_factor')
    op.drop_column('instance', 'night_bus_filter_base_factor')
    op.drop_column('instance', 'max_duration')
