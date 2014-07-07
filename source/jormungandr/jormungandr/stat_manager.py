# coding=utf-8

#  Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Canal TP (www.canaltp.fr).
# Help us simplify mobility and open public transport:
#     a non ending quest to the responsive locomotion way of traveling!
#
# LICENCE: This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Stay tuned using
# twitter @navitia
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from navitiacommon import response_pb2
from flask_restful import reqparse, abort
import flask_restful
from flask import current_app, request, g
from functools import wraps
from navitiacommon import stat_pb2
from navitiacommon import request_pb2
from datetime import datetime, timedelta
import logging
from jormungandr import app
from jormungandr.instance import Instance
from jormungandr.authentification import get_user

import time
import sys
import kombu
f_datetime = "%Y%m%dT%H%M%S"


def init_journey(stat_journey):
    stat_journey.requested_date_time = 0
    stat_journey.departure_date_time = 0
    stat_journey.arrival_date_time = 0
    stat_journey.duration = 0
    stat_journey.nb_transfers = 0
    stat_journey.type = ''


def get_posix_date_time(date_time_value):
    if date_time_value:
        try:
            return int(time.mktime(datetime.strptime(date_time_value, f_datetime).timetuple()))
        except ValueError as e:
            logging.getLogger(__name__).warn("%s", str(e))
        except Exception:
            logging.exception("fail")

    return 0


