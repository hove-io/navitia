"""Fix default value of max_duration

Revision ID: 1173ed75e96c
Revises: 3011cf480291
Create Date: 2015-12-14 16:58:10.243364

"""

# revision identifiers, used by Alembic.
revision = '1173ed75e96c'
down_revision = '3011cf480291'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.alter_column('instance', 'max_duration', existing_type=sa.INTEGER(), server_default='86400')


def downgrade():
    op.alter_column('instance', 'max_duration', existing_type=sa.INTEGER(), server_default='14400')
