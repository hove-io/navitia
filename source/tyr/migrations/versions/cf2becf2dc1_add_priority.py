""" Add an attribut priority in the table instance

Revision ID: cf2becf2dc1
Revises: 64854ef94b3
Create Date: 2016-01-28 16:11:33.430012

"""

# revision identifiers, used by Alembic.
revision = 'cf2becf2dc1'
down_revision = '64854ef94b3'

from alembic import op
import sqlalchemy as sa


def upgrade():
    ### Attribut priority added in the table instance! ###
    op.add_column('instance', sa.Column('priority', sa.Integer(), server_default='0', nullable=False))
    ### end Alembic commands ###


def downgrade():
    ### Delete attribut priority in the table instance! ###
    op.drop_column('instance', 'priority')
    ### end Alembic commands ###
