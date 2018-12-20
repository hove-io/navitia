"""add poi_types_json to mimir autocomplete params in tyr

Revision ID: 3f620c440544
Revises: e632d689de5
Create Date: 2018-11-22 09:35:17.988985

"""

# revision identifiers, used by Alembic.
revision = '3f620c440544'
down_revision = 'e632d689de5'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('autocomplete_parameter', sa.Column('poi_types_json', sa.Text(), nullable=True))


def downgrade():
    op.drop_column('autocomplete_parameter', 'poi_types_json')
