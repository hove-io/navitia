"""merge postal_codes and object_properties

Revision ID: 4bb1cfb0e7ef
Revises: ('13673746db16', '3bea0b3cb116')
Create Date: 2015-05-13 08:43:25.644224

"""

# revision identifiers, used by Alembic.
revision = '4bb1cfb0e7ef'
down_revision = ('13673746db16', '3bea0b3cb116')

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    pass


def downgrade():
    pass
