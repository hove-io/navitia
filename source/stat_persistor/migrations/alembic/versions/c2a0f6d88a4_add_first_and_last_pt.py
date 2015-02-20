"""add_first_and_last_pt

Revision ID: c2a0f6d88a4
Revises: 2c90cc5083e6
Create Date: 2015-02-10 15:20:04.726672

"""

# revision identifiers, used by Alembic.
revision = 'c2a0f6d88a4'
down_revision = '2c90cc5083e6'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.add_column('journey_request', sa.Column('departure_admin_name', sa.Text()), schema='stat')
    op.add_column('journey_request', sa.Column('arrival_admin_name', sa.Text()), schema='stat')

    op.add_column('journey_sections', sa.Column('from_admin_insee', sa.Text()), schema='stat')
    op.add_column('journey_sections', sa.Column('to_admin_insee', sa.Text()), schema='stat')

    op.add_column('journeys', sa.Column('first_pt_id', sa.Text()), schema='stat')
    op.add_column('journeys', sa.Column('first_pt_name', sa.Text()), schema='stat')
    op.add_column('journeys', sa.Column('first_pt_coord', ga.Geography(geometry_type='POINT', srid=4326, spatial_index=False), nullable=True), schema='stat')
    op.add_column('journeys', sa.Column('first_pt_admin_id', sa.Text()), schema='stat')
    op.add_column('journeys', sa.Column('first_pt_admin_name', sa.Text()), schema='stat')
    op.add_column('journeys', sa.Column('first_pt_admin_insee', sa.Text()), schema='stat')

    op.add_column('journeys', sa.Column('last_pt_id', sa.Text()), schema='stat')
    op.add_column('journeys', sa.Column('last_pt_name', sa.Text()), schema='stat')
    op.add_column('journeys', sa.Column('last_pt_coord', ga.Geography(geometry_type='POINT', srid=4326, spatial_index=False), nullable=True), schema='stat')
    op.add_column('journeys', sa.Column('last_pt_admin_id', sa.Text()), schema='stat')
    op.add_column('journeys', sa.Column('last_pt_admin_name', sa.Text()), schema='stat')
    op.add_column('journeys', sa.Column('last_pt_admin_insee', sa.Text()), schema='stat')


def downgrade():
    op.drop_column('journeys', 'last_pt_id', schema='stat')
    op.drop_column('journeys', 'last_pt_name', schema='stat')
    op.drop_column('journeys', 'last_pt_coord', schema='stat')
    op.drop_column('journeys', 'last_pt_admin_id', schema='stat')
    op.drop_column('journeys', 'last_pt_admin_name', schema='stat')
    op.drop_column('journeys', 'last_pt_admin_insee', schema='stat')

    op.drop_column('journeys', 'first_pt_id', schema='stat')
    op.drop_column('journeys', 'first_pt_name', schema='stat')
    op.drop_column('journeys', 'first_pt_coord', schema='stat')
    op.drop_column('journeys', 'first_pt_admin_id', schema='stat')
    op.drop_column('journeys', 'first_pt_admin_name', schema='stat')
    op.drop_column('journeys', 'first_pt_admin_insee', schema='stat')

    op.drop_column('journey_sections', 'from_admin_insee', schema='stat')
    op.drop_column('journey_sections', 'to_admin_insee', schema='stat')

    op.drop_column('journey_request', 'arrival_admin_name', schema='stat')
    op.drop_column('journey_request', 'departure_admin_name', schema='stat')

