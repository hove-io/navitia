"""add the end_point of a user

Revision ID: 18ef01ed8e1
Revises: 25529bc17860
Create Date: 2015-04-21 11:08:08.758684

"""

# revision identifiers, used by Alembic.
revision = '18ef01ed8e1'
down_revision = '25529bc17860'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column(u'requests', sa.Column('end_point_id', sa.Integer(), nullable=True), schema='stat')
    op.add_column(u'requests', sa.Column('end_point_name', sa.Text(), nullable=True), schema='stat')


def downgrade():
    op.drop_column(u'requests', 'end_point_id', schema='stat')
    op.drop_column(u'requests', 'end_point_name', schema='stat')
