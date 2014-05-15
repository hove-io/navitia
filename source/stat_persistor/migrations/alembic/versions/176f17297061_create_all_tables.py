"""create_all_tables

Revision ID: 176f17297061
Revises: None
Create Date: 2014-04-09 09:15:25.882911

"""

# revision identifiers, used by Alembic.
revision = '176f17297061'
down_revision = None

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    create_schema()
    create_requests()
    create_coverages()
    create_parameters()
    create_errors()
    create_journeys()
    create_journey_sections()

def create_schema():
    op.execute("CREATE SCHEMA stat;")

def create_requests():
    op.create_table(
        'requests',
        sa.Column('id', sa.Integer(), primary_key=True, nullable=False),
        sa.Column('request_date', sa.DateTime(), nullable=False),
        sa.Column('user_id', sa.Integer(), nullable=False),
        sa.Column('user_name', sa.Text()),
        sa.Column('app_id', sa.Integer()),
        sa.Column('app_name', sa.Text()),
        sa.Column('request_duration', sa.Integer()),
        sa.Column('api', sa.Text()),
        sa.Column('host', sa.Text()),
        sa.Column('client', sa.Text()),
        sa.Column('response_size', sa.Integer()),
        schema='stat'
    )

def create_coverages():
    op.create_table(
        'coverages',
        sa.Column('region_id', sa.Text()),
        sa.Column('request_id', sa.BigInteger(), nullable=False),
        sa.ForeignKeyConstraint(['request_id'], ['stat.requests.id'],),
        schema='stat'
    )

def create_errors():
    op.create_table(
        'errors',
        sa.Column('id', sa.Text()),
        sa.Column('message', sa.Text()),
        sa.Column('request_id', sa.BigInteger(), nullable=False),
        sa.ForeignKeyConstraint(['request_id'], ['stat.requests.id'],),
        schema='stat'
    )

def create_parameters():
    op.create_table(
        'parameters',
        sa.Column('param_key', sa.Text()),
        sa.Column('param_value', sa.Text()),
        sa.Column('request_id', sa.BigInteger(), nullable=False),
        sa.ForeignKeyConstraint(['request_id'], ['stat.requests.id'],),
        schema='stat'
    )

def create_journeys():
    op.create_table(
        'journeys',
        sa.Column('id', sa.Integer(), primary_key=True, nullable=False),
        sa.Column('requested_date_time', sa.DateTime(), nullable=False),
        sa.Column('departure_date_time', sa.DateTime(), nullable=False),
        sa.Column('arrival_date_time', sa.DateTime(), nullable=False),
        sa.Column('duration', sa.Integer()),
        sa.Column('nb_transfers', sa.Integer()),
        sa.Column('type', sa.Text()),
        sa.Column('request_id', sa.BigInteger(), nullable=False),
        sa.ForeignKeyConstraint(['request_id'], ['stat.requests.id'],),
        schema='stat'
    )

def create_journey_sections():
    op.create_table(
        'journey_sections',
        sa.Column('id', sa.Integer(), primary_key=True, nullable=False),
        sa.Column('departure_date_time', sa.DateTime(), nullable=False),
        sa.Column('arrival_date_time', sa.DateTime(), nullable=False),
        sa.Column('duration', sa.Integer()),
        sa.Column('mode', sa.Text()),
        sa.Column('type', sa.Text()),
        sa.Column('from_embedded_type', sa.Text()),
        sa.Column('from_id', sa.Text()),
        sa.Column('from_name', sa.Text()),
        sa.Column('from_coord', ga.Geography(geometry_type='POINT', srid=4326, spatial_index=False), nullable=True),
        sa.Column('from_admin_id', sa.Text()),
        sa.Column('from_admin_name', sa.Text()),
        sa.Column('to_embedded_type', sa.Text()),
        sa.Column('to_id', sa.Text()),
        sa.Column('to_name', sa.Text()),
        sa.Column('to_coord', ga.Geography(geometry_type='POINT', srid=4326, spatial_index=False), nullable=True),
        sa.Column('to_admin_id', sa.Text()),
        sa.Column('to_admin_name', sa.Text()),
        sa.Column('vehicle_journey_id', sa.Text()),
        sa.Column('line_id', sa.Text()),
        sa.Column('line_code', sa.Text()),
        sa.Column('route_id', sa.Text()),
        sa.Column('network_id', sa.Text()),
        sa.Column('network_name', sa.Text()),
        sa.Column('commercial_mode_id', sa.Text()),
        sa.Column('commercial_mode_name', sa.Text()),
        sa.Column('physical_mode_id', sa.Text()),
        sa.Column('physical_mode_name', sa.Text()),
        sa.Column('request_id', sa.BigInteger(), nullable=False),
        sa.Column('journey_id', sa.BigInteger(), nullable=False),
        sa.ForeignKeyConstraint(['request_id'], ['stat.requests.id'],),
        sa.ForeignKeyConstraint(['journey_id'], ['stat.journeys.id'],),
        schema='stat'
    )

def downgrade():
    op.drop_table('journey_sections', schema='stat')
    op.drop_table('journeys', schema='stat')
    op.drop_table('parameters', schema='stat')
    op.drop_table('coverages', schema='stat')
    op.drop_table('errors', schema='stat')
    op.drop_table('requests', schema='stat')
