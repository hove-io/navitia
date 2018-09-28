"""Add new tables that associate calendars and exception dates to a meta vj

Revision ID: 3b17c15627cb
Revises: 2d86200bcb93
Create Date: 2015-01-20 17:08:23.570348

"""

# revision identifiers, used by Alembic.
revision = '3b17c15627cb'
down_revision = '2d86200bcb93'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga
from sqlalchemy.dialects import postgresql


def upgrade():
    op.create_table(
        'associated_calendar',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('calendar_id', sa.BIGINT(), nullable=False),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.ForeignKeyConstraint(
            ['calendar_id'], [u'navitia.calendar.id'], name=u'associated_calendar_calendar_fkey'
        ),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'associated_exception_date',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('datetime', sa.DATE(), nullable=False),
        sa.Column('type_ex', postgresql.ENUM(u'Add', u'Sub', name='associated_exception_type'), nullable=False),
        sa.Column('associated_calendar_id', sa.BIGINT(), nullable=False),
        sa.ForeignKeyConstraint(
            ['associated_calendar_id'],
            [u'navitia.associated_calendar.id'],
            name=u'associated_exception_date_associated_calendar_fkey',
        ),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'rel_metavj_associated_calendar',
        sa.Column('meta_vj_id', sa.BIGINT(), nullable=True),
        sa.Column('associated_calendar_id', sa.BIGINT(), nullable=True),
        sa.ForeignKeyConstraint(
            ['meta_vj_id'], [u'navitia.meta_vj.id'], name=u'rel_metavj_associated_calendar_meta_vj_fkey'
        ),
        sa.ForeignKeyConstraint(
            ['associated_calendar_id'],
            [u'navitia.associated_calendar.id'],
            name=u'rel_metavj_associated_calendar_associated_calendar_fkey',
        ),
        schema='navitia',
    )


def downgrade():
    op.drop_table('rel_metavj_associated_calendar', schema='navitia')
    op.drop_table('associated_exception_date', schema='navitia')
    op.drop_table('associated_calendar', schema='navitia')
