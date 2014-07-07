"""remove coords

Revision ID: d8ce67cdcaf
Revises: 2dbd03089f34
Create Date: 2014-07-07 11:50:03.560522

"""

# revision identifiers, used by Alembic.
revision = 'd8ce67cdcaf'
down_revision = '2dbd03089f34'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.drop_column('poi', 'coord')
    op.drop_column('stop_area', 'coord')
    op.drop_column('stop_point', 'coord')


def downgrade():
    op.add_column('stop_point', sa.Column('coord', sa.Geography(geometry_type=u'POINT', srid=4326), nullable=True))
    op.add_column('stop_area', sa.Column('coord', sa.Geography(geometry_type=u'POINT', srid=4326), nullable=True))
    op.add_column('poi', sa.Column('coord', sa.Geography(geometry_type=u'POINT', srid=4326), nullable=True))
