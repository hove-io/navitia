""" Scope shape added in user table

Revision ID: 43b19d690da2
Revises: 064f0b16d4f6
Create Date: 2021-03-25 16:36:18.755335

"""

# revision identifiers, used by Alembic.
revision = '43b19d690da2'
down_revision = '064f0b16d4f6'

from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects.postgresql import ENUM, ARRAY
from navitiacommon.constants import DEFAULT_SCOPE_SHAPE, ENUM_SCOPE_SHAPE

scope_shape = ENUM(*ENUM_SCOPE_SHAPE, name='scope_shape', create_type=False)


def upgrade():
    scope_shape.create(bind=op.get_bind(), checkfirst=False)
    default_scope_shape = "{" + ", ".join(DEFAULT_SCOPE_SHAPE) + "}"
    op.add_column(
        'user', sa.Column('scope_shape', ARRAY(scope_shape), nullable=False, server_default=default_scope_shape)
    )


def downgrade():
    op.drop_column('user', 'scope_shape')
    sa.Enum(name='scope_shape').drop(op.get_bind(), checkfirst=False)
