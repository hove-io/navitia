"""Add_api_backends

Revision ID: 06be8f8c0f18
Revises: 8b3ddc6272b0
Create Date: 2025-09-04 12:18:10.528227

"""

# revision identifiers, used by Alembic.
revision = '06be8f8c0f18'
down_revision = '8b3ddc6272b0'

from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects import postgresql


def upgrade():
    op.add_column('instance', sa.Column('api_backends', postgresql.JSONB(astext_type=sa.Text()), nullable=True))


def downgrade():
    op.drop_column('instance', 'api_backends')
