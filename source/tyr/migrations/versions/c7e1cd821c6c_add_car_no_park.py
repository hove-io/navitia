"""add car_no_park attributes

Revision ID: c7e1cd821c6c
Revises: 2ec1da1552ec
Create Date: 2020-05-11 17:55:17.032937

"""

# revision identifiers, used by Alembic.
revision = 'c7e1cd821c6c'
down_revision = '2ec1da1552ec'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column(
        'instance', sa.Column('street_network_car_no_park', sa.Text(), nullable=False, server_default='kraken')
    )

    op.add_column(
        'instance',
        sa.Column('max_car_no_park_direct_path_duration', sa.Integer(), server_default='86400', nullable=False),
    )

    op.create_foreign_key(
        "fk_instance_street_network_backend_car_no_park",
        "instance",
        "streetnetwork_backend",
        ["street_network_car_no_park"],
        ["id"],
    )


def downgrade():

    op.drop_constraint("fk_instance_street_network_backend_car_no_park", 'instance', type_='foreignkey')

    op.drop_column('instance', 'street_network_car_no_park')
    op.drop_column('instance', 'max_car_no_park_direct_path_duration')
    # ### end Alembic commands ###
