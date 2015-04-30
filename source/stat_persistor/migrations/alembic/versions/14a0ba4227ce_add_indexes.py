"""Add some indexes

Revision ID: 14a0ba4227ce
Revises: 18ef01ed8e1
Create Date: 2015-04-27 11:17:24.242790

"""

# revision identifiers, used by Alembic.
revision = '14a0ba4227ce'
down_revision = '18ef01ed8e1'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.create_index('coverages_region_id_idx', 'coverages', ['region_id'], schema='stat')

    op.create_index('journey_sections_from_admin_id_idx', 'journey_sections', ['from_admin_id'], schema='stat')
    op.create_index('journey_sections_type_idx', 'journey_sections', ['type'], schema='stat')

    op.create_index('requests_api_idx', 'requests', ['api'], schema='stat')


def downgrade():
    op.drop_index('coverages_region_id_idx', schema='stat')

    op.drop_index('journey_sections_from_admin_id_idx', schema='stat')
    op.drop_index('journey_sections_type_idx', schema='stat')

    op.drop_index('requests_api_idx', schema='stat')
