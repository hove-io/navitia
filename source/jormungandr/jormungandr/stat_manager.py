# coding=utf-8

#  Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.hove.com).
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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, unicode_literals, division
from flask import request, g
from functools import wraps
from navitiacommon import stat_pb2
import logging
from jormungandr import app
from jormungandr.authentication import get_user, get_token, get_app_name, get_used_coverages
from jormungandr import utils
import re
from threading import Lock

import pytz
import time
import sys
import kombu
import six
import pybreaker
import retrying

f_datetime = "%Y%m%dT%H%M%S"


def init_journey(stat_journey):
    stat_journey.requested_date_time = 0
    stat_journey.departure_date_time = 0
    stat_journey.arrival_date_time = 0
    stat_journey.duration = 0
    stat_journey.nb_transfers = 0
    stat_journey.type = ''


def get_mode(mode, previous_section):
    if mode == 'bike' and previous_section:
        if previous_section.type == 'bss_rent':
            return 'bss'
    return mode


class FindAdminVisitor(object):
    def __init__(self):
        self.admin = None

    def __call__(self, name, value):
        if name in ['administrative_regions', 'administrative_region'] and value.get('level') == 8:
            self.admin = value
            return True  # we have found the admin, we can stop

    def get_admin_id(self):
        if not self.admin:
            return None
        return self.admin['id']

    def get_admin_insee(self):
        if not self.admin:
            return None
        return self.admin['insee']

    def get_admin_name(self):
        if not self.admin:
            return None
        return self.admin['name']


def find_admin(point):
    visitor = FindAdminVisitor()
    utils.walk_dict(point, visitor)
    return visitor.get_admin_id(), visitor.get_admin_insee(), visitor.get_admin_name()


def find_origin_admin(journey):
    if 'sections' not in journey:
        return None, None, None
    return find_admin(journey['sections'][0]['from'])


def find_destination_admin(journey):
    if 'sections' not in journey:
        return None, None, None
    return find_admin(journey['sections'][-1]['to'])


def tz_str_to_utc_timestamp(dt_str, timezone):
    dt = timezone.normalize(timezone.localize(utils.str_to_dt(dt_str)))
    return utils.date_to_timestamp(dt.astimezone(pytz.utc))


