"""Create billing_plan table

Revision ID: 3e56c7e0a4a4
Revises: 3aaddd5707bd
Create Date: 2015-11-05 13:30:32.460413

"""

revision = '3e56c7e0a4a4'
down_revision = '3aaddd5707bd'

from alembic import op, context
import sqlalchemy as sa
from sqlalchemy.dialects import postgresql
from sqlalchemy.types import Enum


def upgrade():

    op.create_table(
        'billing_plan',
        sa.Column('id', sa.Integer(), primary_key=True, nullable=False),
        sa.Column('name', sa.Text(), nullable=False),
        sa.Column('max_request_count', sa.Integer(), nullable=True),
        sa.Column('max_object_count', sa.Integer(), nullable=True),
        sa.Column('default', sa.Boolean(), nullable=False),
        sa.Column('end_point_id', sa.Integer(), nullable=False, server_default='1'),
    )

    op.create_foreign_key("fk_billing_plan_end_point", "billing_plan", "end_point", ["end_point_id"], ["id"])

    op.execute(
        "INSERT INTO end_point (name, \"default\") SELECT 'sncf',false WHERE NOT EXISTS (SELECT id FROM end_point WHERE name = 'sncf');"
    )
    op.execute(
        "INSERT INTO billing_plan (name, max_request_count, max_object_count, \"default\", end_point_id) VALUES ('nav_dev',3000,NULL,false,1),('nav_ent',NULL,NULL,false,1),('nav_ctp',NULL,NULL,true,1),('sncf_dev',3000,60000,true,(SELECT id FROM end_point WHERE name='sncf')),('sncf_ent',NULL,NULL,false,(SELECT id FROM end_point WHERE name='sncf'));"
    )

    op.add_column(u'user', sa.Column('billing_plan_id', sa.Integer(), nullable=True))
    op.create_foreign_key("fk_user_billing_plan", "user", "billing_plan", ["billing_plan_id"], ["id"])

    op.execute(
        "UPDATE public.user SET billing_plan_id = (SELECT b.id FROM billing_plan b WHERE b.default AND end_point_id=1) WHERE end_point_id=1;"
    )
    op.execute(
        "UPDATE public.user u SET billing_plan_id = (SELECT b.id FROM billing_plan b INNER JOIN end_point ep ON ep.id = b.end_point_id WHERE b.default AND ep.name='sncf') FROM end_point ep WHERE ep.id = u.end_point_id AND ep.name='sncf';"
    )


def downgrade():
    op.drop_column('user', 'billing_plan_id')
    op.drop_table('billing_plan')
