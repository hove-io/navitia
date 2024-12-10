"""
Add attribute disruptions_on_poi in the table instance with default value False

Revision ID: ca5a0a70a9ba
Revises: c3ba234a3b9c
Create Date: 2024-12-04 11:47:52.807680

"""

# revision identifiers, used by Alembic.
revision = 'ca5a0a70a9ba'
down_revision = 'c3ba234a3b9c'

from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects import postgresql


def upgrade():
    op.add_column(
        'instance', sa.Column('disruptions_on_poi', sa.Boolean(), server_default='false', nullable=False)
    )


def downgrade():
    op.drop_column('instance', 'disruptions_on_poi')
