"""foreign key datasets

Revision ID: 4f4ee492129f
Revises: 3dc7d057ec3
Create Date: 2016-02-29 10:04:19.632868

"""

# revision identifiers, used by Alembic.
revision = '4f4ee492129f'
down_revision = '3dc7d057ec3'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.execute("ALTER TABLE navitia.vehicle_journey DROP CONSTRAINT IF EXISTS vehicle_journey_frame_id_fkey")
    op.create_foreign_key('vehicle_journey_dataset_id_fkey', 'vehicle_journey', 'dataset', ['dataset_id'], ['id'], referent_schema='navitia', source_schema='navitia')


def downgrade():
    op.execute("ALTER TABLE navitia.vehicle_journey DROP CONSTRAINT IF EXISTS vehicle_journey_dataset_id_fkey")
    op.create_foreign_key('vehicle_journey_frame_id_fkey', 'vehicle_journey', 'frame', ['frame_id'], ['id'], referent_schema='navitia', source_schema='navitia')
