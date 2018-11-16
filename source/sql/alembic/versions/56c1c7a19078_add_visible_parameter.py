"""add visible parameter

Revision ID: 56c1c7a19078
Revises: 46168c7abc89
Create Date: 2018-11-13 15:33:38.146755

"""

# revision identifiers, used by Alembic.
revision = '56c1c7a19078'
down_revision = '46168c7abc89'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga
from sqlalchemy.dialects import postgresql


def upgrade():
    ### Add new flag for input/ouput visibility
    op.add_column(
        'way', sa.Column('visible', sa.BOOLEAN(), nullable=False, server_default='true'), schema='georef'
    )


def downgrade():
    op.drop_column('way', 'visible', schema='georef')
