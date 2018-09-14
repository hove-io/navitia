"""add field to know if a vj is a frequency vj

Revision ID: 82749d34a18
Revises: 43822e80c45a
Create Date: 2014-12-22 14:56:37.358868

"""

# revision identifiers, used by Alembic.
revision = '82749d34a18'
down_revision = '43822e80c45a'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.add_column(
        'vehicle_journey',
        sa.Column('is_frequency', sa.BOOLEAN(), nullable=True, default=False),
        schema='navitia',
    )


def downgrade():
    op.drop_column('vehicle_journey', 'is_frequency', schema='navitia')
