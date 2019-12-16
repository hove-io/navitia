"""add car speed on edges

Revision ID: 844a9fa86ad2
Revises: 25b7f3ace052
Create Date: 2019-12-09 13:52:27.533052

"""

# revision identifiers, used by Alembic.
revision = '844a9fa86ad2'
down_revision = '25b7f3ace052'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.add_column('edge', sa.Column('car_speed', sa.Float(), nullable=True), schema='georef')


def downgrade():
    op.add_column('edge', 'car_speed', schema='georef')
