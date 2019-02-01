"""Remove scenario destineo and default

Revision ID: 19be8e1cd586
Revises: 3f620c440544
Create Date: 2018-12-03 11:27:55.994614

"""

# revision identifiers, used by Alembic.
revision = '19be8e1cd586'
down_revision = '3f620c440544'

from alembic import op
import sqlalchemy as sa
from sqlalchemy.dialects import postgresql


def upgrade():
    # It's already the default value for tyr, but it's better to cleanup
    op.alter_column('instance', 'scenario', server_default='new_default')

    op.execute("update instance set scenario='new_default' where scenario in ('destineo', 'default');")

    op.drop_column('instance', 'min_tc_with_bss')
    op.drop_column('instance', 'min_tc_with_car')
    op.drop_column('instance', 'min_tc_with_bike')
    op.drop_column('instance', 'max_duration_criteria')
    op.drop_column('instance', 'max_duration_fallback_mode')
    op.drop_column('instance', 'min_duration_too_long_journey')
    op.drop_column('instance', 'factor_too_long_journey')


def downgrade():
    op.add_column('instance', sa.Column('min_tc_with_bike', sa.INTEGER(), server_default='300', nullable=False))
    op.add_column('instance', sa.Column('min_tc_with_car', sa.INTEGER(), server_default='300', nullable=False))
    op.add_column('instance', sa.Column('min_tc_with_bss', sa.INTEGER(), server_default='300', nullable=False))
    op.add_column(
        'instance',
        sa.Column(
            'factor_too_long_journey',
            postgresql.DOUBLE_PRECISION(precision=53),
            server_default='4',
            nullable=False,
        ),
    )
    op.add_column(
        'instance',
        sa.Column('min_duration_too_long_journey', sa.INTEGER(), server_default='900', nullable=False),
    )
    op.add_column(
        'instance',
        sa.Column(
            'max_duration_fallback_mode',
            postgresql.ENUM(u'walking', u'bss', u'bike', u'car', name='max_duration_fallback_mode'),
            server_default="walking",
            nullable=False,
        ),
    )
    op.add_column(
        'instance',
        sa.Column(
            'max_duration_criteria',
            postgresql.ENUM(u'time', u'duration', name='max_duration_criteria'),
            server_default="time",
            nullable=False,
        ),
    )
