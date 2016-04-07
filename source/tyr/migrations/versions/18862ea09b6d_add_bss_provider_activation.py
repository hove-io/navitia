"""add_bss_provider_activation

Revision ID: 18862ea09b6d
Revises: 4d83f15e09b9
Create Date: 2016-04-06 16:36:36.344059

"""

# revision identifiers, used by Alembic.
revision = '18862ea09b6d'
down_revision = '4d83f15e09b9'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('instance', sa.Column('bss_provider', existing_type=sa.BOOLEAN(), server_default=sa.true()))


def downgrade():
    op.drop_column('instance', 'bss_provider')
