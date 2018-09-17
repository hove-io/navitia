"""remove global timezone

Revision ID: 47b663b9defc
Revises: 224621d9edde
Create Date: 2015-12-23 10:11:35.657364

"""

# revision identifiers, used by Alembic.
revision = '47b663b9defc'
down_revision = '224621d9edde'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.drop_column('parameters', 'timezone', schema='navitia')


def downgrade():
    op.add_column('parameters', sa.Column('timezone', sa.TEXT(), nullable=True), schema='navitia')
