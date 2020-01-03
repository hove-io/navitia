"""add data_set.state

Revision ID: 2ec1da1552ec
Revises: 6f2635ce00b4
Create Date: 2019-12-30 14:51:59.149999

"""

# revision identifiers, used by Alembic.
revision = '2ec1da1552ec'
down_revision = '6f2635ce00b4'

from alembic import op
import sqlalchemy as sa


dataset_state = sa.Enum('pending', 'running', 'done', 'failed', name='dataset_state')


def upgrade():
    dataset_state.create(op.get_bind())
    op.add_column('data_set', sa.Column('state', dataset_state, nullable=True))


def downgrade():
    op.drop_column('data_set', 'state')
    dataset_state.drop(op.get_bind())
