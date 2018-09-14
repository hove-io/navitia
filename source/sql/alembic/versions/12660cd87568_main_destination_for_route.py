"""main destination for route

Revision ID: 12660cd87568
Revises: 29fc422c56cb
Create Date: 2015-05-05 13:47:06.507810

"""

# revision identifiers, used by Alembic.
revision = '12660cd87568'
down_revision = '3bea0b3cb116'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column(
        'route',
        sa.Column('destination_stop_area_id', sa.BIGINT(), primary_key=False, nullable=True),
        schema='navitia',
    )


def downgrade():
    op.drop_column('route', 'destination_stop_area_id', schema='navitia')
