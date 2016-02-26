"""
rename table frame to dataset
rename vehicle_journey.frame_id to vehicle_journey.dataset_id

Revision ID: 3dc7d057ec3
Revises: 47b663b9defc
Create Date: 2016-02-23 14:33:35.446663

"""

# revision identifiers, used by Alembic.
revision = '3dc7d057ec3'
down_revision = '47b663b9defc'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.rename_table("frame", "dataset", schema='navitia')
    op.add_column('vehicle_journey', sa.Column('dataset_id', sa.BIGINT(), nullable=True), schema='navitia')
    op.execute("UPDATE navitia.vehicle_journey SET dataset_id=frame_id")
    op.execute("ALTER TABLE vehicle_journey DROP CONSTRAINT IF EXISTS vehicle_journey_frame_id_fkey")
    op.drop_column('vehicle_journey', 'frame_id', schema='navitia')
    op.create_foreign_key('vehicle_journey_dataset_id_fkey', 'vehicle_journey', 'dataset', ['dataset_id'], ['id'])


def downgrade():
    op.rename_table("dataset", "frame", schema='navitia')
    op.add_column('vehicle_journey', sa.Column('frame_id', sa.BIGINT(), nullable=True), schema='navitia')
    op.execute("UPDATE navitia.vehicle_journey SET frame_id=dataset_id")
    op.execute("ALTER TABLE vehicle_journey DROP CONSTRAINT IF EXISTS vehicle_journey_dataset_id_fkey")
    op.drop_column('vehicle_journey', 'dataset_id', schema='navitia')
    op.create_foreign_key('vehicle_journey_frame_id_fkey', 'vehicle_journey', 'frame', ['frame_id'], ['id'])
