"""Add_headsign_in_vehiclejourney

Revision ID: 1faee1d2551a
Revises: 844a9fa86ad2
Create Date: 2020-05-12 16:43:03.076464

"""

# revision identifiers, used by Alembic.
revision = '1faee1d2551a'
down_revision = '844a9fa86ad2'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.add_column('vehicle_journey', sa.Column('headsign', sa.TEXT(), nullable=True), schema='navitia')


def downgrade():
    op.drop_column('vehicle_journey', 'headsign', schema='navitia')
