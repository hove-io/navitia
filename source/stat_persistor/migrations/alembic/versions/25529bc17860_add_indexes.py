"""add some basic indexes

Revision ID: 25529bc17860
Revises: c2a0f6d88a4
Create Date: 2015-04-17 08:54:33.122239

"""

# revision identifiers, used by Alembic.
revision = '25529bc17860'
down_revision = 'c2a0f6d88a4'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.create_index('requests_request_date_idx', 'requests', ['request_date'], schema='stat')
    op.create_index('requests_user_id_idx', 'requests', ['user_id'], schema='stat')
    op.create_index('requests_user_name_idx', 'requests', ['user_name'], schema='stat')

    op.create_index('journeys_request_id_idx', 'journeys', ['request_id'], schema='stat')

    op.create_index('journey_request_request_id_idx', 'journey_request', ['request_id'], schema='stat')

    op.create_index('journey_sections_journey_id_idx', 'journey_sections', ['journey_id'], schema='stat')

    op.create_index('filter_interpreted_parameters_id_idx', 'filter', ['interpreted_parameter_id'], schema='stat')

    op.create_index('interpreted_parameters_request_id_idx', 'interpreted_parameters', ['request_id'], schema='stat')

    op.create_index('parameters_request_id_idx', 'parameters', ['request_id'], schema='stat')


def downgrade():
    op.drop_index('requests_request_date_idx', schema='stat')
    op.drop_index('requests_user_id_idx', schema='stat')
    op.drop_index('requests_user_name_idx', schema='stat')

    op.drop_index('journeys_request_id_idx', schema='stat')

    op.drop_index('journey_request_request_id_idx', schema='stat')

    op.drop_index('journey_sections_journey_id_idx', schema='stat')

    op.drop_index('filter_interpreted_parameters_id_idx', schema='stat')

    op.drop_index('interpreted_parameters_request_id_idx', schema='stat')

    op.drop_index('parameters_request_id_idx', schema='stat')
