"""add intrepreted_parameters

Revision ID: 75910df5976
Revises: 236b716d0c88
Create Date: 2015-02-02 11:21:37.028090

"""

# revision identifiers, used by Alembic.
revision = '75910df5976'
down_revision = '236b716d0c88'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.create_table(
        'interpreted_parameters',
        sa.Column('param_key', sa.Text()),
        sa.Column('param_value', sa.Text()),
        sa.Column('request_id', sa.BigInteger(), nullable=False),
        sa.ForeignKeyConstraint(['request_id'], ['stat.requests.id'],),
        schema='stat'
    )


def downgrade():
    op.drop_table('interpreted_parameters', schema='stat')
