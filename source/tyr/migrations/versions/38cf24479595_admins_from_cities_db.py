"""Add a field to use cities in osm2ed

Revision ID: 38cf24479595
Revises: 35d86bbbe642
Create Date: 2019-05-09 17:09:41.400340

"""

revision = '38cf24479595'
down_revision = '35d86bbbe642'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column(
        'instance', sa.Column('admins_from_cities_db', sa.Boolean(), nullable=False, server_default='False')
    )


def downgrade():
    op.drop_column('instance', 'admins_from_cities_db')
