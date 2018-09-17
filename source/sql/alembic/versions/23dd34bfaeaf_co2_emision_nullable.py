"""co2_emision_nullable

Revision ID: 23dd34bfaeaf
Revises: 795308e6d7c
Create Date: 2015-04-01 17:43:41.164959

"""

# revision identifiers, used by Alembic.
revision = '23dd34bfaeaf'
down_revision = '795308e6d7c'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.alter_column("physical_mode", "co2_emission", nullable=True, schema='navitia')
    op.execute(""" UPDATE navitia.physical_mode SET co2_emission=NULL WHERE co2_emission=-1;""")


def downgrade():
    op.execute(""" UPDATE navitia.physical_mode SET co2_emission=-1 WHERE co2_emission=NULL;""")
    op.alter_column("physical_mode", "co2_emission", nullable=False, server_default='0', schema='navitia')
