"""Add attribute additional_parameters in the table instance

Revision ID: 0f57c3342d97
Revises: c7010efdcf62
Create Date: 2023-05-26 11:09:58.935554

"""

# revision identifiers, used by Alembic.
revision = '0f57c3342d97'
down_revision = 'c7010efdcf62'

from alembic import op
import sqlalchemy as sa
from navitiacommon import default_values

def upgrade():
    op.add_column(
        'instance',
        sa.Column(
            'additional_parameters',
            sa.Boolean(),
            server_default='{}'.format(default_values.additional_parameters),
            nullable=False,
        ),
    )


def downgrade():
    op.drop_column('instance', 'additional_parameters')
