#!/usr/bin/env python
import kombu
from navitiacommon import stat_pb2
import datetime
import time

connection = kombu.Connection("amqp://guest:guest@localhost:5672//")
exchange_name = "navitia"
topic_name = "stat.sender"
exchange_object = kombu.Exchange(exchange_name, 'topic', durable = True)
producer = connection.Producer(exchange=exchange_object)

stat_request = stat_pb2.StatRequest()
dt = datetime.datetime.now()
stat_request.request_date = int(time.mktime(dt.timetuple()))
stat_request.user_id = 1
stat_request.user_name = 'Toto'
stat_request.application_id = 2
stat_request.application_name = 'navitia2'
stat_request.request_duration = 4500
stat_request.api = 'places'
stat_request.query_string = '/v1/coverage/centre/places?q=gare'
stat_request.host = 'http://navitia2-ws.ctp.dev.canaltp.fr'
stat_request.client = '127.0.0.1'
stat_request.response_size = 400

#Coverages:
stat_coverage = stat_request.coverages.add()
stat_coverage.region_id = 'paris'

#parameters:
stat_parameter = stat_request.parameters.add()
stat_parameter.key = 'param_key_1'
stat_parameter.value = 'param_value_1'
stat_parameter = stat_request.parameters.add()
stat_parameter.key = 'param_key_2'
stat_parameter.value = 'param_value_2'

#Journey
stat_journey = stat_request.journeys.add()
stat_journey.requested_date_time = int(time.mktime(dt.timetuple()))
stat_journey.departure_date_time = int(time.mktime(dt.timetuple()))
stat_journey.arrival_date_time = int(time.mktime(dt.timetuple()))
stat_journey.duration = 1234
stat_journey.nb_transfers = 3
stat_journey.type = 'rapid'

#section
stat_section = stat_journey.sections.add()
stat_section.departure_date_time = int(time.mktime(dt.timetuple()))
stat_section.arrival_date_time = int(time.mktime(dt.timetuple()))
stat_section.duration = 255
stat_section.mode = 'car'
stat_section.type = 'street_network'
stat_section.from_embedded_type = 'stop_point'
stat_section.from_id = 'from_stop_point_id'
stat_section.from_name = 'from_stop_point_name'
stat_section.from_coord.lat = 47.252525
stat_section.from_coord.lon = 2.1515151
stat_section.from_admin_id = 'from_admin_id'
stat_section.from_admin_name = 'from_admin_name'
stat_section.to_embedded_type = 'address'
stat_section.to_id = 'to_address_id'
stat_section.to_name = 'to_address_name'
stat_section.to_coord.lat = 48.253335
stat_section.to_coord.lon = 2.2545015
stat_section.to_admin_id = 'to_admin_id'
stat_section.to_admin_name = 'to_admin_name'

stat_section.vehicle_journey_id = 'vehicle_journey_id'
stat_section.line_id = 'line_id'
stat_section.line_code = 'line_code'
stat_section.route_id = 'route_id'
stat_section.network_id = 'network_id'
stat_section.network_name = 'network_name'
stat_section.physical_mode_id = 'physical_mode_id'
stat_section.physical_mode_name = 'physical_mode_name'
stat_section.commercial_mode_id = 'commercial_mode_id'
stat_section.commercial_mode_name = 'commercial_mode_name'

producer.publish(stat_request.SerializeToString(), routing_key= topic_name)

connection.close()
