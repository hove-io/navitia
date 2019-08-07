"""type comments

Revision ID: 25b7f3ace052
Revises: 56c1c7a19078
Create Date: 2019-08-06 12:01:56.485924

"""

# revision identifiers, used by Alembic.
revision = '25b7f3ace052'
down_revision = '56c1c7a19078'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.add_column('comments', sa.Column('type', sa.Text(), nullable=True), schema='navitia')


def downgrade():
    op.drop_column('comments', 'type', schema='navitia')
