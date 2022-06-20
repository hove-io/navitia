"""empty message

Revision ID: 45b018b3198b
Revises: 535dfe5be550
Create Date: 2022-06-16 14:42:38.533188

"""

# revision identifiers, used by Alembic.
revision = '45b018b3198b'
down_revision = '535dfe5be550'

from alembic import op
import sqlalchemy as sa
from navitiacommon import default_values


def upgrade():
    op.add_column(
        'instance',
        sa.Column(
            'filter_odt_journeys',
            sa.Boolean(),
            server_default='{}'.format(default_values.filter_odt_journeys),
            nullable=False,
        ),
    )


def downgrade():
    op.drop_column('instance', 'filter_odt_journeys')
