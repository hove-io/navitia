"""change max_nb_crowfly_by_mode to JSONB and set server_default

Revision ID: 1b59ffb421e4
Revises: 483639f1f00
Create Date: 2018-08-30 12:42:21.089095

"""

# revision identifiers, used by Alembic.
revision = '1b59ffb421e4'
down_revision = '483639f1f00'

from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects.postgresql import JSONB
from navitiacommon import default_values
import json

def upgrade():
    op.drop_column('instance', 'max_nb_crowfly_by_mode')
    op.add_column('instance', sa.Column('max_nb_crowfly_by_mode', JSONB,
                                        server_default=json.dumps(default_values.max_nb_crowfly_by_mode)))


def downgrade():
    op.drop_column('instance', 'max_nb_crowfly_by_mode')
    op.add_column('instance', sa.Column('max_nb_crowfly_by_mode', sa.PickleType(pickler=json), nullable=True))