class StatManager(object):

    def __init__(self):
        if 'SAVE_STAT' in app.config:
            self.save_stat = app.config['SAVE_STAT']
        else:
            self.save_stat = False
        self.connection = None
        self.producer = None
        if 'BROKER_URL' in app.config:
            self.broker_url = app.config['BROKER_URL']
        else:
            self.broker_url = None
        if 'EXCHANGE_NAME' in app.config:
            self.exchange_name = app.config['EXCHANGE_NAME']
        else:
            self.exchange_name = None

        if self.save_stat:
            self._init_rabbitmq()

    def _init_rabbitmq(self):
        """
        connection to rabbitmq and initialize queues
        """
        try:
            self.connection = kombu.Connection(self.broker_url)
            exchange = kombu.Exchange(self.exchange_name, type="direct")
            self.producer = self.connection.Producer(exchange=exchange)
            self.save_stat = True
        except:
            self.save_stat = False
            logging.getLogger(__name__).warn('Unable to activate the producer of stat')

    def manage_stat(self, start_time, call_result):
        """
        Function to fill stat objects (requests, parameters, journeys et sections) sand send them to Broker
        """
        if not self.save_stat:
            return

        try:
            self._manage_stat(start_time, call_result)
        except Exception as e:
            #if stat are not working we don't want jormungandr to stop.
            current_app.logger.exception('Error during stat management')

    def _manage_stat(self, start_time, call_result):
        end_time = time.time()
        stat_request = stat_pb2.StatRequest()
        stat_request.request_duration = int((end_time - start_time) * 1000) #In milliseconds
        self.fill_request(stat_request, call_result)
        self.fill_coverages(stat_request)
        self.fill_parameters(stat_request)
        self.fill_result(stat_request, call_result)
        self.publish_request(stat_request)

    def fill_result(self, stat_request, call_result):
        if 'error' in call_result[0]:
            self.fill_error(stat_request, call_result[0]['error'])

        #We do not save informations of journeys and sections for a request
        #Isochron (parameter "&to" is absent for API Journeys )
        if 'journeys' in request.endpoint and \
                'to' in request.args:
            self.fill_journeys(stat_request, call_result)

    def fill_request(self, stat_request, call_result):
        """
        Remplir requests
        """
        dt = datetime.utcnow()
        stat_request.request_date = int(time.mktime(dt.timetuple()))
        # Note: for stat we don't want to abort if no token has been
        # given (it's up to the authentication process)
        user = get_user(abort_if_no_token=False)
        if user is not None:
            stat_request.user_id = user.id
            stat_request.user_name = user.login

        stat_request.application_id = -1
        stat_request.application_name = ''
        stat_request.api = request.endpoint
        stat_request.host = request.host_url
        if request.remote_addr and \
                not request.headers.getlist("X-Forwarded-For"):
            stat_request.client = request.remote_addr
        elif request.headers.getlist("X-Forwarded-For"):
            stat_request.client = request.headers.getlist("X-Forwarded-For")[0]

        stat_request.response_size = sys.getsizeof(call_result[0])

    def fill_parameters(self, stat_request):
        for item in request.args.iteritems():
            stat_parameter = stat_request.parameters.add()
            stat_parameter.key = item[0]
            stat_parameter.value = item[1]

    def fill_coverages(self, stat_request):
        stat_coverage = stat_request.coverages.add()
        if 'region' in request.view_args:
            stat_coverage.region_id = request.view_args['region']

    def fill_error(self, stat_request, error):
        stat_error = stat_request.error
        if 'id' in error:
            stat_error.id = error['id']
        if 'message' in error:
            stat_error.message = error['message']

    def fill_journey(self, stat_journey, resp_journey):
        """"
        Fill journey and all sections from resp_journey.
        resp_journey is a OrderedDict and contains information
        of the node journeys in the result.
        """
        init_journey(stat_journey)
        if 'requested_date_time' in resp_journey:
            req_date_time = resp_journey['requested_date_time']
            stat_journey.requested_date_time = get_posix_date_time(req_date_time)

        if 'departure_date_time' in resp_journey:
            dep_date_time = resp_journey['departure_date_time']
            stat_journey.departure_date_time = get_posix_date_time(dep_date_time)

        if 'arrival_date_time' in resp_journey:
            arr_date_time = resp_journey['arrival_date_time']
            stat_journey.arrival_date_time = get_posix_date_time(arr_date_time)

        if 'duration' in resp_journey:
            stat_journey.duration = resp_journey['duration']

        if 'nb_transfers' in resp_journey:
            stat_journey.nb_transfers = resp_journey['nb_transfers']

        if 'type' in resp_journey:
            stat_journey.type = resp_journey['type']

    def fill_journeys(self, stat_request, call_result):
        """
        Fill journeys and sections for each journey
        """
        if 'journeys' in call_result[0]:
            for resp_journey in call_result[0]['journeys']:
                stat_journey = stat_request.journeys.add()
                self.fill_journey(stat_journey, resp_journey)
                self.fill_sections(stat_journey, resp_journey)

    def get_section_link(self, resp_section, link_type):
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
            stat_section.departure_date_time = get_posix_date_time(dep_time)

        if 'arrival_date_time' in resp_section:
            arr_time = resp_section['arrival_date_time']
            stat_section.arrival_date_time = get_posix_date_time(arr_time)

        if 'duration' in resp_section:
            stat_section.duration = resp_section['duration']

        if 'mode' in resp_section:
            stat_section.mode = resp_section['mode']

        if 'type' in resp_section:
            stat_section.type = resp_section['type']

        if 'from' in resp_section:
            section_from = resp_section['from']
            self.fill_section_from(stat_section, section_from)

        if 'to' in resp_section:
            section_to = resp_section['to']
            self.fill_section_to(stat_section, section_to)

        stat_section.vehicle_journey_id = self.get_section_link(resp_section, 'vehicle_journey')
        stat_section.line_id = self.get_section_link(resp_section, 'line')
        stat_section.route_id = self.get_section_link(resp_section, 'route')
        stat_section.network_id = self.get_section_link(resp_section, 'network')
        stat_section.physical_mode_id = self.get_section_link(resp_section, 'physical_mode')
        stat_section.commercial_mode_id = self.get_section_link(resp_section, 'commercial_mode')

        self.fill_section_display_informations(stat_section, resp_section)

    def fill_section_from(self, stat_section, section_point):
        """
        If EmbeddedType is stop_point then fill information of Stop_area
        instead of stop_point
        """
        stat_section.from_embedded_type = section_point['embedded_type']
        if 'administrative_regions' in section_point[stat_section.from_embedded_type]:
            self.fill_admin_from(stat_section, section_point[stat_section.from_embedded_type]['administrative_regions'])

        if stat_section.from_embedded_type == 'stop_point'\
            and 'stop_area' in section_point['stop_point']:
            section_point = section_point['stop_point']['stop_area']
            stat_section.from_embedded_type = 'stop_area'
        else:
            section_point = section_point[stat_section.from_embedded_type]

        stat_section.from_id = section_point['id']
        stat_section.from_name = section_point['name']

        if 'coord' in section_point:
            self.fill_coord(stat_section.from_coord, section_point['coord'])

    def fill_section_to(self, stat_section, section_point):
        """
        If EmbeddedType is stop_point then fill information of Stop_area
        instead of stop_point
        """
        stat_section.to_embedded_type = section_point['embedded_type']

        if 'administrative_regions' in section_point[stat_section.to_embedded_type]:
            self.fill_admin_to(stat_section, section_point[stat_section.to_embedded_type]['administrative_regions'])

        if stat_section.to_embedded_type == 'stop_point' \
            and 'stop_area' in section_point['stop_point']:
            section_point = section_point['stop_point']['stop_area']
            stat_section.to_embedded_type = 'stop_area'
        else:
            section_point = section_point[stat_section.to_embedded_type]

        stat_section.to_id = section_point['id']
        stat_section.to_name = section_point['name']
        if 'coord' in section_point:
            self.fill_coord(stat_section.to_coord, section_point['coord'])

    def fill_coord(self, stat_coord, point_coord):
        try:
            to_lat = point_coord['lat']
            stat_coord.lat = float(to_lat)

            to_lon = point_coord['lon']
            stat_coord.lon = float(to_lon)

        except ValueError as e:
            current_app.logger.warn('Unable to parse coordinates: %s', str(e))

    def fill_sections(self, stat_journey, resp_journey):
        if 'sections' in resp_journey:
            for resp_section in resp_journey['sections']:
                stat_section = stat_journey.sections.add()
                self.fill_section(stat_section, resp_section)

    def publish_request(self, stat_request):
        try:
            self.producer.publish(stat_request.SerializeToString())
        except self.connection.connection_errors + self.connection.channel_errors:
            logging.getLogger(__name__).exception('Server went away, will be reconnected..')
            #Relese and close the previous connection
            if self.connection and self.connection.connected:
                self.connection.release()
            #Initialize a new connection to RabbitMQ
            self._init_rabbitmq()
            #If connection is established publish the stat message.
            if self.save_stat:
                self.producer.publish(stat_request.SerializeToString())

    def fill_admin_from(self, stat_section, admins):
        for admin in admins:
            if admin['level'] == 8:
                stat_section.from_admin_id = admin['id']
                stat_section.from_admin_name = admin['name']
                break


    def fill_admin_to(self,stat_section, admins):
        for admin in admins:
            if admin['level'] == 8:
                stat_section.to_admin_id = admin['id']
                stat_section.to_admin_name = admin['name']
                break

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
            call_result = f(*args, **kwargs)
            self.manager.manage_stat(start_time, call_result)
            return call_result
        return wrapper