""" Add attributes block_able and notify_threshold_list in the table billing_plan


Revision ID: efa1d64928ca
Revises: ad0fd582c1ec
Create Date: 2022-01-05 16:01:33.815393

"""

# revision identifiers, used by Alembic.
revision = 'efa1d64928ca'
down_revision = 'ad0fd582c1ec'

from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects import postgresql


def upgrade():
    op.add_column('billing_plan', sa.Column('block_able', sa.Boolean(), nullable=False, server_default="false"))
    op.add_column(
        'billing_plan',
        sa.Column('notify_threshold_list', postgresql.ARRAY(sa.Integer()), server_default='{}', nullable=True),
    )
    # Fill attributes block_able and notify_threshold_list depending on max_request_count and end_point
    op.execute(
        "update billing_plan set notify_threshold_list='{100}' "
        "where end_point_id in (select id from end_point where name = 'sncf') "
        "and max_request_count > 0"
    )
    op.execute(
        "update billing_plan set notify_threshold_list='{75,100}' "
        "where end_point_id in (select id from end_point where name = 'navitia.io') "
        "and max_request_count > 0"
    )
    op.execute(
        "update billing_plan set block_able=true where max_request_count > 0 "
        "and end_point_id in (select id from end_point where name = 'sncf')"
    )


def downgrade():
    op.drop_column('billing_plan', 'notify_threshold_list')
    op.drop_column('billing_plan', 'block_able')
