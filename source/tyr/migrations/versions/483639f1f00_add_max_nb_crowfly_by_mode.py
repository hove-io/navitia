"""empty message

Revision ID: 483639f1f00
Revises: 4cd2ff722a7c
Create Date: 2018-08-21 11:48:08.941764

"""

# revision identifiers, used by Alembic.
revision = '483639f1f00'
down_revision = '4cd2ff722a7c'

from alembic import op
import sqlalchemy as sa


def upgrade():
    import json
    op.add_column('instance', sa.Column('max_nb_crowfly_by_mode', sa.PickleType(pickler=json), nullable=True))


def downgrade():
    op.drop_column('instance', 'max_nb_crowfly_by_mode')
