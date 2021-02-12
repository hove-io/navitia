"""Add table external_service and associate it with instance

Revision ID: 064f0b16d4f6
Revises: 606a3dad91d1
Create Date: 2021-02-02 18:16:30.536536

"""

# revision identifiers, used by Alembic.
revision = '064f0b16d4f6'
down_revision = '606a3dad91d1'

from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects import postgresql

navitia_service_type = sa.Enum('free_floatings', 'vehicle_occupancies', name='navitia_service_type')


def upgrade():
    # Add table external_service
    op.create_table(
        'external_service',
        sa.Column('created_at', sa.DateTime(), nullable=False),
        sa.Column('updated_at', sa.DateTime(), nullable=True),
        sa.Column('id', sa.Text(), nullable=False),
        sa.Column('klass', sa.Text(), nullable=False),
        sa.Column('discarded', sa.Boolean(), nullable=False),
        sa.Column('navitia_service', navitia_service_type, nullable=False, server_default='free_floatings'),
        sa.Column('args', postgresql.JSONB(astext_type=sa.Text()), server_default='{}', nullable=True),
        sa.PrimaryKeyConstraint('id'),
    )
    # Add table associate_instance_external_service
    op.create_table(
        'associate_instance_external_service',
        sa.Column('instance_id', sa.Integer(), nullable=False),
        sa.Column('external_service_id', sa.Text(), nullable=False),
        sa.ForeignKeyConstraint(['external_service_id'], ['external_service.id']),
        sa.ForeignKeyConstraint(['instance_id'], ['instance.id']),
        sa.PrimaryKeyConstraint('instance_id', 'external_service_id', name='instance_external_service_pk'),
    )


def downgrade():
    # drop tables external_service and it's association with instance
    op.drop_table('associate_instance_external_service')
    op.drop_table('external_service')
    navitia_service_type.drop(op.get_bind(), checkfirst=False)
