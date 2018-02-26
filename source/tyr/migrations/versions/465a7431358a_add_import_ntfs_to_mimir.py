"""empty message

Revision ID: 465a7431358a
Revises: 3acdde6329db
Create Date: 2018-02-23 14:50:23.086805

"""

# revision identifiers, used by Alembic.
revision = '465a7431358a'
down_revision = '3acdde6329db'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('instance', sa.Column('import_ntfs_in_mimir', sa.Boolean(), nullable=True))
    op.execute('update instance set import_ntfs_in_mimir=false')
    op.alter_column('instance', 'import_ntfs_in_mimir', nullable=False)


def downgrade():
    op.drop_column('instance', 'import_ntfs_in_mimir')