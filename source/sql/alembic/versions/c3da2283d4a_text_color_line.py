"""text_color_line

Revision ID: c3da2283d4a
Revises: 4ccd66ece649
Create Date: 2015-11-19 15:55:03.740677

"""

# revision identifiers, used by Alembic.
revision = 'c3da2283d4a'
down_revision = '4ccd66ece649'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga
from sqlalchemy.dialects import postgresql


def upgrade():
    op.add_column('line', sa.Column('text_color', sa.TEXT(), nullable=True), schema='navitia')


def downgrade():
    op.drop_column('line', 'text_color', schema='navitia')
