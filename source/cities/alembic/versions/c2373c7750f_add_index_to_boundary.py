"""Add index to boundary

Revision ID: c2373c7750f
Revises: 15e13307464d
Create Date: 2014-09-17 10:18:24.739440

"""

# revision identifiers, used by Alembic.
revision = 'c2373c7750f'
down_revision = '15e13307464d'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.create_index(
        'administrative_regions_boundary_idx', 'administrative_regions', ['boundary'], postgresql_using='gist'
    )


def downgrade():
    op.drop_index('administrative_regions_boundary_idx')
