"""Add streetnetwork_backend in db

Revision ID: 57f52a929056
Revises: 38cf24479595
Create Date: 2019-06-24 15:53:15.867404

"""

# revision identifiers, used by Alembic.
revision = '57f52a929056'
down_revision = '38cf24479595'

from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects import postgresql
import datetime


def upgrade():
    op.create_table(
        'streetnetwork_backend',
        sa.Column('created_at', sa.DateTime(), server_default=sa.func.now(), nullable=False),
        sa.Column('updated_at', sa.DateTime(), nullable=True),
        sa.Column('id', sa.Text(), nullable=False),
        sa.Column('klass', sa.Text(), nullable=False),
        sa.Column('discarded', sa.Boolean(), server_default='False', nullable=False),
        sa.Column('args', postgresql.JSONB(), server_default='{}', nullable=True),
        sa.PrimaryKeyConstraint('id'),
    )

    op.execute(
        "INSERT INTO streetnetwork_backend (id, klass) VALUES ('kraken','jormungandr.street_network.kraken.Kraken');"
    )

    op.add_column(
        'instance', sa.Column('street_network_car', sa.Text(), nullable=False, server_default='kraken')
    )
    op.add_column(
        'instance', sa.Column('street_network_walking', sa.Text(), nullable=False, server_default='kraken')
    )
    op.add_column(
        'instance', sa.Column('street_network_bike', sa.Text(), nullable=False, server_default='kraken')
    )
    op.add_column(
        'instance', sa.Column('street_network_bss', sa.Text(), nullable=False, server_default='kraken')
    )

    op.add_column(
        'instance',
        sa.Column('street_network_ridesharing', sa.Text(), nullable=False, server_default='ridesharingKraken'),
    )
    op.add_column(
        'instance', sa.Column('street_network_taxi', sa.Text(), nullable=False, server_default='taxiKraken')
    )

    op.create_foreign_key(
        "fk_instance_street_network_backend",
        "instance",
        "streetnetwork_backend",
        [
            "street_network_car",
            "street_network_walking",
            "street_network_bike",
            "street_network_bss",
            "street_network_ridesharing",
            "street_network_taxi",
        ],
        ["id"],
    )


def downgrade():
    op.drop_constraint('fk_instance_street_network_backend', 'instance', type_='foreignkey')

    op.drop_column('instance', 'street_network_taxi')
    op.drop_column('instance', 'street_network_ridesharing')
    op.drop_column('instance', 'street_network_bss')
    op.drop_column('instance', 'street_network_bike')
    op.drop_column('instance', 'street_network_walking')
    op.drop_column('instance', 'street_network_car')

    op.drop_table('streetnetwork_backend')
