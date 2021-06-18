"""Add places_proximity_radius in instance table

Revision ID: 7f757939ace8
Revises: 941b197dbd58
Create Date: 2021-06-09 13:12:50.315961

"""

# revision identifiers, used by Alembic.
revision = '7f757939ace8'
down_revision = '941b197dbd58'

from alembic import op
import sqlalchemy as sa

from navitiacommon import default_values


def upgrade():
    op.add_column(
        'instance',
        sa.Column(
            'places_proximity_radius',
            sa.Integer(),
            server_default=str(default_values.places_proximity_radius),
            nullable=False,
        ),
    )


def downgrade():
    op.drop_column('instance', 'places_proximity_radius')
