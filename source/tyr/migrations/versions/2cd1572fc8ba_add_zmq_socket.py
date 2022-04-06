""" Add zmq_socket per instance

Revision ID: 2cd1572fc8ba
Revises: 535dfe5be550
Create Date: 2022-04-06 10:28:59.181769

"""

# revision identifiers, used by Alembic.
revision = '2cd1572fc8ba'
down_revision = '535dfe5be550'

from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects import postgresql

from navitiacommon import default_values


def upgrade():
    op.add_column(
        'instance', sa.Column('zmq_socket', sa.Text(), server_default=default_values.zmq_socket, nullable=False)
    )


def downgrade():
    op.drop_column('instance', 'zmq_socket')
