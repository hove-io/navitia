"""Remove column 'instances' in table 'equipments_provider'

Revision ID: 553c4f353aeb
Revises: 204aae05372a
Create Date: 2019-04-11 12:02:10.857623

"""

# revision identifiers, used by Alembic.
revision = '553c4f353aeb'
down_revision = '204aae05372a'

from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects import postgresql


def upgrade():
    op.drop_column('equipments_provider', 'instances')


def downgrade():
    op.add_column('equipments_provider', sa.Column('instances', postgresql.ARRAY(sa.TEXT()), nullable=False))
