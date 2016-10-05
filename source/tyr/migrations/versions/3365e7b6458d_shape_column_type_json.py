"""empty message

Revision ID: 3365e7b6458d
Revises: 44dab62dfff7
Create Date: 2016-10-05 11:44:51.763828

"""

# revision identifiers, used by Alembic.
revision = '3365e7b6458d'
down_revision = '44dab62dfff7'

from alembic import op


def upgrade():
    op.execute('ALTER TABLE "user" ALTER COLUMN shape TYPE JSON USING shape::json')


def downgrade():
    op.execute('ALTER TABLE "user" ALTER COLUMN shape TYPE TEXT')
