"""add a type column on user for authentication

Revision ID: 42185d6d93cb
Revises: 2a77596bdf40
Create Date: 2015-04-16 08:42:07.405011

"""

# revision identifiers, used by Alembic.
revision = '42185d6d93cb'
down_revision = '2a77596bdf40'

from alembic import op
import sqlalchemy as sa

user_type = sa.Enum('with_free_instances', 'without_free_instances', 'super_user', name='user_type')


def upgrade():
    user_type.create(op.get_bind())
    op.add_column('user', sa.Column('type', user_type, nullable=False, server_default='with_free_instances'))


def downgrade():
    op.drop_column('user', 'type')
    user_type.drop(op.get_bind())
