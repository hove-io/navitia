"""import stops in global autocomplete (mimir)

Revision ID: 361f3993960e
Revises: 57ee5581cc30
Create Date: 2016-12-23 11:48:47.003811

"""

# revision identifiers, used by Alembic.
revision = '361f3993960e'
down_revision = '502b7c6fd00b'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('instance', sa.Column('import_stops_in_mimir', sa.Boolean(), nullable=True))
    op.execute('update instance set import_stops_in_mimir=false')
    op.alter_column('instance', 'import_stops_in_mimir', nullable=False)


def downgrade():
    op.drop_column('instance', 'import_stops_in_mimir')
