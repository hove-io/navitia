"""
Add columns max_{mode}_direct_path_distance for walking, car and bike

Revision ID: 893abc0159ba
Revises: 1918ed13e84f
Create Date: 2021-05-20 12:12:50.938325

"""

# revision identifiers, used by Alembic.
revision = '893abc0159ba'
down_revision = '1918ed13e84f'

from alembic import op
import sqlalchemy as sa

from navitiacommon import default_values


def upgrade():
    op.add_column(
        'instance',
        sa.Column(
            'max_bike_direct_path_distance',
            sa.Integer(),
            server_default=str(default_values.max_bike_direct_path_distance),
            nullable=False,
        ),
    )
    op.add_column(
        'instance',
        sa.Column(
            'max_bss_direct_path_distance',
            sa.Integer(),
            server_default=str(default_values.max_bss_direct_path_distance),
            nullable=False,
        ),
    )
    op.add_column(
        'instance',
        sa.Column(
            'max_car_direct_path_distance',
            sa.Integer(),
            server_default=str(default_values.max_car_direct_path_distance),
            nullable=False,
        ),
    )
    op.add_column(
        'instance',
        sa.Column(
            'max_walking_direct_path_distance',
            sa.Integer(),
            server_default=str(default_values.max_walking_direct_path_distance),
            nullable=False,
        ),
    )
    op.add_column(
        'instance',
        sa.Column(
            'max_car_no_park_direct_path_distance',
            sa.Integer(),
            server_default=str(default_values.max_car_no_park_direct_path_distance),
            nullable=False,
        ),
    )
    op.add_column(
        'instance',
        sa.Column(
            'max_ridesharing_direct_path_distance',
            sa.Integer(),
            server_default=str(default_values.max_ridesharing_direct_path_distance),
            nullable=False,
        ),
    )
    op.add_column(
        'instance',
        sa.Column(
            'max_taxi_direct_path_distance',
            sa.Integer(),
            server_default=str(default_values.max_taxi_direct_path_distance),
            nullable=False,
        ),
    )


def downgrade():
    op.drop_column('instance', 'max_walking_direct_path_distance')
    op.drop_column('instance', 'max_taxi_direct_path_distance')
    op.drop_column('instance', 'max_ridesharing_direct_path_distance')
    op.drop_column('instance', 'max_car_no_park_direct_path_distance')
    op.drop_column('instance', 'max_car_direct_path_distance')
    op.drop_column('instance', 'max_bss_direct_path_distance')
    op.drop_column('instance', 'max_bike_direct_path_distance')
