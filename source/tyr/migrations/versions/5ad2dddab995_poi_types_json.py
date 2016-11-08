"""empty message

Revision ID: 5ad2dddab995
Revises: c5cd5a41def
Create Date: 2016-11-04 17:07:25.934771

"""

# revision identifiers, used by Alembic.
revision = '5ad2dddab995'
down_revision = 'c5cd5a41def'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.create_table('poi_type_json',
                    sa.Column('poi_types_json', sa.Text(), nullable=False),
                    sa.Column('instance_id', sa.Integer(), primary_key=True, nullable=False),
                    sa.ForeignKeyConstraint(['instance_id'], ['instance.id'], )
                    )


def downgrade():
    op.drop_table('poi_type_json')

