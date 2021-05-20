"""Add timeo in external_service table

Revision ID: 941b197dbd58
Revises: 1918ed13e84f
Create Date: 2021-05-20 12:26:27.292726

"""

# revision identifiers, used by Alembic.
revision = '941b197dbd58'
down_revision = '1918ed13e84f'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.execute("ALTER TYPE navitia_service_type ADD VALUE 'timeo'")


def downgrade():
    op.execute("ALTER TABLE external_service ALTER COLUMN navitia_service TYPE text")
    op.execute("DROP TYPE navitia_service_type CASCADE")
    op.execute("CREATE TYPE navitia_service_type AS ENUM ( 'free_floatings', 'vehicle_occupancies')")
    op.execute(
        "ALTER TABLE external_service ALTER COLUMN navitia_service TYPE navitia_service_type USING navitia_service::navitia_service_type"
    )
