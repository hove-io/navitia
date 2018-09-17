"""Create tables

Revision ID: 35c20e6103d6
Revises: None
Create Date: 2014-09-24 16:57:34.319952

"""

# revision identifiers, used by Alembic.
revision = '35c20e6103d6'
down_revision = None

from alembic import op
import sqlalchemy as sa
import geoalchemy2 as ga
from sqlalchemy.dialects import postgresql


def upgrade():
    op.execute("CREATE SCHEMA navitia;")
    op.execute("CREATE SCHEMA georef;")
    op.execute("CREATE SCHEMA realtime;")
    op.create_table(
        'connection_type',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'vehicle_properties',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('wheelchair_accessible', sa.BOOLEAN(), nullable=False),
        sa.Column('bike_accepted', sa.BOOLEAN(), nullable=False),
        sa.Column('air_conditioned', sa.BOOLEAN(), nullable=False),
        sa.Column('visual_announcement', sa.BOOLEAN(), nullable=False),
        sa.Column('audible_announcement', sa.BOOLEAN(), nullable=False),
        sa.Column('appropriate_escort', sa.BOOLEAN(), nullable=False),
        sa.Column('appropriate_signage', sa.BOOLEAN(), nullable=False),
        sa.Column('school_vehicle', sa.BOOLEAN(), nullable=False),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'parameters',
        sa.Column('beginning_date', sa.DATE(), nullable=True),
        sa.Column('end_date', sa.DATE(), nullable=True),
        sa.Column('timezone', sa.TEXT(), nullable=True),
        sa.Column('shape', ga.Geography(geometry_type='POLYGON', srid=4326, spatial_index=False), nullable=True),
        sa.Column('shape_computed', sa.BOOLEAN(), nullable=True, server_default="TRUE"),
        schema='navitia',
    )
    op.create_table(
        'meta_vj',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'commercial_mode',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('uri', sa.TEXT(), nullable=False),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'properties',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('wheelchair_boarding', sa.BOOLEAN(), nullable=False),
        sa.Column('sheltered', sa.BOOLEAN(), nullable=False),
        sa.Column('elevator', sa.BOOLEAN(), nullable=False),
        sa.Column('escalator', sa.BOOLEAN(), nullable=False),
        sa.Column('bike_accepted', sa.BOOLEAN(), nullable=False),
        sa.Column('bike_depot', sa.BOOLEAN(), nullable=False),
        sa.Column('visual_announcement', sa.BOOLEAN(), nullable=False),
        sa.Column('audible_announcement', sa.BOOLEAN(), nullable=False),
        sa.Column('appropriate_escort', sa.BOOLEAN(), nullable=False),
        sa.Column('appropriate_signage', sa.BOOLEAN(), nullable=False),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'poi_type',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('uri', sa.TEXT(), nullable=False),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.PrimaryKeyConstraint('id'),
        schema='georef',
    )
    op.create_table(
        'connection_kind',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'network',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('comment', sa.TEXT(), nullable=True),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.Column('uri', sa.TEXT(), nullable=False),
        sa.Column('external_code', sa.TEXT(), nullable=False),
        sa.Column('sort', sa.INTEGER(), nullable=False),
        sa.Column('website', sa.TEXT(), nullable=True),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'admin',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.Column('comment', sa.TEXT(), nullable=True),
        sa.Column('post_code', sa.TEXT(), nullable=True),
        sa.Column('insee', sa.TEXT(), nullable=True),
        sa.Column('level', sa.INTEGER(), nullable=False),
        sa.Column('coord', ga.Geography(geometry_type='POINT', srid=4326, spatial_index=False), nullable=True),
        sa.Column(
            'boundary', ga.Geography(geometry_type='MULTIPOLYGON', srid=4326, spatial_index=False), nullable=True
        ),
        sa.Column('uri', sa.TEXT(), nullable=False),
        sa.PrimaryKeyConstraint('id'),
        schema='georef',
    )
    op.create_table(
        'ticket',
        sa.Column('ticket_key', sa.TEXT(), nullable=False),
        sa.Column('ticket_title', sa.TEXT(), nullable=True),
        sa.Column('ticket_comment', sa.TEXT(), nullable=True),
        sa.PrimaryKeyConstraint('ticket_key'),
        schema='navitia',
    )
    op.create_table(
        'week_pattern',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('monday', sa.BOOLEAN(), nullable=False),
        sa.Column('tuesday', sa.BOOLEAN(), nullable=False),
        sa.Column('wednesday', sa.BOOLEAN(), nullable=False),
        sa.Column('thursday', sa.BOOLEAN(), nullable=False),
        sa.Column('friday', sa.BOOLEAN(), nullable=False),
        sa.Column('saturday', sa.BOOLEAN(), nullable=False),
        sa.Column('sunday', sa.BOOLEAN(), nullable=False),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'way',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.Column('uri', sa.TEXT(), nullable=False),
        sa.Column('type', sa.TEXT(), nullable=True),
        sa.PrimaryKeyConstraint('id'),
        schema='georef',
    )
    op.create_table(
        'physical_mode',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('uri', sa.TEXT(), nullable=False),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'node',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('coord', ga.Geography(geometry_type='POINT', srid=4326, spatial_index=False), nullable=True),
        sa.PrimaryKeyConstraint('id'),
        schema='georef',
    )
    op.create_table(
        'company',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('comment', sa.TEXT(), nullable=True),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.Column('uri', sa.TEXT(), nullable=False),
        sa.Column('address_name', sa.TEXT(), nullable=True),
        sa.Column('address_number', sa.TEXT(), nullable=True),
        sa.Column('address_type_name', sa.TEXT(), nullable=True),
        sa.Column('phone_number', sa.TEXT(), nullable=True),
        sa.Column('mail', sa.TEXT(), nullable=True),
        sa.Column('website', sa.TEXT(), nullable=True),
        sa.Column('fax', sa.TEXT(), nullable=True),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'message_status',
        sa.Column('id', sa.INTEGER(), nullable=False),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.PrimaryKeyConstraint('id'),
        schema='realtime',
    )
    op.create_table(
        'contributor',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('uri', sa.TEXT(), nullable=False),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'odt_type',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    fare_od_mode = postgresql.ENUM(u'Zone', u'StopArea', u'Mode', name='fare_od_mode')
    op.create_table(
        'origin_destination',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('origin_id', sa.TEXT(), nullable=False),
        sa.Column('origin_mode', fare_od_mode, nullable=False),
        sa.Column('destination_id', sa.TEXT(), nullable=False),
        sa.Column('destination_mode', fare_od_mode, nullable=False),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'synonym',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('key', sa.TEXT(), nullable=False),
        sa.Column('value', sa.TEXT(), nullable=False),
        sa.PrimaryKeyConstraint('id'),
        schema='georef',
    )
    op.create_table(
        'object_type',
        sa.Column('id', sa.INTEGER(), nullable=False),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'validity_pattern',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('days', postgresql.BIT(length=400, varying=True), nullable=False),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'calendar',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('external_code', sa.TEXT(), nullable=False),
        sa.Column('uri', sa.TEXT(), nullable=False),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.Column('week_pattern_id', sa.BIGINT(), nullable=False),
        sa.ForeignKeyConstraint(
            ['week_pattern_id'], [u'navitia.week_pattern.id'], name=u'calendar_week_pattern_id_fkey'
        ),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'dated_ticket',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('ticket_id', sa.TEXT(), nullable=True),
        sa.Column('valid_from', sa.DATE(), nullable=False),
        sa.Column('valid_to', sa.DATE(), nullable=False),
        sa.Column('ticket_price', sa.INTEGER(), nullable=False),
        sa.Column('comments', sa.TEXT(), nullable=True),
        sa.Column('currency', sa.TEXT(), nullable=True),
        sa.ForeignKeyConstraint(
            ['ticket_id'], [u'navitia.ticket.ticket_key'], name=u'dated_ticket_ticket_id_fkey'
        ),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'message',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('uri', sa.TEXT(), nullable=False),
        sa.Column('start_publication_date', postgresql.TIMESTAMP(timezone=True), nullable=False),
        sa.Column('end_publication_date', postgresql.TIMESTAMP(timezone=True), nullable=False),
        sa.Column('start_application_date', postgresql.TIMESTAMP(timezone=True), nullable=False),
        sa.Column('end_application_date', postgresql.TIMESTAMP(timezone=True), nullable=False),
        sa.Column('start_application_daily_hour', postgresql.TIME(timezone=True), nullable=False),
        sa.Column('end_application_daily_hour', postgresql.TIME(timezone=True), nullable=False),
        sa.Column('active_days', postgresql.BIT(length=8), nullable=False),
        sa.Column('object_uri', sa.TEXT(), nullable=False),
        sa.Column('object_type_id', sa.INTEGER(), nullable=False),
        sa.Column('message_status_id', sa.INTEGER(), nullable=False),
        sa.Column('is_active', sa.BOOLEAN(), nullable=False),
        sa.ForeignKeyConstraint(
            ['message_status_id'], [u'realtime.message_status.id'], name=u'message_message_status_id_fkey'
        ),
        sa.ForeignKeyConstraint(
            ['object_type_id'], [u'navitia.object_type.id'], name=u'message_object_type_id_fkey'
        ),
        sa.PrimaryKeyConstraint('id'),
        schema='realtime',
    )
    op.create_table(
        'edge',
        sa.Column('source_node_id', sa.BIGINT(), nullable=False),
        sa.Column('target_node_id', sa.BIGINT(), nullable=False),
        sa.Column('way_id', sa.BIGINT(), nullable=False),
        sa.Column(
            'the_geog', ga.Geography(geometry_type='LINESTRING', srid=4326, spatial_index=False), nullable=False
        ),
        sa.Column('pedestrian_allowed', sa.BOOLEAN(), nullable=False),
        sa.Column('cycles_allowed', sa.BOOLEAN(), nullable=False),
        sa.Column('cars_allowed', sa.BOOLEAN(), nullable=False),
        sa.ForeignKeyConstraint(['source_node_id'], [u'georef.node.id'], name=u'edge_source_node_id_fkey'),
        sa.ForeignKeyConstraint(['target_node_id'], [u'georef.node.id'], name=u'edge_target_node_id_fkey'),
        schema='georef',
    )
    op.create_table(
        'od_ticket',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('od_id', sa.BIGINT(), nullable=False),
        sa.Column('ticket_id', sa.TEXT(), nullable=False),
        sa.ForeignKeyConstraint(['od_id'], [u'navitia.origin_destination.id'], name=u'od_ticket_od_id_fkey'),
        sa.ForeignKeyConstraint(['ticket_id'], [u'navitia.ticket.ticket_key'], name=u'od_ticket_ticket_id_fkey'),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'rel_admin_admin',
        sa.Column('master_admin_id', sa.BIGINT(), nullable=False),
        sa.Column('admin_id', sa.BIGINT(), nullable=False),
        sa.ForeignKeyConstraint(['admin_id'], [u'georef.admin.id'], name=u'rel_admin_admin_admin_id_fkey'),
        sa.ForeignKeyConstraint(
            ['master_admin_id'], [u'georef.admin.id'], name=u'rel_admin_admin_master_admin_id_fkey'
        ),
        sa.PrimaryKeyConstraint('master_admin_id', 'admin_id'),
        schema='georef',
    )
    op.create_table(
        'rel_way_admin',
        sa.Column('admin_id', sa.BIGINT(), nullable=False),
        sa.Column('way_id', sa.BIGINT(), nullable=False),
        sa.ForeignKeyConstraint(['admin_id'], [u'georef.admin.id'], name=u'rel_way_admin_admin_id_fkey'),
        sa.ForeignKeyConstraint(['way_id'], [u'georef.way.id'], name=u'rel_way_admin_way_id_fkey'),
        sa.PrimaryKeyConstraint('admin_id', 'way_id'),
        schema='georef',
    )
    op.create_table(
        'stop_area',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('properties_id', sa.BIGINT(), nullable=True),
        sa.Column('uri', sa.TEXT(), nullable=False),
        sa.Column('external_code', sa.TEXT(), nullable=False),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.Column('coord', ga.Geography(geometry_type='POINT', srid=4326, spatial_index=False), nullable=True),
        sa.Column('comment', sa.TEXT(), nullable=True),
        sa.Column('visible', sa.BOOLEAN(), nullable=False),
        sa.Column('timezone', sa.TEXT(), nullable=True),
        sa.ForeignKeyConstraint(
            ['properties_id'], [u'navitia.properties.id'], name=u'stop_area_properties_id_fkey'
        ),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'house_number',
        sa.Column('way_id', sa.BIGINT(), nullable=True),
        sa.Column('coord', ga.Geography(geometry_type='POINT', srid=4326, spatial_index=False), nullable=False),
        sa.Column('number', sa.TEXT(), nullable=False),
        sa.Column('left_side', sa.BOOLEAN(), nullable=False),
        sa.ForeignKeyConstraint(['way_id'], [u'georef.way.id'], name=u'house_number_way_id_fkey'),
        schema='georef',
    )
    op.create_table(
        'line',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('network_id', sa.BIGINT(), nullable=False),
        sa.Column('commercial_mode_id', sa.BIGINT(), nullable=False),
        sa.Column('comment', sa.TEXT(), nullable=True),
        sa.Column('uri', sa.TEXT(), nullable=False),
        sa.Column('external_code', sa.TEXT(), nullable=False),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.Column('code', sa.TEXT(), nullable=True),
        sa.Column('color', sa.TEXT(), nullable=True),
        sa.Column('sort', sa.INTEGER(), nullable=False),
        sa.ForeignKeyConstraint(
            ['commercial_mode_id'], [u'navitia.commercial_mode.id'], name=u'line_commercial_mode_id_fkey'
        ),
        sa.ForeignKeyConstraint(['network_id'], [u'navitia.network.id'], name=u'line_network_id_fkey'),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'transition',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('before_change', sa.TEXT(), nullable=False),
        sa.Column('after_change', sa.TEXT(), nullable=False),
        sa.Column('start_trip', sa.TEXT(), nullable=False),
        sa.Column('end_trip', sa.TEXT(), nullable=False),
        sa.Column(
            'global_condition',
            postgresql.ENUM(u'nothing', u'exclusive', u'with_changes', name='fare_transition_condition'),
            nullable=False,
        ),
        sa.Column('ticket_id', sa.TEXT(), nullable=True),
        sa.ForeignKeyConstraint(
            ['ticket_id'], [u'navitia.ticket.ticket_key'], name=u'transition_ticket_id_fkey'
        ),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'poi',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('weight', sa.INTEGER(), nullable=False),
        sa.Column('coord', ga.Geography(geometry_type='POINT', srid=4326, spatial_index=False), nullable=True),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.Column('uri', sa.TEXT(), nullable=False),
        sa.Column('visible', sa.BOOLEAN(), nullable=False),
        sa.Column('poi_type_id', sa.BIGINT(), nullable=False),
        sa.Column('address_name', sa.TEXT(), nullable=True),
        sa.Column('address_number', sa.TEXT(), nullable=True),
        sa.ForeignKeyConstraint(['poi_type_id'], [u'georef.poi_type.id'], name=u'poi_poi_type_id_fkey'),
        sa.PrimaryKeyConstraint('id'),
        schema='georef',
    )
    op.create_table(
        'at_perturbation',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('uri', sa.TEXT(), nullable=False),
        sa.Column('start_application_date', postgresql.TIMESTAMP(timezone=True), nullable=False),
        sa.Column('end_application_date', postgresql.TIMESTAMP(timezone=True), nullable=False),
        sa.Column('start_application_daily_hour', postgresql.TIME(timezone=True), nullable=False),
        sa.Column('end_application_daily_hour', postgresql.TIME(timezone=True), nullable=False),
        sa.Column('active_days', postgresql.BIT(length=8), nullable=False),
        sa.Column('object_uri', sa.TEXT(), nullable=False),
        sa.Column('object_type_id', sa.INTEGER(), nullable=False),
        sa.Column('is_active', sa.BOOLEAN(), nullable=False),
        sa.ForeignKeyConstraint(
            ['object_type_id'], [u'navitia.object_type.id'], name=u'at_perturbation_object_type_id_fkey'
        ),
        sa.PrimaryKeyConstraint('id'),
        schema='realtime',
    )
    op.create_table(
        'admin_stop_area',
        sa.Column('admin_id', sa.TEXT(), nullable=False),
        sa.Column('stop_area_id', sa.BIGINT(), nullable=False),
        sa.ForeignKeyConstraint(
            ['stop_area_id'], [u'navitia.stop_area.id'], name=u'admin_stop_area_stop_area_id_fkey'
        ),
        schema='navitia',
    )
    op.create_table(
        'localized_message',
        sa.Column('message_id', sa.BIGINT(), nullable=False),
        sa.Column('language', sa.TEXT(), nullable=False),
        sa.Column('body', sa.TEXT(), nullable=False),
        sa.Column('title', sa.TEXT(), nullable=True),
        sa.ForeignKeyConstraint(
            ['message_id'], [u'realtime.message.id'], name=u'localized_message_message_id_fkey'
        ),
        sa.PrimaryKeyConstraint('message_id', 'language'),
        schema='realtime',
    )
    op.create_table(
        'exception_date',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('datetime', sa.DATE(), nullable=False),
        sa.Column('type_ex', postgresql.ENUM(u'Add', u'Sub', name='exception_type'), nullable=False),
        sa.Column('calendar_id', sa.BIGINT(), nullable=False),
        sa.ForeignKeyConstraint(
            ['calendar_id'], [u'navitia.calendar.id'], name=u'exception_date_calendar_id_fkey'
        ),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'route',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('line_id', sa.BIGINT(), nullable=False),
        sa.Column('comment', sa.TEXT(), nullable=True),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.Column('uri', sa.TEXT(), nullable=False),
        sa.Column('external_code', sa.TEXT(), nullable=False),
        sa.ForeignKeyConstraint(['line_id'], [u'navitia.line.id'], name=u'route_line_id_fkey'),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'period',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('calendar_id', sa.BIGINT(), nullable=False),
        sa.Column('begin_date', sa.DATE(), nullable=False),
        sa.Column('end_date', sa.DATE(), nullable=False),
        sa.ForeignKeyConstraint(['calendar_id'], [u'navitia.calendar.id'], name=u'period_calendar_id_fkey'),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'poi_properties',
        sa.Column('poi_id', sa.BIGINT(), nullable=False),
        sa.Column('key', sa.TEXT(), nullable=True),
        sa.Column('value', sa.TEXT(), nullable=True),
        sa.ForeignKeyConstraint(['poi_id'], [u'georef.poi.id'], name=u'poi_properties_poi_id_fkey'),
        schema='georef',
    )
    op.create_table(
        'stop_point',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('properties_id', sa.BIGINT(), nullable=True),
        sa.Column('uri', sa.TEXT(), nullable=False),
        sa.Column('external_code', sa.TEXT(), nullable=False),
        sa.Column('coord', ga.Geography(geometry_type='POINT', srid=4326, spatial_index=False), nullable=True),
        sa.Column('fare_zone', sa.INTEGER(), nullable=True),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.Column('comment', sa.TEXT(), nullable=True),
        sa.Column('stop_area_id', sa.BIGINT(), nullable=False),
        sa.Column('platform_code', sa.TEXT(), nullable=True),
        sa.ForeignKeyConstraint(
            ['properties_id'], [u'navitia.properties.id'], name=u'stop_point_properties_id_fkey'
        ),
        sa.ForeignKeyConstraint(
            ['stop_area_id'], [u'navitia.stop_area.id'], name=u'stop_point_stop_area_id_fkey'
        ),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'rel_calendar_line',
        sa.Column('calendar_id', sa.BIGINT(), nullable=False),
        sa.Column('line_id', sa.BIGINT(), nullable=False),
        sa.ForeignKeyConstraint(
            ['calendar_id'], [u'navitia.calendar.id'], name=u'rel_calendar_line_calendar_id_fkey'
        ),
        sa.ForeignKeyConstraint(['line_id'], [u'navitia.line.id'], name=u'rel_calendar_line_line_id_fkey'),
        sa.PrimaryKeyConstraint('calendar_id', 'line_id'),
        schema='navitia',
    )
    op.create_table(
        'rel_line_company',
        sa.Column('line_id', sa.BIGINT(), nullable=False),
        sa.Column('company_id', sa.BIGINT(), nullable=False),
        sa.ForeignKeyConstraint(
            ['company_id'], [u'navitia.company.id'], name=u'rel_line_company_company_id_fkey'
        ),
        sa.ForeignKeyConstraint(['line_id'], [u'navitia.line.id'], name=u'rel_line_company_line_id_fkey'),
        sa.PrimaryKeyConstraint('line_id', 'company_id'),
        schema='navitia',
    )
    op.create_table(
        'connection',
        sa.Column('departure_stop_point_id', sa.BIGINT(), nullable=False),
        sa.Column('destination_stop_point_id', sa.BIGINT(), nullable=False),
        sa.Column('connection_type_id', sa.BIGINT(), nullable=False),
        sa.Column('properties_id', sa.BIGINT(), nullable=True),
        sa.Column('duration', sa.INTEGER(), nullable=False),
        sa.Column('max_duration', sa.INTEGER(), nullable=False),
        sa.Column('display_duration', sa.INTEGER(), nullable=False),
        sa.ForeignKeyConstraint(
            ['connection_type_id'], [u'navitia.connection_type.id'], name=u'connection_connection_type_id_fkey'
        ),
        sa.ForeignKeyConstraint(
            ['departure_stop_point_id'],
            [u'navitia.stop_point.id'],
            name=u'connection_departure_stop_point_id_fkey',
        ),
        sa.ForeignKeyConstraint(
            ['destination_stop_point_id'],
            [u'navitia.stop_point.id'],
            name=u'connection_destination_stop_point_id_fkey',
        ),
        sa.ForeignKeyConstraint(
            ['properties_id'], [u'navitia.properties.id'], name=u'connection_properties_id_fkey'
        ),
        sa.PrimaryKeyConstraint('departure_stop_point_id', 'destination_stop_point_id'),
        schema='navitia',
    )
    op.create_table(
        'journey_pattern',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('route_id', sa.BIGINT(), nullable=False),
        sa.Column('physical_mode_id', sa.BIGINT(), nullable=False),
        sa.Column('comment', sa.TEXT(), nullable=True),
        sa.Column('uri', sa.TEXT(), nullable=False),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.Column('is_frequence', sa.BOOLEAN(), nullable=False),
        sa.ForeignKeyConstraint(
            ['physical_mode_id'], [u'navitia.physical_mode.id'], name=u'journey_pattern_physical_mode_id_fkey'
        ),
        sa.ForeignKeyConstraint(['route_id'], [u'navitia.route.id'], name=u'journey_pattern_route_id_fkey'),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'vehicle_journey',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('adapted_validity_pattern_id', sa.BIGINT(), nullable=False),
        sa.Column('validity_pattern_id', sa.BIGINT(), nullable=True),
        sa.Column('company_id', sa.BIGINT(), nullable=False),
        sa.Column('journey_pattern_id', sa.BIGINT(), nullable=False),
        sa.Column('uri', sa.TEXT(), nullable=False),
        sa.Column('external_code', sa.TEXT(), nullable=False),
        sa.Column('comment', sa.TEXT(), nullable=True),
        sa.Column('odt_message', sa.TEXT(), nullable=True),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.Column('odt_type_id', sa.BIGINT(), nullable=True),
        sa.Column('vehicle_properties_id', sa.BIGINT(), nullable=True),
        sa.Column('theoric_vehicle_journey_id', sa.BIGINT(), nullable=True),
        sa.Column('previous_vehicle_journey_id', sa.BIGINT(), nullable=True),
        sa.Column('next_vehicle_journey_id', sa.BIGINT(), nullable=True),
        sa.Column('start_time', sa.INTEGER(), nullable=True),
        sa.Column('end_time', sa.INTEGER(), nullable=True),
        sa.Column('headway_sec', sa.INTEGER(), nullable=True),
        sa.Column('utc_to_local_offset', sa.INTEGER(), nullable=True),
        sa.ForeignKeyConstraint(
            ['adapted_validity_pattern_id'],
            [u'navitia.validity_pattern.id'],
            name=u'vehicle_journey_adapted_validity_pattern_id_fkey',
        ),
        sa.ForeignKeyConstraint(
            ['company_id'], [u'navitia.company.id'], name=u'vehicle_journey_company_id_fkey'
        ),
        sa.ForeignKeyConstraint(
            ['journey_pattern_id'],
            [u'navitia.journey_pattern.id'],
            name=u'vehicle_journey_journey_pattern_id_fkey',
        ),
        sa.ForeignKeyConstraint(
            ['next_vehicle_journey_id'],
            [u'navitia.vehicle_journey.id'],
            name=u'vehicle_journey_next_vehicle_journey_id_fkey',
        ),
        sa.ForeignKeyConstraint(
            ['previous_vehicle_journey_id'],
            [u'navitia.vehicle_journey.id'],
            name=u'vehicle_journey_previous_vehicle_journey_id_fkey',
        ),
        sa.ForeignKeyConstraint(
            ['theoric_vehicle_journey_id'],
            [u'navitia.vehicle_journey.id'],
            name=u'vehicle_journey_theoric_vehicle_journey_id_fkey',
        ),
        sa.ForeignKeyConstraint(
            ['validity_pattern_id'],
            [u'navitia.validity_pattern.id'],
            name=u'vehicle_journey_validity_pattern_id_fkey',
        ),
        sa.ForeignKeyConstraint(
            ['vehicle_properties_id'],
            [u'navitia.vehicle_properties.id'],
            name=u'vehicle_journey_vehicle_properties_id_fkey',
        ),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'journey_pattern_point',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('journey_pattern_id', sa.BIGINT(), nullable=False),
        sa.Column('name', sa.TEXT(), nullable=False),
        sa.Column('uri', sa.TEXT(), nullable=False),
        sa.Column('order', sa.INTEGER(), nullable=False),
        sa.Column('comment', sa.TEXT(), nullable=True),
        sa.Column('stop_point_id', sa.BIGINT(), nullable=False),
        sa.ForeignKeyConstraint(
            ['journey_pattern_id'],
            [u'navitia.journey_pattern.id'],
            name=u'journey_pattern_point_journey_pattern_id_fkey',
        ),
        sa.ForeignKeyConstraint(
            ['stop_point_id'], [u'navitia.stop_point.id'], name=u'journey_pattern_point_stop_point_id_fkey'
        ),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'stop_time',
        sa.Column('id', sa.BIGINT(), nullable=False),
        sa.Column('vehicle_journey_id', sa.BIGINT(), nullable=False),
        sa.Column('journey_pattern_point_id', sa.BIGINT(), nullable=False),
        sa.Column('arrival_time', sa.INTEGER(), nullable=True),
        sa.Column('departure_time', sa.INTEGER(), nullable=True),
        sa.Column('local_traffic_zone', sa.INTEGER(), nullable=True),
        sa.Column('odt', sa.BOOLEAN(), nullable=False),
        sa.Column('pick_up_allowed', sa.BOOLEAN(), nullable=False),
        sa.Column('drop_off_allowed', sa.BOOLEAN(), nullable=False),
        sa.Column('is_frequency', sa.BOOLEAN(), nullable=False),
        sa.Column('comment', sa.TEXT(), nullable=True),
        sa.Column('date_time_estimated', sa.BOOLEAN(), nullable=False),
        sa.Column('properties_id', sa.BIGINT(), nullable=True),
        sa.ForeignKeyConstraint(
            ['journey_pattern_point_id'],
            [u'navitia.journey_pattern_point.id'],
            name=u'stop_time_journey_pattern_point_id_fkey',
        ),
        sa.ForeignKeyConstraint(
            ['properties_id'], [u'navitia.properties.id'], name=u'stop_time_properties_id_fkey'
        ),
        sa.ForeignKeyConstraint(
            ['vehicle_journey_id'], [u'navitia.vehicle_journey.id'], name=u'stop_time_vehicle_journey_id_fkey'
        ),
        sa.PrimaryKeyConstraint('id'),
        schema='navitia',
    )
    op.create_table(
        'rel_metavj_vj',
        sa.Column('meta_vj', sa.BIGINT(), nullable=True),
        sa.Column('vehicle_journey', sa.BIGINT(), nullable=True),
        sa.Column(
            'vj_class',
            postgresql.ENUM(u'Theoric', u'Adapted', u'RealTime', name='vj_classification'),
            nullable=False,
        ),
        sa.ForeignKeyConstraint(['meta_vj'], [u'navitia.meta_vj.id'], name=u'rel_metavj_vj_meta_vj_fkey'),
        sa.ForeignKeyConstraint(
            ['vehicle_journey'], [u'navitia.vehicle_journey.id'], name=u'rel_metavj_vj_vehicle_journey_fkey'
        ),
        schema='navitia',
    )
    ### end Alembic commands ###


def downgrade():
    ### commands auto generated by Alembic - please adjust! ###
    op.drop_table('rel_metavj_vj', schema='navitia')
    op.drop_table('stop_time', schema='navitia')
    op.drop_table('journey_pattern_point', schema='navitia')
    op.drop_table('vehicle_journey', schema='navitia')
    op.drop_table('journey_pattern', schema='navitia')
    op.drop_table('connection', schema='navitia')
    op.drop_table('rel_line_company', schema='navitia')
    op.drop_table('rel_calendar_line', schema='navitia')
    op.drop_table('stop_point', schema='navitia')
    op.drop_table('poi_properties', schema='georef')
    op.drop_table('period', schema='navitia')
    op.drop_table('route', schema='navitia')
    op.drop_table('exception_date', schema='navitia')
    op.drop_table('localized_message', schema='realtime')
    op.drop_table('admin_stop_area', schema='navitia')
    op.drop_table('at_perturbation', schema='realtime')
    op.drop_table('poi', schema='georef')
    op.drop_table('transition', schema='navitia')
    op.drop_table('line', schema='navitia')
    op.drop_table('house_number', schema='georef')
    op.drop_table('stop_area', schema='navitia')
    op.drop_table('rel_way_admin', schema='georef')
    op.drop_table('rel_admin_admin', schema='georef')
    op.drop_table('od_ticket', schema='navitia')
    op.drop_table('edge', schema='georef')
    op.drop_table('message', schema='realtime')
    op.drop_table('dated_ticket', schema='navitia')
    op.drop_table('calendar', schema='navitia')
    op.drop_table('validity_pattern', schema='navitia')
    op.drop_table('object_type', schema='navitia')
    op.drop_table('synonym', schema='georef')
    op.drop_table('origin_destination', schema='navitia')
    op.drop_table('odt_type', schema='navitia')
    op.drop_table('contributor', schema='navitia')
    op.drop_table('message_status', schema='realtime')
    op.drop_table('company', schema='navitia')
    op.drop_table('node', schema='georef')
    op.drop_table('physical_mode', schema='navitia')
    op.drop_table('way', schema='georef')
    op.drop_table('week_pattern', schema='navitia')
    op.drop_table('administrative_regions', schema='navitia')
    op.drop_table('ticket', schema='navitia')
    op.drop_table('admin', schema='georef')
    op.drop_table('network', schema='navitia')
    op.drop_table('connection_kind', schema='navitia')
    op.drop_table('poi_type', schema='georef')
    op.drop_table('properties', schema='navitia')
    op.drop_table('commercial_mode', schema='navitia')
    op.drop_table('meta_vj', schema='navitia')
    op.drop_table('parameters', schema='navitia')
    op.drop_table('vehicle_properties', schema='navitia')
    op.drop_table('connection_type', schema='navitia')
    op.execute("DROP SCHEMA navitia;")
    op.execute("DROP SCHEMA georef;")
    op.execute("DROP SCHEMA realtime;")
