"""add calendar to navitia types

The Calendar type was missing from the navitia.object_type table

Revision ID: 2c510fae878d
Revises: 14e13cf7a042
Create Date: 2015-05-15 15:08:24.766935

"""

# revision identifiers, used by Alembic.
revision = '2c510fae878d'
down_revision = '14e13cf7a042'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.execute("insert into navitia.object_type VALUES (24, 'Calendar');")


def downgrade():
    op.execute("delete from navitia.object_type where id = 24;")
