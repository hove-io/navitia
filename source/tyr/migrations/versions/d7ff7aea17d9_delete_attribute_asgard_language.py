"""
Delete attribute asgard_language from the table instance

Revision ID: d7ff7aea17d9
Revises: d4b48ec451ed
Create Date: 2023-10-02 14:39:53.256194

"""

# revision identifiers, used by Alembic.
revision = 'd7ff7aea17d9'
down_revision = 'd4b48ec451ed'

from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects import postgresql


def upgrade():
    op.drop_column('instance', 'asgard_language')


def downgrade():
    op.add_column(
        'instance',
        sa.Column(
            'asgard_language',
            sa.TEXT(),
            server_default=sa.text(u"'english_us'::text"),
            nullable=False,
        ),
    )
