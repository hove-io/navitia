"""add journey_request

Revision ID: d8cff35d69
Revises: 75910df5976
Create Date: 2015-02-04 09:15:14.086759

"""

# revision identifiers, used by Alembic.
revision = 'd8cff35d69'
down_revision = '75910df5976'

from alembic import op
import sqlalchemy as sa

def upgrade():
    op.create_table(
        'journey_request',
        sa.Column('requested_date_time', sa.DateTime()),
        sa.Column('clockwise', sa.Boolean()),
        sa.Column('departure_insee', sa.Text()),
        sa.Column('departure_admin', sa.Text()),
        sa.Column('arrival_insee', sa.Text()),
        sa.Column('arrival_admin', sa.Text()),
        sa.Column('request_id', sa.BigInteger(), nullable=False),
        sa.ForeignKeyConstraint(['request_id'], ['stat.requests.id'],),
        schema='stat'
    )


def downgrade():
    op.drop_table('journey_request', schema='stat')
