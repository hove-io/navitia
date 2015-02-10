"""add_filter

Revision ID: 2c90cc5083e6
Revises: d8cff35d69
Create Date: 2015-02-09 15:55:23.698184

"""

# revision identifiers, used by Alembic.
revision = '2c90cc5083e6'
down_revision = 'd8cff35d69'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.execute('''CREATE SEQUENCE stat.interpred_parameter_seq;''')
    op.execute('''ALTER TABLE stat.interpreted_parameters ADD COLUMN id BIGINT DEFAULT nextval('stat.interpred_parameter_seq') PRIMARY KEY;''')

    op.create_table(
        'filter',
        sa.Column('object', sa.Text()),
        sa.Column('attribute', sa.Text()),
        sa.Column('operator', sa.Text()),
        sa.Column('value', sa.Text()),
        sa.Column('interpreted_parameter_id', sa.BigInteger(), nullable=False),
        sa.ForeignKeyConstraint(['interpreted_parameter_id'], ['stat.interpreted_parameters.id'],),
        schema='stat'
    )


def downgrade():
    op.drop_table('filter', schema='stat')

    op.execute('''ALTER TABLE stat.interpreted_parameters DROP COLUMN id;''')
    op.execute('''DROP SEQUENCE stat.interpred_parameter_seq;''')
