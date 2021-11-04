"""Add new column config_es7_toml

Revision ID: 72e59a9b184a
Revises: 9c9e9706c049
Create Date: 2021-11-02 17:57:13.483520

"""

# revision identifiers, used by Alembic.
revision = '72e59a9b184a'
down_revision = '9c9e9706c049'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('autocomplete_parameter', sa.Column('config_es7_toml', sa.Text(), nullable=True))


def downgrade():
    op.drop_column('autocomplete_parameter', 'config_es7_toml')
