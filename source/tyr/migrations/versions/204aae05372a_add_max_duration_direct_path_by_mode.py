"""max_{mode}_direct_path_duration
these parameters are nullable. When null, there is no limit

Revision ID: 204aae05372a
Revises: 1c8334cd1997
Create Date: 2019-03-25 15:49:29.025453

"""

# revision identifiers, used by Alembic.
revision = '204aae05372a'
down_revision = '1c8334cd1997'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column(
        'instance',
        sa.Column('max_bike_direct_path_duration', sa.Integer(), server_default='86400', nullable=False),
    )
    op.add_column(
        'instance',
        sa.Column('max_bss_direct_path_duration', sa.Integer(), server_default='86400', nullable=False),
    )
    op.add_column(
        'instance',
        sa.Column('max_car_direct_path_duration', sa.Integer(), server_default='86400', nullable=False),
    )
    op.add_column(
        'instance',
        sa.Column('max_ridesharing_direct_path_duration', sa.Integer(), server_default='86400', nullable=False),
    )
    op.add_column(
        'instance',
        sa.Column('max_taxi_direct_path_duration', sa.Integer(), server_default='86400', nullable=False),
    )
    op.add_column(
        'instance',
        sa.Column('max_walking_direct_path_duration', sa.Integer(), server_default='86400', nullable=False),
    )


def downgrade():
    op.drop_column('instance', 'max_walking_direct_path_duration')
    op.drop_column('instance', 'max_taxi_direct_path_duration')
    op.drop_column('instance', 'max_ridesharing_direct_path_duration')
    op.drop_column('instance', 'max_car_direct_path_duration')
    op.drop_column('instance', 'max_bss_direct_path_duration')
    op.drop_column('instance', 'max_bike_direct_path_duration')
