"""journey pattern suppression

Revision ID: 1651f9b235f5
Revises: 2e1a3cb6555b
Create Date: 2015-09-29 10:30:04.072190

"""

# revision identifiers, used by Alembic.
revision = '1651f9b235f5'
down_revision = '2e1a3cb6555b'

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga


def upgrade():
    op.drop_constraint('journey_pattern_route_id_fkey', 'journey_pattern', schema='navitia')
    op.drop_constraint('journey_pattern_physical_mode_id_fkey', 'journey_pattern', schema='navitia')
    op.drop_constraint('vehicle_journey_journey_pattern_id_fkey', 'vehicle_journey', schema='navitia')
    op.drop_constraint('stop_time_journey_pattern_point_id_fkey', 'stop_time', schema='navitia')
    op.drop_table('journey_pattern_point', schema='navitia')
    op.drop_table('journey_pattern', schema='navitia')
    op.add_column('stop_time', sa.Column('order', sa.INTEGER(), nullable=True), schema='navitia')
    op.add_column(
        'stop_time',
        sa.Column(
            'shape_from_prev',
            ga.Geography(geometry_type='LINESTRING', srid=4326, spatial_index=False),
            nullable=True,
        ),
        schema='navitia',
    )
    op.add_column('stop_time', sa.Column('stop_point_id', sa.BIGINT(), nullable=True), schema='navitia')
    op.drop_column('stop_time', 'journey_pattern_point_id', schema='navitia')
    op.add_column('vehicle_journey', sa.Column('physical_mode_id', sa.BIGINT(), nullable=True), schema='navitia')
    op.add_column('vehicle_journey', sa.Column('route_id', sa.BIGINT(), nullable=True), schema='navitia')
    op.drop_column('vehicle_journey', 'journey_pattern_id', schema='navitia')


def downgrade():
    op.add_column(
        'vehicle_journey',
        sa.Column('journey_pattern_id', sa.BIGINT(), autoincrement=False, nullable=False),
        schema='navitia',
    )
    op.drop_column('vehicle_journey', 'route_id', schema='navitia')
    op.drop_column('vehicle_journey', 'physical_mode_id', schema='navitia')
    op.add_column(
        'stop_time',
        sa.Column('journey_pattern_point_id', sa.BIGINT(), autoincrement=False, nullable=False),
        schema='navitia',
    )
    op.drop_column('stop_time', 'stop_point_id', schema='navitia')
    op.drop_column('stop_time', 'shape_from_prev', schema='navitia')
    op.drop_column('stop_time', 'order', schema='navitia')
    op.create_table(
        'journey_pattern',
        sa.Column('id', BIGINT(), primary_key=True, nullable=False),
        sa.Column('route_id', BIGINT(), primary_key=False, nullable=False),
        sa.Column('physical_mode_id', BIGINT(), primary_key=False, nullable=False),
        sa.Column('uri', TEXT(), primary_key=False, nullable=False),
        sa.Column('name', TEXT(), primary_key=False, nullable=False),
        sa.Column('is_frequence', BOOLEAN(), primary_key=False, nullable=False),
        sa.ForeignKeyConstraint(['route_id'], [u'navitia.route.id'], name=u'journey_pattern_route_id_fkey'),
        sa.ForeignKeyConstraint(
            ['physical_mode_id'], [u'navitia.physical_mode.id'], name=u'journey_pattern_physical_mode_id_fkey'
        ),
        schema='navitia',
    )
    op.create_table(
        'journey_pattern_point',
        sa.Column('id', BIGINT(), primary_key=True, nullable=False),
        sa.Column('journey_pattern_id', BIGINT(), primary_key=False, nullable=False),
        sa.Column('name', TEXT(), primary_key=False, nullable=False),
        sa.Column('uri', TEXT(), primary_key=False, nullable=False),
        sa.Column('order', INTEGER(), primary_key=False, nullable=False),
        sa.Column('stop_point_id', BIGINT(), primary_key=False, nullable=False),
        sa.Column(
            'shape_from_prev',
            Geography(geometry_type='LINESTRING', srid=4326, spatial_index=False),
            primary_key=False,
        ),
        sa.ForeignKeyConstraint(
            ['stop_point_id'], [u'navitia.stop_point.id'], name=u'journey_pattern_point_stop_point_id_fkey'
        ),
        sa.ForeignKeyConstraint(
            ['journey_pattern_id'],
            [u'navitia.journey_pattern.id'],
            name=u'journey_pattern_point_journey_pattern_id_fkey',
        ),
        schema='navitia',
    )
