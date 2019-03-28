"""
Add equipments provider

Revision ID: 1c8334cd1997
Revises: 16d8bca1453b
Create Date: 2019-03-26 15:16:17.438767

"""

# revision identifiers, used by Alembic.
revision = '1c8334cd1997'
down_revision = '16d8bca1453b'

from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects import postgresql


def upgrade():
    op.create_table(
        'equipments_provider',
        sa.Column('id', sa.Text(), nullable=False),
        sa.Column('instances', postgresql.ARRAY(sa.Text()), nullable=False),
        sa.Column('klass', sa.Text(), nullable=False),
        sa.Column('discarded', sa.Boolean(), nullable=False),
        sa.Column('args', postgresql.JSONB(), server_default='{}', nullable=True),
        sa.Column('created_at', sa.DateTime(), nullable=False),
        sa.Column('updated_at', sa.DateTime(), nullable=True),
        sa.PrimaryKeyConstraint('id'),
    )


def downgrade():
    op.drop_table('equipments_provider')
