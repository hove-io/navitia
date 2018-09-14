"""Add autocomplete_backend to Instance

Revision ID: 105823db902c
Revises: 483639f1f00
Create Date: 2018-08-30 07:57:48.630448

"""

# revision identifiers, used by Alembic.
revision = '105823db902c'
down_revision = '483639f1f00'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column(
        'instance', sa.Column('autocomplete_backend', sa.Text(), nullable=False, server_default='kraken')
    )


def downgrade():
    op.drop_column('instance', 'autocomplete_backend')
