"""add poi_type for an instance
they are used only for osm import

Revision ID: 3fde52b5e132
Revises: 3e56c7e0a4a4
Create Date: 2015-11-26 08:50:13.923059

"""

# revision identifiers, used by Alembic.
revision = '3fde52b5e132'
down_revision = '3e56c7e0a4a4'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.create_table(
        'poi_type',
        sa.Column('uri', sa.Text(), nullable=False),
        sa.Column('name', sa.Text(), nullable=True),
        sa.Column('instance_id', sa.Integer(), nullable=False),
        sa.ForeignKeyConstraint(['instance_id'], ['instance.id']),
        sa.PrimaryKeyConstraint('instance_id', 'uri'),
    )


def downgrade():
    op.drop_table('poi_type')
