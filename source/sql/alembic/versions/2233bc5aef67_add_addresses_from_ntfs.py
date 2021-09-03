"""add addresses from ntfs

Revision ID: 2233bc5aef67
Revises: 1faee1d2551a
Create Date: 2021-09-02 15:24:05.549960

"""

# revision identifiers, used by Alembic.
revision = '2233bc5aef67'
down_revision = '1faee1d2551a'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.create_table(
        'address',
        sa.Column('id', sa.TEXT(), nullable=False),
        sa.Column('house_number', sa.TEXT(), nullable=True),
        sa.Column('street_name', sa.TEXT(), nullable=True),
        schema='navitia',
    )
    op.add_column('stop_point', sa.Column('address_id', sa.TEXT(), nullable=True), schema='navitia')


def downgrade():
    op.drop_table('address', schema='navitia')
    op.drop_column('stop_point', 'address_id', schema='navitia')
