"""Add boarding and alighting time to stop_time

Revision ID: 11b1fb5bd523
Revises: 4429fe91ac94
Create Date: 2017-02-14 10:46:14.819580

"""

# revision identifiers, used by Alembic.
revision = '11b1fb5bd523'
down_revision = '4429fe91ac94'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.add_column('stop_time', sa.Column('alighting_time', sa.INTEGER(), nullable=True), schema='navitia')
    op.add_column('stop_time', sa.Column('boarding_time', sa.INTEGER(), nullable=True), schema='navitia')


def downgrade():
    op.drop_column('stop_time', 'boarding_time', schema='navitia')
    op.drop_column('stop_time', 'alighting_time', schema='navitia')
