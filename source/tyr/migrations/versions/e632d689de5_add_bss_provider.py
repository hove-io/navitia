"""Add bss provider

Revision ID: e632d689de5
Revises: 1b59ffb421e4
Create Date: 2018-09-18 08:57:37.738724

"""

# revision identifiers, used by Alembic.
revision = 'e632d689de5'
down_revision = '1b59ffb421e4'

from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects import postgresql


def upgrade():
    op.create_table(
        'bss_provider',
        sa.Column('id', sa.Text(), nullable=False),
        sa.Column('network', sa.Text(), nullable=False),
        sa.Column('klass', sa.Text(), nullable=False),
        sa.Column('timeout', sa.Interval(), nullable=False),
        sa.Column('discarded', sa.Boolean(), nullable=False),
        sa.Column('args', postgresql.JSONB(), server_default='{}', nullable=True),
        sa.Column('created_at', sa.DateTime(), nullable=False),
        sa.Column('updated_at', sa.DateTime(), nullable=True),
        sa.PrimaryKeyConstraint('id'),
    )


def downgrade():
    op.drop_table('bss_provider')
