"""Add attribute ghost_words in the table instance

Revision ID: 75681b9c74aa
Revises: 45b018b3198b
Create Date: 2022-11-15 09:24:40.871668

"""

# revision identifiers, used by Alembic.
revision = '75681b9c74aa'
down_revision = '45b018b3198b'

from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects import postgresql


def upgrade():
    op.add_column(
        'instance', sa.Column('ghost_words', postgresql.ARRAY(sa.Text()), server_default='{}', nullable=True)
    )


def downgrade():
    op.drop_column('instance', 'ghost_words')
