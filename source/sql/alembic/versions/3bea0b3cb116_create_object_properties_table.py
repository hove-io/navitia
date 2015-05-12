"""create object_properties table

Revision ID: 3bea0b3cb116
Revises: 29fc422c56cb
Create Date: 2015-05-05 12:24:38.022248

"""

# revision identifiers, used by Alembic.
revision = '3bea0b3cb116'
down_revision = '29fc422c56cb'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.create_table('object_properties',
    sa.Column('object_id', sa.BIGINT(), nullable=False),
    sa.Column('object_type', sa.TEXT(), nullable=False),
    sa.Column('property_name', sa.TEXT(), nullable=False),
    sa.Column('property_value', sa.TEXT(), nullable=False),
    schema='navitia'
    )


def downgrade():
    op.drop_table('object_properties', schema='navitia')
