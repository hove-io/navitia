"""empty message

Revision ID: c5cd5a41def
Revises: 44dab62dfff7
Create Date: 2016-10-06 14:38:01.870272

"""

# revision identifiers, used by Alembic.
revision = 'c5cd5a41def'
down_revision = '44dab62dfff7'

from alembic import op
import sqlalchemy as sa


def upgrade():
    user = sa.table('user', sa.column('shape'))
    op.execute(user.update().values(shape='null').where(user.c.shape==None))


def downgrade():
    pass
