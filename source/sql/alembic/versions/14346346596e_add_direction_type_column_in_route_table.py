"""add_direction_type_column_in_route_table

Revision ID: 14346346596e
Revises: 59f4456a029
Create Date: 2015-11-25 15:07:39.179241

"""

# revision identifiers, used by Alembic.
revision = '14346346596e'
down_revision = '59f4456a029'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.add_column('route', sa.Column('direction_type', sa.TEXT(), nullable=True), schema='navitia')


def downgrade():
    op.drop_column('route', 'direction_type', schema='navitia')
