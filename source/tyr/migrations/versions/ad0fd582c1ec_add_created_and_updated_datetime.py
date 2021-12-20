"""
Add created_at and updated at in the following tables
 - user
 - billing_plan
 - key

 Add blocked_at in the table user

Revision ID: ad0fd582c1ec
Revises: 7a77ccf8f94e
Create Date: 2021-12-14 14:26:09.599630

"""

# revision identifiers, used by Alembic.
revision = 'ad0fd582c1ec'
down_revision = '7a77ccf8f94e'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('billing_plan', sa.Column('created_at', sa.DateTime(), nullable=True))
    op.add_column('billing_plan', sa.Column('updated_at', sa.DateTime(), nullable=True))
    op.add_column('key', sa.Column('created_at', sa.DateTime(), nullable=True))
    op.add_column('key', sa.Column('updated_at', sa.DateTime(), nullable=True))
    op.add_column('user', sa.Column('blocked_at', sa.DateTime(), nullable=True))
    op.add_column('user', sa.Column('created_at', sa.DateTime(), nullable=True))
    op.add_column('user', sa.Column('updated_at', sa.DateTime(), nullable=True))
    op.execute("UPDATE billing_plan SET created_at = '2021-01-01 00:00:00'")
    op.execute("UPDATE key SET created_at = '2021-01-01 00:00:00'")
    op.execute("UPDATE \"user\" SET created_at = '2021-01-01 00:00:00'")
    op.execute("ALTER TABLE billing_plan ALTER COLUMN created_at SET DEFAULT CURRENT_TIMESTAMP")
    op.execute("ALTER TABLE billing_plan ALTER COLUMN created_at SET NOT NULL")
    op.execute("ALTER TABLE key ALTER COLUMN created_at SET DEFAULT CURRENT_TIMESTAMP")
    op.execute("ALTER TABLE key ALTER COLUMN created_at SET NOT NULL")
    op.execute("ALTER TABLE \"user\" ALTER COLUMN created_at SET DEFAULT CURRENT_TIMESTAMP")
    op.execute("ALTER TABLE \"user\" ALTER COLUMN created_at SET NOT NULL")


def downgrade():
    op.drop_column('user', 'updated_at')
    op.drop_column('user', 'created_at')
    op.drop_column('user', 'blocked_at')
    op.drop_column('key', 'updated_at')
    op.drop_column('key', 'created_at')
    op.drop_column('billing_plan', 'updated_at')
    op.drop_column('billing_plan', 'created_at')
