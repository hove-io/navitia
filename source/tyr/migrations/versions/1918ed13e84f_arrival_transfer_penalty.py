"""
Add column arrival_transfer_penalty in instance

Revision ID: 1918ed13e84f
Revises: fe508248efbd
Create Date: 2021-05-12 17:51:03.192049

"""

# revision identifiers, used by Alembic.
revision = '1918ed13e84f'
down_revision = 'fe508248efbd'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column(
        'instance', sa.Column('arrival_transfer_penalty', sa.Integer(), server_default='120', nullable=False)
    )


def downgrade():
    op.drop_column('instance', 'arrival_transfer_penalty')
