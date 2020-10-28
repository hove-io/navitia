"""empty message

Revision ID: 3cf7f605e9ac
Revises: 061f7f29d76d
Create Date: 2020-10-28 16:46:43.250676

"""

# revision identifiers, used by Alembic.
revision = '3cf7f605e9ac'
down_revision = '061f7f29d76d'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column(
        'instance',
        sa.Column(
            'greenlet_pool_for_ridesharing_services', sa.Boolean(), nullable=False, server_default=sa.false()
        ),
    )
    op.add_column(
        'instance',
        sa.Column('ridesharing_greenlet_pool_size', sa.Integer(), nullable=False, server_default='10'),
    )


def downgrade():
    op.drop_column('instance', 'ridesharing_greenlet_pool_size')
    op.drop_column('instance', 'greenlet_pool_for_ridesharing_services')
