"""modify_id_type

Revision ID: 236b716d0c88
Revises: 176f17297061
Create Date: 2014-06-10 11:30:56.113410

"""

# revision identifiers, used by Alembic.
revision = '236b716d0c88'
down_revision = '176f17297061'

from alembic import op
import sqlalchemy as sa


def upgrade():
    alter_requests_id_BigInt()
    alter_journeys_id_BigInt()
    alter_journey_sections_id_BigInt()

def downgrade():
    alter_requests_id_Int()
    alter_journeys_id_Int()
    alter_journey_sections_id_Int()

def alter_requests_id_BigInt():
    op.alter_column(
        'requests',
        'id',
        type_=sa.BigInteger(),
        existing_type=sa.Integer(),
        schema='stat'
    )

def alter_journeys_id_BigInt():
    op.alter_column(
        'journeys',
        'id',
        type_=sa.BigInteger(),
        existing_type=sa.Integer(),
        schema='stat'
    )

def alter_journey_sections_id_BigInt():
    op.alter_column(
        'journey_sections',
        'id',
        type_=sa.BigInteger(),
        existing_type=sa.Integer(),
        schema='stat'
    )

def alter_requests_id_Int():
    op.alter_column(
        'requests',
        'id',
        type_=sa.Integer(),
        existing_type=sa.BigInteger(),
        schema='stat'
    )

def alter_journeys_id_Int():
    op.alter_column(
        'journeys',
        'id',
        type_=sa.Integer(),
        existing_type=sa.BigInteger(),
        schema='stat'
    )

def alter_journey_sections_id_Int():
    op.alter_column(
        'journey_sections',
        'id',
        type_=sa.Integer(),
        existing_type=sa.BigInteger(),
        schema='stat'
    )