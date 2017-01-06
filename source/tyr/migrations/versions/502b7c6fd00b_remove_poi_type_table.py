"""Remove poi_type table

Revision ID: 502b7c6fd00b
Revises: 57ee5581cc30
Create Date: 2016-11-24 16:33:34.318234

"""

# revision identifiers, used by Alembic.
revision = '502b7c6fd00b'
down_revision = '57ee5581cc30'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.drop_table('poi_type')


def downgrade():
    op.create_table('poi_type',
                    sa.Column('uri', sa.Text(), nullable=False),
                    sa.Column('name', sa.Text(), nullable=True),
                    sa.Column('instance_id', sa.Integer(), nullable=False),
                    sa.ForeignKeyConstraint(['instance_id'], ['instance.id'], ),
                    sa.PrimaryKeyConstraint('instance_id', 'uri')
                    )
