# coding=utf-8
from navitiacommon import response_pb2
from flask_restful import reqparse, abort
import flask_restful
from flask import current_app, request
from functools import wraps
from navitiacommon import stat_pb2
from navitiacommon import request_pb2
from datetime import datetime, timedelta
from datetime import datetime

import time
import sys
import kombu
f_datetime = "%Y%m%dT%H%M%S"

class StatManager(object):

    def __init__(self):
        self.connection = None
        self.producer = None
        self.broker_url = None
        self.exchange_name = None
        self.topic_name = None

    def init(self, broker_url, exchange_name, topic_name):
        self.broker_url = broker_url
        self.exchange_name = exchange_name
        self.topic_name = topic_name
        self._init_rabbitmq()

    def _init_rabbitmq(self):
        """
        connection to rabbitmq and initialize queues
        """
        self.connection = kombu.Connection(self.broker_url)
        exchange = kombu.Exchange(self.exchange_name, 'topic', durable = True)
        self.producer = self.connection.Producer(exchange=exchange)


    def manage_stat(self, start_time, func_call):
        """
        Function to fill stat objects (requests, parameters, journeys et sections) sand send them to Broker
        """
        end_time = time.time()
        response_duration = (end_time - start_time) #In seconds
        stat_request = stat_pb2.StatRequest()
        stat_request.request_duration = int(response_duration * 1000) #In milliseconds
        self.fill_request(stat_request, func_call)
        self.fill_coverages(stat_request, func_call)
        self.fill_parameters(stat_request, func_call)
        if 'journeys' in request.endpoint:
            self.fill_journeys(stat_request, func_call)
        self.publish_request(stat_request)

    def fill_request(self, stat_request, func_call):
        """
        Remplir requests
        """
        try:
            dt = datetime.now()
            stat_request.request_date = int(time.mktime(dt.timetuple()))
            stat_request.user_id = 1 #??
            stat_request.user_name = 'Toto' #??
            stat_request.application_id = 2 #??
            stat_request.application_name = 'navitia2' #??
            stat_request.api = request.endpoint
            stat_request.query_string = request.url
            stat_request.host = request.host_url
            stat_request.response_size = 400
            stat_request.client = request.remote_addr
        except:
            pass

    def fill_parameters(self, stat_request, func_call):
        for item in request.args.iteritems():
            stat_parameter = stat_request.parameters.add()
            try:
                stat_parameter.key = item[0]
                stat_parameter.value = item[1]
            except:
                pass

    def fill_coverages(self, stat_request, func_call):
        stat_coverage = stat_request.coverages.add()
        try:
            stat_coverage.region_id = request.view_args['region']
        except:
            pass


    def init_journey(self, stat_journey):
        stat_journey.requested_date_time = 0
        stat_journey.departure_date_time = 0
        stat_journey.arrival_date_time = 0
        stat_journey.duration = 0
        stat_journey.nb_transfers = 0
        stat_journey.type = ''

    def fill_journey(self, stat_journey, resp_journey):
        """"
        Fill journey and all sections:
        """
        try:
            req_date_time = resp_journey["requested_date_time"]
            if len(req_date_time) > 0:
                stat_journey.requested_date_time = int(time.mktime(datetime.strptime(req_date_time, f_datetime).timetuple()))

            dep_date_time = resp_journey["departure_date_time"]
            if len(dep_date_time) > 0:
                stat_journey.departure_date_time = int(time.mktime(datetime.strptime(dep_date_time, f_datetime).timetuple()))

            arr_date_time = resp_journey["arrival_date_time"]
            if len(arr_date_time) > 0:
                stat_journey.arrival_date_time = int(time.mktime(datetime.strptime(arr_date_time, f_datetime).timetuple()))

            stat_journey.duration = resp_journey["duration"]
            stat_journey.nb_transfers = resp_journey["nb_transfers"]
            stat_journey.type = resp_journey["type"]

        except:
            pass

        self.fill_sections(stat_journey, resp_journey)


    def fill_journeys(self, stat_request, func_call):
        """
        Fill journeys and sections for each journey
        """
        for resp_journey in func_call[0]['journeys']:
            stat_journey = stat_request.journeys.add()
            self.fill_journey(stat_journey, resp_journey)


    def get_section_link(self, resp_section,link_type):
        result = ''
        for section_link in resp_section['links']:
            if section_link['type'] == link_type:
                result = section_link['id']
                break

        return result


    def fill_section(self, stat_section, resp_section):
        try:
            dep_time = resp_section['departure_date_time']
            if len(dep_time) > 0:
                stat_section.departure_date_time = int(time.mktime(datetime.strptime(dep_time, f_datetime).timetuple()))
            arr_time = resp_section['arrival_date_time']
            if len(arr_time) > 0:
                stat_section.arrival_date_time = int(time.mktime(datetime.strptime(arr_time, f_datetime).timetuple()))
            stat_section.duration = resp_section['duration']
            stat_section.type = resp_section['type']
            stat_section.from_embedded_type = resp_section['from']['embedded_type']
            stat_section.from_id = resp_section['from']['id']

            from_lat = resp_section['from'][stat_section.from_embedded_type]['coord']['lat']
            if len(from_lat) > 0:
                stat_section.from_coord.lat = float(from_lat)
            from_lon = resp_section['from'][stat_section.from_embedded_type]['coord']['lon']
            if len(from_lon) > 0:
                stat_section.from_coord.lon = float(from_lon)

            stat_section.to_embedded_type = resp_section['to']['embedded_type']
            stat_section.to_id = resp_section['to']['id']

            to_lat = resp_section['to'][stat_section.to_embedded_type]['coord']['lat']
            if len(to_lat) > 0:
                stat_section.to_coord.lat = float(to_lat)
            to_lon = resp_section['to'][stat_section.to_embedded_type]['coord']['lon']
            if len(to_lon) > 0:
                stat_section.to_coord.lon = float(to_lon)

            stat_section.vehicle_journey_id = self.get_section_link(resp_section, 'vehicle_journey')
            stat_section.line_id = self.get_section_link(resp_section, 'line')
            stat_section.route_id = self.get_section_link(resp_section, 'route')
            stat_section.network_id = self.get_section_link(resp_section, 'network')
            stat_section.physical_mode_id = self.get_section_link(resp_section, 'physical_mode')
            stat_section.commercial_mode_id = self.get_section_link(resp_section, 'commercial_mode')

        except:
            pass

    def fill_sections(self, stat_journey, resp_journey):

        for resp_section in resp_journey['sections']:
            stat_section = stat_journey.sections.add()
            self.fill_section(stat_section, resp_section)


    def publish_request(self, stat_request):
        self.producer.publish(stat_request.SerializeToString(), routing_key=self.topic_name)


class manage_stat_caller:

    def __init__(self, stat_mana):
        self.manager = stat_mana

    def __call__(self, f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            start_time = time.time()
            func_call = f(*args, **kwargs)
            self.manager.manage_stat(start_time, func_call)
            return func_call
        return wrapper


















