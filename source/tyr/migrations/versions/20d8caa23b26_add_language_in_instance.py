"""
Add attribute language in the table instance

Revision ID: 20d8caa23b26
Revises: 0f57c3342d97
Create Date: 2023-08-08 11:32:32.294073

"""

# revision identifiers, used by Alembic.
revision = '20d8caa23b26'
down_revision = '0f57c3342d97'

from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects import postgresql
from navitiacommon import default_values


def upgrade():
    op.add_column(
        'instance',
        sa.Column('language', sa.Text(), server_default=default_values.language, nullable=False)
    )


def downgrade():
    op.drop_column('instance', 'language')