class StatManager(object):
    def __init__(self, auto_delete=False):
        self.connection = None
        self.producer = None

        self.save_stat = app.config.get('SAVE_STAT', False)
        self.broker_url = app.config.get('BROKER_URL', None)
        self.exchange_name = app.config.get('EXCHANGE_NAME', None)
        self.connection_timeout = app.config.get('STAT_CONNECTION_TIMEOUT', 1)

        if self.save_stat:
            try:
                self._init_rabbitmq(auto_delete)
            except Exception:
                logging.getLogger(__name__).exception('Unable to activate the producer of stat')

        self.lock = Lock()
        fail_max = app.config.get('STAT_CIRCUIT_BREAKER_MAX_FAIL', 5)
        reset_timeout = app.config.get('STAT_CIRCUIT_BREAKER_TIMEOUT_S', 60)

        self.breaker = pybreaker.CircuitBreaker(fail_max=fail_max, reset_timeout=reset_timeout)

    def _init_rabbitmq(self, auto_delete=False):
        """
        connection to rabbitmq and initialize queues
        """
        self.connection = kombu.Connection(self.broker_url, connect_timeout=self.connection_timeout)
        retry_policy = {'interval_start': 0, 'interval_step': 1, 'interval_max': 1, 'max_retries': 5}

        self.connection.ensure_connection(**retry_policy)
        self.exchange = kombu.Exchange(self.exchange_name, type="topic", auto_delete=auto_delete)
        self.producer = self.connection.Producer(exchange=self.exchange)

    def manage_stat(self, start_time, call_result):
        """
        Function to fill stat objects (requests, parameters, journeys et sections) sand send them to Broker
        """
        if not self.save_stat:
            return

        try:
            self._manage_stat(start_time, call_result)
        except Exception as e:
            # if stat are not working we don't want jormungandr to stop.
            logging.getLogger(__name__).exception('Error during stat management')

    def _manage_stat(self, start_time, call_result):
        end_time = time.time()
        stat_request = stat_pb2.StatRequest()
        stat_request.request_duration = int((end_time - start_time) * 1000)  # In milliseconds
        self.fill_request(stat_request, start_time, call_result)
        self.fill_coverages(stat_request)
        self.fill_parameters(stat_request)
        self.fill_result(stat_request, call_result)

        retry = retrying.Retrying(
            stop_max_attempt_number=2,
            retry_on_exception=lambda e: not isinstance(e, pybreaker.CircuitBreakerError),
        )
        retry.call(self.breaker.call, self.publish_request, stat_request.api, stat_request.SerializeToString())

    def fill_info_response(self, stat_info_response, call_result):
        """
        store data from response of all requests
        """
        if (
            'pagination' in call_result[0]
            and call_result[0]['pagination']
            and 'items_on_page' in call_result[0]['pagination']
        ):
            stat_info_response.object_count = call_result[0]['pagination']['items_on_page']

    def fill_result(self, stat_request, call_result):
        if 'error' in call_result[0] and call_result[0]['error']:
            self.fill_error(stat_request, call_result[0]['error'])

        # We do not save informations of journeys and sections for a request
        # Isochron (parameter "&to" is absent for API Journeys )
        if 'journeys' in request.endpoint and 'to' in request.args:
            self.fill_journeys(stat_request, call_result)

    def fill_request(self, stat_request, start_time, call_result):
        """
        fill stat requests message (protobuf)
        """
        stat_request.request_date = int(start_time)
        # Note: for stat we don't want to abort if no token has been
        # given (it's up to the authentication process)
        token = get_token()
        user = get_user(token=token, abort_if_no_token=False)

        if user is not None:
            stat_request.user_id = user.id
            stat_request.user_name = user.login
            if user.end_point_id:
                stat_request.end_point_id = user.end_point_id
                stat_request.end_point_name = user.end_point.name
        if token is not None:
            stat_request.token = token
        stat_request.application_id = -1
        app_name = get_app_name(token)
        if app_name:
            stat_request.application_name = app_name
        else:
            stat_request.application_name = ''
        stat_request.api = request.endpoint
        stat_request.host = request.host_url
        if request.remote_addr and not request.headers.getlist("X-Forwarded-For"):
            stat_request.client = request.remote_addr
        elif request.headers.getlist("X-Forwarded-For"):
            stat_request.client = request.headers.getlist("X-Forwarded-For")[0]
        stat_request.path = request.path

        stat_request.response_size = sys.getsizeof(call_result[0])

        self.fill_info_response(stat_request.info_response, call_result)

    def register_interpreted_parameters(self, args):
        """
        allow to add calculated parameters for a request
        """
        g.stat_interpreted_parameters = args

    def fill_parameters(self, stat_request):
        for key in request.args:
            items = request.args.getlist(key)
            for value in items:
                stat_parameter = stat_request.parameters.add()
                stat_parameter.key = key
                stat_parameter.value = six.text_type(value)

        if hasattr(g, 'stat_interpreted_parameters'):
            for item in g.stat_interpreted_parameters.items():
                if isinstance(item[1], list):
                    for value in item[1]:
                        stat_parameter = stat_request.interpreted_parameters.add()
                        stat_parameter.key = item[0]
                        stat_parameter.value = six.text_type(value)
                else:
                    stat_parameter = stat_request.interpreted_parameters.add()
                    stat_parameter.key = item[0]
                    stat_parameter.value = six.text_type(item[1])
                    if item[0] in ('filter', 'departure_filter', 'arrival_filter'):
                        # we parse ptref filter here
                        self.fill_filters(item[1], stat_parameter)

    def fill_filters(self, value, stat_parameter):
        """
        basic parsing of a ptref filter
        """
        if not value:
            return
        for elem in re.split('\s+and\s+', value, flags=re.IGNORECASE):
            match = re.match('^\s*(\w+).(\w+)\s*([=!><]{1,2}|DWITHIN)\s*(.*)$', elem, re.IGNORECASE)
            if match:
                filter = stat_parameter.filters.add()
                filter.object = match.group(1)
                filter.attribute = match.group(2)
                filter.operator = match.group(3)
                filter.value = match.group(4)
            else:
                logging.getLogger(__name__).warning('impossible to parse: %s', elem)

    def fill_coverages(self, stat_request):
        coverages = get_used_coverages()
        if coverages:
            for coverage in coverages:
                stat_coverage = stat_request.coverages.add()
                stat_coverage.region_id = coverage
        else:
            # We need an empty coverage.
            stat_request.coverages.add()

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

        tz = utils.get_timezone()
        if 'requested_date_time' in resp_journey:
            stat_journey.requested_date_time = tz_str_to_utc_timestamp(resp_journey['requested_date_time'], tz)

        if 'departure_date_time' in resp_journey:
            stat_journey.departure_date_time = tz_str_to_utc_timestamp(resp_journey['departure_date_time'], tz)

        if 'arrival_date_time' in resp_journey:
            stat_journey.arrival_date_time = tz_str_to_utc_timestamp(resp_journey['arrival_date_time'], tz)

        if 'duration' in resp_journey:
            stat_journey.duration = resp_journey['duration']

        if 'nb_transfers' in resp_journey:
            stat_journey.nb_transfers = resp_journey['nb_transfers']

        if 'type' in resp_journey:
            stat_journey.type = resp_journey['type']

        first_pt_section = None
        last_pt_section = None
        for section in resp_journey.get('sections', []):
            if 'type' in section and section['type'] == 'public_transport':
                if not first_pt_section:
                    first_pt_section = section
                last_pt_section = section

        if first_pt_section and last_pt_section:
            stat_journey.first_pt_id = first_pt_section['from']['id']
            stat_journey.first_pt_name = first_pt_section['from']['name']
            self.fill_coord(
                stat_journey.first_pt_coord,
                first_pt_section['from'][first_pt_section['from']['embedded_type']]['coord'],
            )
            admin = find_admin(first_pt_section['from'])
            if admin[0]:
                stat_journey.first_pt_admin_id = admin[0]
            if admin[1]:
                stat_journey.first_pt_admin_insee = admin[1]
            if admin[2]:
                stat_journey.first_pt_admin_name = admin[2]

            stat_journey.last_pt_id = last_pt_section['to']['id']
            stat_journey.last_pt_name = last_pt_section['to']['name']
            self.fill_coord(
                stat_journey.last_pt_coord,
                last_pt_section['to'][last_pt_section['to']['embedded_type']]['coord'],
            )
            admin = find_admin(last_pt_section['to'])
            if admin[0]:
                stat_journey.last_pt_admin_id = admin[0]
            if admin[1]:
                stat_journey.last_pt_admin_insee = admin[1]
            if admin[2]:
                stat_journey.last_pt_admin_name = admin[2]

    def fill_journeys(self, stat_request, call_result):
        """
        Fill journeys and sections for each journey (datetimes are all UTC)
        """
        journey_request = stat_request.journey_request
        if hasattr(g, 'stat_interpreted_parameters') and g.stat_interpreted_parameters['original_datetime']:
            tz = utils.get_timezone()
            dt = g.stat_interpreted_parameters['original_datetime']
            if dt.tzinfo is None:
                dt = tz.normalize(tz.localize(dt))
            journey_request.requested_date_time = utils.date_to_timestamp(dt.astimezone(pytz.utc))
            journey_request.clockwise = g.stat_interpreted_parameters['clockwise']
        if 'journeys' in call_result[0] and call_result[0]['journeys']:
            first_journey = call_result[0]['journeys'][0]
            origin = find_origin_admin(first_journey)
            if origin[0]:
                journey_request.departure_admin = origin[0]
            if origin[1]:
                journey_request.departure_insee = origin[1]
            if origin[2]:
                journey_request.departure_admin_name = origin[2]
            destination = find_destination_admin(first_journey)
            if destination[0]:
                journey_request.arrival_admin = destination[0]
            if destination[1]:
                journey_request.arrival_insee = destination[1]
            if destination[2]:
                journey_request.arrival_admin_name = destination[2]
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

    def fill_section(self, stat_section, resp_section, previous_section):
        tz = utils.get_timezone()
        if 'departure_date_time' in resp_section:
            stat_section.departure_date_time = tz_str_to_utc_timestamp(resp_section['departure_date_time'], tz)

        if 'arrival_date_time' in resp_section:
            stat_section.arrival_date_time = tz_str_to_utc_timestamp(resp_section['arrival_date_time'], tz)

        if 'duration' in resp_section:
            stat_section.duration = resp_section['duration']

        if 'mode' in resp_section:
            stat_section.mode = get_mode(resp_section['mode'], previous_section)

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
        self.fill_admin_from(stat_section, find_admin(section_point))

        if stat_section.from_embedded_type == 'stop_point' and 'stop_area' in section_point['stop_point']:
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

        self.fill_admin_to(stat_section, find_admin(section_point))

        if stat_section.to_embedded_type == 'stop_point' and 'stop_area' in section_point['stop_point']:
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
            logging.getLogger(__name__).warning('Unable to parse coordinates: %s', six.text_type(e))

    def fill_sections(self, stat_journey, resp_journey):
        previous_section = None
        if 'sections' in resp_journey:
            for resp_section in resp_journey['sections']:
                stat_section = stat_journey.sections.add()
                self.fill_section(stat_section, resp_section, previous_section)
                previous_section = stat_section

    def publish_request(self, api, pbf):
        with self.lock:
            try:
                if self.producer is None:
                    # if the initialization failed we have to retry the creation of the objects
                    self._init_rabbitmq()
                self.producer.publish(pbf, routing_key=api)
            except self.connection.connection_errors + self.connection.channel_errors:
                logging.getLogger(__name__).exception('Server went away, will be reconnected..')
                # Relese and close the previous connection
                if self.connection and self.connection.connected:
                    self.connection.release()
                self.producer = None
                raise

    def fill_admin_from(self, stat_section, admin):
        if admin[0]:
            stat_section.from_admin_id = admin[0]
        if admin[2]:
            stat_section.from_admin_name = admin[2]
        if admin[1]:
            stat_section.from_admin_insee = admin[1]

    def fill_admin_to(self, stat_section, admin):
        if admin[0]:
            stat_section.to_admin_id = admin[0]
        if admin[2]:
            stat_section.to_admin_name = admin[2]
        if admin[1]:
            stat_section.to_admin_insee = admin[1]

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
