"""add of successive_physical_mode_to_limit_id for instances in case id changes

Revision ID: 520d48947e65
Revises: 4fa7f3f529e
Create Date: 2016-06-03 15:50:32.765884

"""

# revision identifiers, used by Alembic.
revision = '520d48947e65'
down_revision = '4fa7f3f529e'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('instance', sa.Column('successive_physical_mode_to_limit_id', sa.Text(), server_default='physical_mode:Bus', nullable=False))


def downgrade():
    op.drop_column('instance', 'successive_physical_mode_to_limit_id')
