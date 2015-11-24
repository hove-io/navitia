"""license and website in contributor

Revision ID: 59f4456a029
Revises: c3da2283d4a
Create Date: 2015-11-20 11:18:47.970759

"""

# revision identifiers, used by Alembic.
revision = '59f4456a029'
down_revision = 'c3da2283d4a'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('contributor', sa.Column('website', sa.TEXT(), nullable=True), schema='navitia')
    op.add_column('contributor', sa.Column('license', sa.TEXT(), nullable=True), schema='navitia')


def downgrade():
    op.drop_column('contributor', 'website', schema='navitia')
    op.drop_column('contributor', 'license', schema='navitia')
