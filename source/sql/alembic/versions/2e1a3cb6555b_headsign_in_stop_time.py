"""headsign in stop_time

Revision ID: 2e1a3cb6555b
Revises: 1cec3dee6597
Create Date: 2015-07-24 10:27:41.674943

"""

# revision identifiers, used by Alembic.
revision = '2e1a3cb6555b'
down_revision = '1cec3dee6597'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column(
        'stop_time', sa.Column('headsign', sa.TEXT(), primary_key=False, nullable=True), schema='navitia'
    )


def downgrade():
    op.drop_column('stop_time', 'headsign', schema='navitia')
