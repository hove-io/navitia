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
import logging

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
        except Exception:
            pass

    def fill_parameters(self, stat_request, func_call):
        for item in request.args.iteritems():
            stat_parameter = stat_request.parameters.add()
            try:
                stat_parameter.key = item[0]
                stat_parameter.value = item[1]
            except Exception:
                pass


    def fill_coverages(self, stat_request, func_call):
        stat_coverage = stat_request.coverages.add()
        try:
            stat_coverage.region_id = request.view_args['region']
        except Exception:
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
            self.init_journey(stat_journey)
            key_name = 'requested_date_time'
            if key_name in resp_journey:
                req_date_time = resp_journey[key_name]
                if len(req_date_time) > 0:
                    stat_journey.requested_date_time = int(time.mktime(datetime.strptime(req_date_time, f_datetime).timetuple()))

            key_name = 'departure_date_time'
            if key_name in resp_journey:
                dep_date_time = resp_journey[key_name]
                if len(dep_date_time) > 0:
                    stat_journey.departure_date_time = int(time.mktime(datetime.strptime(dep_date_time, f_datetime).timetuple()))

            key_name = 'arrival_date_time'
            if key_name in resp_journey:
                arr_date_time = resp_journey["arrival_date_time"]
                if len(arr_date_time) > 0:
                    stat_journey.arrival_date_time = int(time.mktime(datetime.strptime(arr_date_time, f_datetime).timetuple()))

            key_name = 'duration'
            if key_name in resp_journey:
                stat_journey.duration = resp_journey[key_name]

            key_name = 'nb_transfers'
            if key_name in resp_journey:
                stat_journey.nb_transfers = resp_journey[key_name]

            key_name = 'type'
            if key_name in resp_journey:
                stat_journey.type = resp_journey[key_name]

        except Exception:
            pass


    def fill_journeys(self, stat_request, func_call):
        """
        Fill journeys and sections for each journey
        """
        for resp_journey in func_call[0]['journeys']:
            stat_journey = stat_request.journeys.add()
            self.fill_journey(stat_journey, resp_journey)
            self.fill_sections(stat_journey, resp_journey)


    def get_section_link(self, resp_section,link_type):
        result = ''
        for section_link in resp_section['links']:
            if section_link['type'] == link_type:
                result = section_link['id']
                break

        return result


    def fill_section(self, stat_section, resp_section):
        try:
            key_name = 'departure_date_time'
            if key_name in resp_section:
                dep_time = resp_section[key_name]
                if len(dep_time) > 0:
                    stat_section.departure_date_time = int(time.mktime(datetime.strptime(dep_time, f_datetime).timetuple()))

            key_name = 'arrival_date_time'
            if key_name in resp_section:
                arr_time = resp_section[key_name]
                if len(arr_time) > 0:
                    stat_section.arrival_date_time = int(time.mktime(datetime.strptime(arr_time, f_datetime).timetuple()))

            key_name = 'duration'
            if key_name in resp_section:
                stat_section.duration = resp_section[key_name]

            key_name = 'mode'
            if key_name in resp_section:
                stat_section.mode = resp_section[key_name]

            key_name = 'type'
            if key_name in resp_section:
                stat_section.type = resp_section[key_name]
        except Exception:
            pass

        try:
            stat_section.from_embedded_type = resp_section['from']['embedded_type']
            stat_section.from_id = resp_section['from']['id']
            stat_section.from_name = resp_section['from']['name']

            from_lat = resp_section['from'][stat_section.from_embedded_type]['coord']['lat']
            if len(from_lat) > 0:
                stat_section.from_coord.lat = float(from_lat)
            from_lon = resp_section['from'][stat_section.from_embedded_type]['coord']['lon']
            if len(from_lon) > 0:
                stat_section.from_coord.lon = float(from_lon)
        except Exception:
            pass

        try:
            stat_section.to_embedded_type = resp_section['to']['embedded_type']
            stat_section.to_id = resp_section['to']['id']
            stat_section.to_name = resp_section['to']['name']

            to_lat = resp_section['to'][stat_section.to_embedded_type]['coord']['lat']
            if len(to_lat) > 0:
                stat_section.to_coord.lat = float(to_lat)
            to_lon = resp_section['to'][stat_section.to_embedded_type]['coord']['lon']
            if len(to_lon) > 0:
                stat_section.to_coord.lon = float(to_lon)

        except Exception:
            pass

        #Ajouter les communes de départ et arrivé
        self.fill_admin_from_to(stat_section, resp_section)

        try:
            stat_section.vehicle_journey_id = self.get_section_link(resp_section, 'vehicle_journey')
            stat_section.line_id = self.get_section_link(resp_section, 'line')
            stat_section.route_id = self.get_section_link(resp_section, 'route')
            stat_section.network_id = self.get_section_link(resp_section, 'network')
            stat_section.physical_mode_id = self.get_section_link(resp_section, 'physical_mode')
            stat_section.commercial_mode_id = self.get_section_link(resp_section, 'commercial_mode')
        except Exception:
            pass

        self.fill_section_display_informations(stat_section, resp_section)

    def fill_sections(self, stat_journey, resp_journey):

        for resp_section in resp_journey['sections']:
            stat_section = stat_journey.sections.add()
            self.fill_section(stat_section, resp_section)


    def publish_request(self, stat_request):
        self.producer.publish(stat_request.SerializeToString(), routing_key=self.topic_name)


    def fill_admin_from_to(self, stat_section, resp_section):
        try:
            for admin in resp_section['from'][stat_section.from_embedded_type]['administrative_regions']:
                if (admin['level'] == 8):
                    stat_section.from_admin_id = admin['id']
                    stat_section.from_admin_name = admin['name']
                    pass

            for admin in resp_section['to'][stat_section.from_embedded_type]['administrative_regions']:
                if (admin['level'] == 8):
                    stat_section.to_admin_id = admin['id']
                    stat_section.to_admin_name = admin['name']
                    pass
        except Exception:
            pass

    def fill_section_display_informations(self, stat_section, resp_section):
        try:
            stat_section.line_code = resp_section['display_informations']['code']
            stat_section.network_name = resp_section['display_informations']['network']
            stat_section.physical_mode_name = resp_section['display_informations']['physical_mode']
            stat_section.commercial_mode_name = resp_section['display_informations']['commercial_mode']
        except Exception:
            pass

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


















