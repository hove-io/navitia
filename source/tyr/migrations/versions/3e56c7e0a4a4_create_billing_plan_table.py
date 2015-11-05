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

    op.create_table('billing_plan',
    sa.Column('id', sa.Integer(), primary_key=True, nullable=False),
    sa.Column('name', sa.String(255), nullable=False),
    sa.Column('max_request_count', sa.Integer(), nullable=True),
    sa.Column('max_object_count', sa.Integer(), nullable=True),
    sa.Column('default', sa.Boolean(), nullable=False)
    )

    op.execute("INSERT INTO billing_plan VALUES (1,'Developpeur',3000,3000,true),(2,'Entreprise',NULL,NULL,false);")

    op.add_column(u'user', sa.Column('billing_plan_id', sa.Integer(), nullable=True))
    op.create_foreign_key(
        "fk_user_billing_plan", "user",
        "billing_plan", ["billing_plan_id"], ["id"]
    )

    op.execute('UPDATE public.user SET billing_plan_id = (SELECT id FROM billing_plan b WHERE b.default);')

def downgrade():
    op.drop_column('user', 'billing_plan_id')
    op.drop_table('billing_plan')
