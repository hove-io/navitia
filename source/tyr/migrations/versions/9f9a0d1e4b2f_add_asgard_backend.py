"""Add asgard backend

Revision ID: 9f9a0d1e4b2f
Revises: e94705a6f9a1
Create Date: 2026-10-23
"""

from alembic import op

# revision identifiers, used by Alembic.
revision = '9f9a0d1e4b2f'
down_revision = 'e94705a6f9a1'


def upgrade():
    op.execute(
        """
        INSERT INTO streetnetwork_backend (id, klass, args)
        SELECT :id, :klass, :args::jsonb
        WHERE NOT EXISTS (
            SELECT 1 FROM streetnetwork_backend WHERE id = :id
        )
        """,
        {"id": "asgard", "klass": "jormungandr.street_network.Kraken", "args": "{}"},
    )


def downgrade():
    op.execute("DELETE FROM streetnetwork_backend WHERE id = 'asgard'")
