"""add_skipped_stop_in_stop_time

Revision ID: 2c69b03d20fc
Revises: 2233bc5aef67
Create Date: 2021-10-14 16:48:30.105562

"""

# revision identifiers, used by Alembic.
revision = '2c69b03d20fc'
down_revision = '2233bc5aef67'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.add_column(
        'stop_time',
        sa.Column('skipped_stop', sa.BOOLEAN(), nullable=False, server_default="false"),
        schema='navitia',
    )


def downgrade():
    op.drop_column('stop_time', 'skipped_stop', schema='navitia')
