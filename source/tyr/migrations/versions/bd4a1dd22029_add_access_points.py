"""Add new column: access_points

Revision ID: bd4a1dd22029
Revises: efa1d64928ca
Create Date: 2022-02-21 16:12:03.367595

"""

# revision identifiers, used by Alembic.
revision = 'bd4a1dd22029'
down_revision = 'efa1d64928ca'

from alembic import op
import sqlalchemy as sa
from navitiacommon import default_values


def upgrade():
    op.add_column(
        'instance',
        sa.Column(
            'access_points',
            sa.Boolean(),
            server_default='{}'.format(default_values.access_points),
            nullable=False,
        ),
    )


def downgrade():
    op.drop_column('instance', 'access_points')
