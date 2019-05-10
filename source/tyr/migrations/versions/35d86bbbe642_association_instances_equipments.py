"""
Association table between instances and equipments providers
Remove unused column 'instances' in table 'equipments_provider'

Revision ID: 35d86bbbe642
Revises: 266658781c00
Create Date: 2019-04-25 15:09:21.276526

"""

# revision identifiers, used by Alembic.
revision = '35d86bbbe642'
down_revision = '266658781c00'

from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects import postgresql


def upgrade():
    op.create_table(
        'associate_instance_equipments',
        sa.Column('instance_id', sa.Integer(), nullable=False),
        sa.Column('equipments_id', sa.Text(), nullable=False),
        sa.ForeignKeyConstraint(['equipments_id'], ['equipments_provider.id']),
        sa.ForeignKeyConstraint(['instance_id'], ['instance.id']),
        sa.PrimaryKeyConstraint('instance_id', 'equipments_id', name='instance_equipments_pk'),
    )
    op.drop_column(u'equipments_provider', 'instances')


def downgrade():
    op.add_column(u'equipments_provider', sa.Column('instances', postgresql.ARRAY(sa.TEXT()), nullable=True))
    op.drop_table('associate_instance_equipments')
