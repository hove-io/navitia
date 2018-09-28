"""fix default value of walking_transfer_penalty;
   set max_duration and walking_transfer_penalty to thiers default values

Revision ID: 11eae2b0e6b0
Revises: 1173ed75e96c
Create Date: 2015-12-15 09:11:55.858847

"""

# revision identifiers, used by Alembic.
revision = '11eae2b0e6b0'
down_revision = '1173ed75e96c'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.alter_column('instance', 'walking_transfer_penalty', existing_type=sa.INTEGER(), server_default='120')
    op.execute(
        '''
               update instance set max_duration='86400';
               update instance set walking_transfer_penalty='120';
               '''
    )


def downgrade():
    op.alter_column('instance', 'walking_transfer_penalty', existing_type=sa.INTEGER(), server_default='2')
