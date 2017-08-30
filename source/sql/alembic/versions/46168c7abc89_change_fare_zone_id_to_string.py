"""change fare_zone to string in the table stop_point

Revision ID: 46168c7abc89
Revises: 11b1fb5bd523
Create Date: 2017-08-30 17:03:17.764614

"""

# revision identifiers, used by Alembic.
revision = '46168c7abc89'
down_revision = '11b1fb5bd523'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga
from sqlalchemy.dialects import postgresql

def upgrade():
    op.execute("ALTER TABLE stop_point ALTER COLUMN fare_zone TYPE text;")


def downgrade():
    op.execute("ALTER TABLE stop_point ALTER COLUMN fare_zone TYPE integer;")

