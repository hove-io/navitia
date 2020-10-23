"""Add RidesharingService table

Revision ID: 061f7f29d76d
Revises: 5127fcffb248
Create Date: 2020-10-07 15:13:00.279814

"""

from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects import postgresql

# revision identifiers, used by Alembic.
revision = '061f7f29d76d'
down_revision = '5127fcffb248'


def upgrade():
    op.create_table(
        'ridesharing_service',
        sa.Column('created_at', sa.DateTime(), nullable=False),
        sa.Column('updated_at', sa.DateTime(), nullable=True),
        sa.Column('id', sa.Text(), nullable=False),
        sa.Column('klass', sa.Text(), nullable=False),
        sa.Column('discarded', sa.Boolean(), nullable=False, server_default='false'),
        sa.Column('args', postgresql.JSONB(astext_type=sa.Text()), server_default='{}', nullable=True),
        sa.PrimaryKeyConstraint('id'),
    )
    op.create_table(
        'associate_instance_ridesharing',
        sa.Column('instance_id', sa.Integer(), nullable=False),
        sa.Column('ridesharing_id', sa.Text(), nullable=False),
        sa.ForeignKeyConstraint(['instance_id'], ['instance.id']),
        sa.ForeignKeyConstraint(['ridesharing_id'], ['ridesharing_service.id']),
        sa.PrimaryKeyConstraint('instance_id', 'ridesharing_id', name='instance_ridesharing_pk'),
    )


def downgrade():
    op.drop_table('associate_instance_ridesharing')
    op.drop_table('ridesharing_service')
