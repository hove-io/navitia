"""shape in line and route

Revision ID: 40d9139ba0ed
Revises: 5812ecbcd76d
Create Date: 2014-10-03 17:00:06.118173

"""

# revision identifiers, used by Alembic.
revision = '40d9139ba0ed'
down_revision = '5812ecbcd76d'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.add_column(
        'line',
        sa.Column(
            'shape', ga.Geography(geometry_type='MULTILINESTRING', srid=4326, spatial_index=False), nullable=True
        ),
        schema='navitia',
    )
    op.add_column(
        'route',
        sa.Column(
            'shape', ga.Geography(geometry_type='MULTILINESTRING', srid=4326, spatial_index=False), nullable=True
        ),
        schema='navitia',
    )


def downgrade():
    op.drop_column('route', 'shape', schema='navitia')
    op.drop_column('line', 'shape', schema='navitia')
