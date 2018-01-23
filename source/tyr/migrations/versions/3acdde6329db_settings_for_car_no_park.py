"""settings for car_no_park

Aka ridesharing

Revision ID: 3acdde6329db
Revises: 4e80856d6e89
Create Date: 2018-01-22 09:13:19.131809

"""

# revision identifiers, used by Alembic.
revision = '3acdde6329db'
down_revision = '4e80856d6e89'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('instance', sa.Column('max_car_no_park_duration_to_pt', sa.Integer(), nullable=False, server_default='1800'))
    op.add_column('instance', sa.Column('car_no_park_speed', sa.Float(), nullable=False, server_default='6.94'))


def downgrade():
    op.drop_column('instance', 'car_no_park_speed')
    op.drop_column('instance', 'max_car_no_park_duration_to_pt')
