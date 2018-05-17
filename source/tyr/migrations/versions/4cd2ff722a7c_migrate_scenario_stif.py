"""
Add parameters from scenario STIF in the TYR database

Revision ID: 4cd2ff722a7c
Revises: 465a7431358a
Create Date: 2018-05-16 15:45:15.867995

"""

# revision identifiers, used by Alembic.
revision = '4cd2ff722a7c'
down_revision = '465a7431358a'

from alembic import op
import sqlalchemy as sa


def upgrade():
    op.add_column('instance', sa.Column('final_line_filter', sa.Boolean(), server_default='false', nullable=False))
    op.add_column('instance', sa.Column('max_extra_second_pass', sa.Integer(), server_default='0', nullable=False))
    op.add_column('instance', sa.Column('max_successive_physical_mode', sa.Integer(),
                                        server_default='100', nullable=False))
    op.add_column('instance', sa.Column('min_journeys_calls', sa.Integer(), server_default='1', nullable=False))
    op.add_column('instance', sa.Column('min_nb_journeys', sa.Integer(), server_default='0', nullable=False))


def downgrade():
    op.drop_column('instance', 'min_nb_journeys')
    op.drop_column('instance', 'min_journeys_calls')
    op.drop_column('instance', 'max_successive_physical_mode')
    op.drop_column('instance', 'max_extra_second_pass')
    op.drop_column('instance', 'final_line_filter')
