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
from navitiacommon.constants import DEFAULT_SHAPE_SCOPE, ENUM_SHAPE_SCOPE

shape_scope = ENUM(*ENUM_SHAPE_SCOPE, name='shape_scope', create_type=False)


def upgrade():
    shape_scope.create(bind=op.get_bind(), checkfirst=False)
    default_shape_scope = "{" + ", ".join(DEFAULT_SHAPE_SCOPE) + "}"
    op.add_column(
        'user', sa.Column('shape_scope', ARRAY(shape_scope), nullable=False, server_default=default_shape_scope)
    )


def downgrade():
    op.drop_column('user', 'shape_scope')
    sa.Enum(name='shape_scope').drop(op.get_bind(), checkfirst=False)
