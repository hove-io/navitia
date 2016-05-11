"""Add max_additional_changes in the table instance

Revision ID: 4fa7f3f529e
Revises: 2d56e135cd38
Create Date: 2016-05-11 10:48:32.093523

"""

# revision identifiers, used by Alembic.
revision = '4fa7f3f529e'
down_revision = '2d56e135cd38'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('instance', sa.Column('max_additional_changes', sa.Integer(), server_default='2', nullable=False))


def downgrade():
    op.drop_column('instance', 'max_additional_changes')

