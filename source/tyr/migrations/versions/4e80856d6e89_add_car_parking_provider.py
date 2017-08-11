"""empty message

Revision ID: 4e80856d6e89
Revises: 4ec9c897fc03
Create Date: 2017-08-11 11:09:46.739733

"""

# revision identifiers, used by Alembic.
revision = '4e80856d6e89'
down_revision = '4ec9c897fc03'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('instance', sa.Column('car_parking_provider', sa.Boolean(), server_default='true', nullable=False))


def downgrade():
    op.drop_column('instance', 'car_parking_provider')
