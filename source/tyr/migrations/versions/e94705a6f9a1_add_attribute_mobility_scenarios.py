"""Add attribute mobility_scenarios_enabled in the table instance

Revision ID: e94705a6f9a1
Revises: 06be8f8c0f18
Create Date: 2025-09-10 11:26:39.892455

"""

# revision identifiers, used by Alembic.
revision = 'e94705a6f9a1'
down_revision = '06be8f8c0f18'

from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects import postgresql


def upgrade():
    op.add_column(
        'instance', sa.Column('mobility_scenarios_enabled', sa.Boolean(), server_default='False', nullable=False)
    )


def downgrade():
    op.drop_column('instance', 'mobility_scenarios_enabled')
