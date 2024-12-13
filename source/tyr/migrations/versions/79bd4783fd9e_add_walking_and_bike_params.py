"""
Add some attributs in the table instance as well as traveler_profile

Revision ID: 79bd4783fd9e
Revises: ca5a0a70a9ba
Create Date: 2024-12-11 16:47:57.879165

"""

# revision identifiers, used by Alembic.
revision = '79bd4783fd9e'
down_revision = 'ca5a0a70a9ba'

from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects import postgresql

def upgrade():
    op.add_column('instance', sa.Column('bicycle_type', sa.Text(), server_default='hybrid', nullable=False))
    op.add_column('instance', sa.Column('bike_avoid_bad_surfaces', sa.Float(), server_default='0.25', nullable=False))
    op.add_column('instance', sa.Column('bike_country_crossing_cost', sa.Float(), server_default='600', nullable=False))
    op.add_column('instance', sa.Column('bike_country_crossing_penalty', sa.Float(), server_default='0', nullable=False))
    op.add_column('instance', sa.Column('bike_destination_only_penalty', sa.Float(), server_default='120', nullable=False))
    op.add_column('instance', sa.Column('bike_maneuver_penalty', sa.Float(), server_default='5', nullable=False))
    op.add_column('instance', sa.Column('bike_service_factor', sa.Float(), server_default='1', nullable=False))
    op.add_column('instance', sa.Column('bike_service_penalty', sa.Float(), server_default='0', nullable=False))
    op.add_column('instance', sa.Column('bike_shortest', sa.Boolean(), server_default='False', nullable=False))
    op.add_column('instance', sa.Column('bike_use_ferry', sa.Float(), server_default='0.5', nullable=False))
    op.add_column('instance', sa.Column('bike_use_hills', sa.Float(), server_default='0.5', nullable=False))
    op.add_column('instance', sa.Column('bike_use_living_streets', sa.Float(), server_default='0.5', nullable=False))
    op.add_column('instance', sa.Column('bike_use_roads', sa.Float(), server_default='0.5', nullable=False))
    op.add_column('instance', sa.Column('walking_alley_factor', sa.Float(), server_default='2.0', nullable=False))
    op.add_column('instance', sa.Column('walking_destination_only_penalty', sa.Float(), server_default='120', nullable=False))
    op.add_column('instance', sa.Column('walking_driveway_factor', sa.Float(), server_default='5.0', nullable=False))
    op.add_column('instance', sa.Column('walking_ignore_oneways', sa.Boolean(), server_default='True', nullable=False))
    op.add_column('instance', sa.Column('walking_max_hiking_difficulty', sa.Integer(), server_default='1', nullable=False))
    op.add_column('instance', sa.Column('walking_service_factor', sa.Float(), server_default='1', nullable=False))
    op.add_column('instance', sa.Column('walking_shortest', sa.Boolean(), server_default='False', nullable=False))
    op.add_column('instance', sa.Column('walking_sidewalk_factor', sa.Float(), server_default='1.0', nullable=False))
    op.add_column('instance', sa.Column('walking_step_penalty', sa.Float(), server_default='30', nullable=False))
    op.add_column('instance', sa.Column('walking_use_ferry', sa.Float(), server_default='0.5', nullable=False))
    op.add_column('instance', sa.Column('walking_use_hills', sa.Float(), server_default='0.5', nullable=False))
    op.add_column('instance', sa.Column('walking_use_living_streets', sa.Float(), server_default='0.6', nullable=False))
    op.add_column('instance', sa.Column('walking_use_tracks', sa.Float(), server_default='0.5', nullable=False))
    op.add_column('instance', sa.Column('walking_walkway_factor', sa.Float(), server_default='1.0', nullable=False))
    op.add_column('traveler_profile', sa.Column('walking_step_penalty', sa.Float(), server_default='30', nullable=False))
    op.add_column('traveler_profile', sa.Column('walking_use_hills', sa.Float(), server_default='0.5', nullable=False))
    op.add_column(
        'traveler_profile',
        sa.Column('max_walking_direct_path_duration', sa.Integer(), server_default='86400', nullable=False),
    )
    op.add_column(
        'traveler_profile',
        sa.Column('max_bike_direct_path_duration', sa.Integer(), server_default='86400', nullable=False),
    )
    op.add_column(
        'traveler_profile',
        sa.Column('max_bss_direct_path_duration', sa.Integer(), server_default='86400', nullable=False),
    )
    op.add_column(
        'traveler_profile',
        sa.Column('max_car_direct_path_duration', sa.Integer(), server_default='86400', nullable=False),
    )
    op.add_column(
        'traveler_profile',
        sa.Column('max_ridesharing_direct_path_duration', sa.Integer(), server_default='86400', nullable=False),
    )


def downgrade():
    op.drop_column('instance', 'walking_walkway_factor')
    op.drop_column('instance', 'walking_use_tracks')
    op.drop_column('instance', 'walking_use_living_streets')
    op.drop_column('instance', 'walking_use_hills')
    op.drop_column('instance', 'walking_use_ferry')
    op.drop_column('instance', 'walking_step_penalty')
    op.drop_column('instance', 'walking_sidewalk_factor')
    op.drop_column('instance', 'walking_shortest')
    op.drop_column('instance', 'walking_service_factor')
    op.drop_column('instance', 'walking_max_hiking_difficulty')
    op.drop_column('instance', 'walking_ignore_oneways')
    op.drop_column('instance', 'walking_driveway_factor')
    op.drop_column('instance', 'walking_destination_only_penalty')
    op.drop_column('instance', 'walking_alley_factor')
    op.drop_column('instance', 'bike_use_roads')
    op.drop_column('instance', 'bike_use_living_streets')
    op.drop_column('instance', 'bike_use_hills')
    op.drop_column('instance', 'bike_use_ferry')
    op.drop_column('instance', 'bike_shortest')
    op.drop_column('instance', 'bike_service_penalty')
    op.drop_column('instance', 'bike_service_factor')
    op.drop_column('instance', 'bike_maneuver_penalty')
    op.drop_column('instance', 'bike_destination_only_penalty')
    op.drop_column('instance', 'bike_country_crossing_penalty')
    op.drop_column('instance', 'bike_country_crossing_cost')
    op.drop_column('instance', 'bike_avoid_bad_surfaces')
    op.drop_column('instance', 'bicycle_type')
    op.drop_column('traveler_profile', 'walking_step_penalty')
    op.drop_column('traveler_profile', 'walking_use_hills')
    op.drop_column('traveler_profile', 'max_walking_direct_path_duration')
    op.drop_column('traveler_profile', 'max_bike_direct_path_duration')
    op.drop_column('traveler_profile', 'max_bss_direct_path_duration')
    op.drop_column('traveler_profile', 'max_car_direct_path_duration')
    op.drop_column('traveler_profile', 'max_ridesharing_direct_path_duration')