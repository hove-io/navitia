"""co2_emission column added

Revision ID: 2d86200bcb93
Revises: 82749d34a18
Create Date: 2014-12-30 17:23:39.654559

"""

# revision identifiers, used by Alembic.
revision = '2d86200bcb93'
down_revision = '82749d34a18'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.add_column(
        'physical_mode',
        sa.Column('co2_emission', sa.FLOAT(), nullable=False, server_default='0'),
        schema='navitia',
    )


def downgrade():
    op.drop_column('physical_mode', 'co2_emission', schema='navitia')
