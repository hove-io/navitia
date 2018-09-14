"""feed_info table added

Revision ID: 1cec3dee6597
Revises: 538bc4ea9cd1
Create Date: 2015-06-15 14:54:28.590546

"""

# revision identifiers, used by Alembic.
revision = '1cec3dee6597'
down_revision = '291c8dbbaed1'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.create_table(
        'feed_info',
        sa.Column('key', sa.TEXT(), primary_key=True, nullable=False),
        sa.Column('value', sa.TEXT(), primary_key=False, nullable=True),
        sa.PrimaryKeyConstraint('key'),
        schema='navitia',
    )


def downgrade():
    op.drop_table('feed_info', schema='navitia')
