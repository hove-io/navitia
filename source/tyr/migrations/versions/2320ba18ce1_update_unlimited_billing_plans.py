"""Update unlimited billing plans.

Revision ID: 2320ba18ce1
Revises: 962cfcce76f
Create Date: 2015-12-17 11:58:42.675422

"""

# revision identifiers, used by Alembic.
revision = '2320ba18ce1'
down_revision = '962cfcce76f'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.execute("update billing_plan set max_request_count=0 where max_request_count IS NULL")
    op.execute("update billing_plan set max_object_count=0 where max_object_count IS NULL")
    op.alter_column('billing_plan', 'max_request_count', server_default='0')
    op.alter_column('billing_plan', 'max_object_count', server_default='0')


def downgrade():
    op.execute("update billing_plan set max_request_count=NULL where max_request_count=0")
    op.execute("update billing_plan set max_object_count=NULL where max_object_count=0")
    op.alter_column('billing_plan', 'max_request_count')
    op.alter_column('billing_plan', 'max_object_count')
