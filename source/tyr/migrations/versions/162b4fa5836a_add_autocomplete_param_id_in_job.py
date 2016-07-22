"""Add autocomplete_params_id in job so that we can link job and autocomplete

Revision ID: 162b4fa5836a
Revises: d1d12707b76
Create Date: 2016-06-29 14:37:18.863221

"""

# revision identifiers, used by Alembic.
revision = '162b4fa5836a'
down_revision = 'd1d12707b76'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('job', sa.Column('autocomplete_params_id', sa.Integer(), nullable=True))
    op.create_foreign_key(
        "fk_job_autocomplete_parameter",
        "job", "autocomplete_parameter",
        ["autocomplete_params_id"], ["id"]
    )


def downgrade():
    op.drop_constraint("fk_job_autocomplete_parameter", "job")
    op.drop_column('job', 'autocomplete_params_id')
