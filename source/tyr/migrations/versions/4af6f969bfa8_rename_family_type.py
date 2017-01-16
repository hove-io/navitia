""" rename autocomplete's dataset family_type

Revision ID: 4af6f969bfa8
Revises: 132cbd5a61a4
Create Date: 2017-01-16 15:11:10.661733

"""

# revision identifiers, used by Alembic.
revision = '4af6f969bfa8'
down_revision = '132cbd5a61a4'

from alembic import op


def upgrade():
    op.execute("update data_set set family_type = 'autocomplete_' || type where family_type = 'autocomplete';")


def downgrade():
    op.execute("update data_set set family_type = 'autocomplete' where family_type like 'autocomplete_%';")
