# coding=utf-8
from navitiacommon import response_pb2
from flask_restful import reqparse, abort
import flask_restful
from flask import current_app, request, g
from functools import wraps
from navitiacommon import stat_pb2
from navitiacommon import request_pb2
from datetime import datetime, timedelta
import logging

import time
import sys
import kombu
f_datetime = "%Y%m%dT%H%M%S"

class StatManager(object):

    def __init__(self):
        self.save_stat = False
        self.connection = None
        self.producer = None
        self.broker_url = None
        self.exchange_name = None
        self.topic_name = None

    def init(self, save_stat=False, broker_url=None, exchange_name=None, topic_name=None):
        self.save_stat = save_stat
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

    def param_exists(self, param_name, stat_request):
        result = False
        for param in stat_request.parameters:
            if param.key == param_name:
                result = True
                break
        return result

    def manage_stat(self, start_time, func_call):
        """
        Function to fill stat objects (requests, parameters, journeys et sections) sand send them to Broker
        """
        if not self.save_stat:
            logging.getLogger(__name__).info('Registration of stat is not activated')
            return

        end_time = time.time()
        stat_request = stat_pb2.StatRequest()
        stat_request.request_duration = int((end_time - start_time) * 1000) #In milliseconds
        self.fill_request(stat_request, func_call)
        self.fill_coverages(stat_request, func_call)
        self.fill_parameters(stat_request, func_call)

        #We do not save informations of journeys and sections for a request
        #Isochron (parameter "&to" is absent for API Journeys )
        if 'journeys' in request.endpoint and \
                self.param_exists('to', stat_request):
            self.fill_journeys(stat_request, func_call)
        self.publish_request(stat_request)

    def fill_request(self, stat_request, func_call):
        """
        Remplir requests
        """
        dt = datetime.now()
        stat_request.request_date = int(time.mktime(dt.timetuple()))
        if g.user is not None and 'user_id' in g.user:
            stat_request.user_id = g.user['user_id']
        if g.user is not None and 'user_name' in g.user:
            stat_request.user_name = g.user['user_id']
        stat_request.application_id = -1
        stat_request.application_name = ''
        stat_request.api = request.endpoint
        stat_request.query_string = request.url
        stat_request.host = request.host_url
        if request.remote_addr:
            stat_request.client = request.remote_addr
        stat_request.response_size = sys.getsizeof(func_call[0])


    def fill_parameters(self, stat_request, func_call):
        for item in request.args.iteritems():
            stat_parameter = stat_request.parameters.add()
            stat_parameter.key = item[0]
            stat_parameter.value = item[1]


    def fill_coverages(self, stat_request, func_call):
        stat_coverage = stat_request.coverages.add()
        if 'region' in request.view_args:
            stat_coverage.region_id = request.view_args['region']


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
        self.init_journey(stat_journey)
        if 'requested_date_time' in resp_journey:
            req_date_time = resp_journey['requested_date_time']
            stat_journey.requested_date_time = self.get_posix_date_time(req_date_time)

        if 'departure_date_time' in resp_journey:
            dep_date_time = resp_journey['departure_date_time']
            stat_journey.departure_date_time = self.get_posix_date_time(dep_date_time)

        if 'arrival_date_time' in resp_journey:
            arr_date_time = resp_journey['arrival_date_time']
            stat_journey.arrival_date_time = self.get_posix_date_time(arr_date_time)

        if 'duration' in resp_journey:
            stat_journey.duration = resp_journey['duration']

        if 'nb_transfers' in resp_journey:
            stat_journey.nb_transfers = resp_journey['nb_transfers']

        if 'type' in resp_journey:
            stat_journey.type = resp_journey['type']


    def get_posix_date_time(self, date_time_value):
        if date_time_value:
            try:
                return int(time.mktime(datetime.strptime(date_time_value, f_datetime).timetuple()))
            except ValueError as e:
                logging.getLogger(__name__).warn("%s", str(e))

        return 0


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
        if 'links' in resp_section:
            for section_link in resp_section['links']:
                if section_link['type'] == link_type:
                    result = section_link['id']
                    break

        return result


    def fill_section(self, stat_section, resp_section):
        if 'departure_date_time' in resp_section:
            dep_time = resp_section['departure_date_time']
            stat_section.departure_date_time = self.get_posix_date_time(dep_time)

        if 'arrival_date_time' in resp_section:
            arr_time = resp_section['arrival_date_time']
            stat_section.arrival_date_time = self.get_posix_date_time(arr_time)

        if 'duration' in resp_section:
            stat_section.duration = resp_section['duration']

        if 'mode' in resp_section:
            stat_section.mode = resp_section['mode']

        if 'type' in resp_section:
            stat_section.type = resp_section['type']

        if 'from' in resp_section:
            section_from = resp_section['from']
            stat_section.from_embedded_type = section_from['embedded_type']
            stat_section.from_id = section_from['id']
            stat_section.from_name = section_from['name']
            try:
                from_lat = section_from[stat_section.from_embedded_type]['coord']['lat']
                stat_section.from_coord.lat = float(from_lat)

                from_lon = section_from[stat_section.from_embedded_type]['coord']['lon']
                stat_section.from_coord.lon = float(from_lon)

            except ValueError as e:
                current_app.logger.warn('Unable to parse coordinates: %s', str(e))

        if 'to' in resp_section:
            section_to = resp_section['to']
            stat_section.to_embedded_type = section_to['embedded_type']
            stat_section.to_id = section_to['id']
            stat_section.to_name = section_to['name']

            try:
                to_lat = section_to[stat_section.to_embedded_type]['coord']['lat']
                stat_section.to_coord.lat = float(to_lat)

                to_lon = section_to[stat_section.to_embedded_type]['coord']['lon']
                stat_section.to_coord.lon = float(to_lon)

            except ValueError as e:
                current_app.logger.warn('Unable to parse coordinates: %s', str(e))

        #Ajouter les communes de départ et arrivé
        self.fill_admin_from_to(stat_section, resp_section)

        stat_section.vehicle_journey_id = self.get_section_link(resp_section, 'vehicle_journey')
        stat_section.line_id = self.get_section_link(resp_section, 'line')
        stat_section.route_id = self.get_section_link(resp_section, 'route')
        stat_section.network_id = self.get_section_link(resp_section, 'network')
        stat_section.physical_mode_id = self.get_section_link(resp_section, 'physical_mode')
        stat_section.commercial_mode_id = self.get_section_link(resp_section, 'commercial_mode')

        self.fill_section_display_informations(stat_section, resp_section)


    def fill_sections(self, stat_journey, resp_journey):

        if 'sections' in resp_journey:
            for resp_section in resp_journey['sections']:
                stat_section = stat_journey.sections.add()
                self.fill_section(stat_section, resp_section)


    def publish_request(self, stat_request):
        self.producer.publish(stat_request.SerializeToString(), routing_key=self.topic_name)


    def fill_admin_from_to(self, stat_section, resp_section):
        try:
            if 'administrative_regions' in resp_section['from'][stat_section.from_embedded_type]:
                for admin in resp_section['from'][stat_section.from_embedded_type]['administrative_regions']:
                    if (admin['level'] == 8):
                        stat_section.from_admin_id = admin['id']
                        stat_section.from_admin_name = admin['name']
                        break

            if 'administrative_regions' in resp_section['to'][stat_section.to_embedded_type]:
                for admin in resp_section['to'][stat_section.to_embedded_type]['administrative_regions']:
                    if (admin['level'] == 8):
                        stat_section.to_admin_id = admin['id']
                        stat_section.to_admin_name = admin['name']
                        break

        except ValueError as e:
            current_app.logger.warn('Unable to parse Information: %s', str(e))


    def fill_section_display_informations(self, stat_section, resp_section):
        if 'display_informations' in resp_section:
            disp_info = resp_section['display_informations']

            if 'code' in disp_info:
                stat_section.line_code = disp_info['code']

            if 'network' in disp_info:
                stat_section.network_name = disp_info['network']

            if 'physical_mode' in disp_info:
                stat_section.physical_mode_name = disp_info['physical_mode']

            if 'commercial_mode' in disp_info:
                stat_section.commercial_mode_name = disp_info['commercial_mode']


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