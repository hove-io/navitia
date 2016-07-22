"""add source for poi and street_network

Revision ID: 4429fe91ac94
Revises: 27fae61b6786
Create Date: 2016-07-07 16:30:29.214966

"""

# revision identifiers, used by Alembic.
revision = '4429fe91ac94'
down_revision = '27fae61b6786'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.add_column('parameters', sa.Column('poi_source', sa.TEXT(), nullable=True), schema='navitia')
    op.add_column('parameters', sa.Column('street_network_source', sa.TEXT(), nullable=True), schema='navitia')


def downgrade():
    op.drop_column('parameters', 'street_network_source', schema='navitia')
    op.drop_column('parameters', 'poi_source', schema='navitia')
