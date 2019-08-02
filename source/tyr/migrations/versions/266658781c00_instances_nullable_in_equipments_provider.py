"""
column 'instances' will be deleted later. Has to be nullable for transition

Revision ID: 266658781c00
Revises: 204aae05372a
Create Date: 2019-04-15 16:27:22.362244

"""

# revision identifiers, used by Alembic.
revision = '266658781c00'
down_revision = '204aae05372a'

from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects import postgresql


def upgrade():
    op.alter_column('equipments_provider', 'instances', existing_type=postgresql.ARRAY(sa.TEXT()), nullable=True)


def downgrade():
    op.execute("UPDATE equipments_provider SET instances = '{null_instance}';")
    op.alter_column(
        'equipments_provider', 'instances', existing_type=postgresql.ARRAY(sa.TEXT()), nullable=False
    )
