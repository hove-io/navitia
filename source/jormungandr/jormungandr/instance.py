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

from __future__ import absolute_import, print_function, unicode_literals, division
from contextlib import contextmanager
import queue
from threading import Lock
from flask.ext.restful import abort
from zmq import green as zmq

from jormungandr.exceptions import TechnicalError
from navitiacommon import response_pb2, request_pb2, type_pb2
from navitiacommon.default_values import get_value_or_default
from jormungandr.timezone import set_request_instance_timezone
import logging
from .exceptions import DeadSocketException
from navitiacommon import models
from importlib import import_module
from jormungandr import cache, app, global_autocomplete
from shapely import wkt
from shapely.geos import ReadingError
from shapely import geometry
from flask import g
import flask
import pybreaker
from jormungandr import georef, planner, schedule, realtime_schedule, ptref, street_network
import itertools

type_to_pttype = {
      "stop_area": request_pb2.PlaceCodeRequest.StopArea,
      "network": request_pb2.PlaceCodeRequest.Network,
      "company": request_pb2.PlaceCodeRequest.Company,
      "line": request_pb2.PlaceCodeRequest.Line,
      "route": request_pb2.PlaceCodeRequest.Route,
      "vehicle_journey": request_pb2.PlaceCodeRequest.VehicleJourney,
      "stop_point": request_pb2.PlaceCodeRequest.StopPoint,
      "calendar": request_pb2.PlaceCodeRequest.Calendar
}

STREET_NETWORK_MODES = ('walking', 'car', 'bss', 'bike')

@app.before_request
def _init_g():
    g.instances_model = {}


# For street network modes that are not set in the given config file,
# we set kraken as their default engine
def _set_default_street_network_config(street_network_configs):
    if not isinstance(street_network_configs, list):
        street_network_configs = []
    default_sn_class = 'jormungandr.street_network.kraken.Kraken'

    modes_in_configs = set(list(itertools.chain.from_iterable(config['modes'] for config in street_network_configs)))
    modes_not_set = set(STREET_NETWORK_MODES) - modes_in_configs
    if modes_not_set:
        street_network_configs.append({"modes": list(modes_not_set),
                                       "class": default_sn_class})
    return street_network_configs


class Instance(object):

    def __init__(self,
                 context,
                 name,
                 zmq_socket,
                 street_network_configurations,
                 realtime_proxies_configuration,
                 zmq_socket_type,
                 autocomplete_type):
        self.geom = None
        self._sockets = queue.Queue()
        self.socket_path = zmq_socket
        self._scenario = None
        self._scenario_name = None
        self.lock = Lock()
        self.context = context
        self.name = name
        self.timezone = None  # timezone will be fetched from the kraken
        self.publication_date = -1
        self.is_initialized = False #kraken hasn't been called yet we don't have geom nor timezone
        self.breaker = pybreaker.CircuitBreaker(fail_max=app.config['CIRCUIT_BREAKER_MAX_INSTANCE_FAIL'],
                                                reset_timeout=app.config['CIRCUIT_BREAKER_INSTANCE_TIMEOUT_S'])
        self.georef = georef.Kraken(self)
        self.planner = planner.Kraken(self)

        street_network_configurations = _set_default_street_network_config(street_network_configurations)
        self.street_network_services = street_network.StreetNetwork.get_street_network_services(self,
                                                                                                street_network_configurations)
        self.ptref = ptref.PtRef(self)

        self.schedule = schedule.MixedSchedule(self)
        self.realtime_proxy_manager = realtime_schedule.RealtimeProxyManager(realtime_proxies_configuration, self)

        self.autocomplete = global_autocomplete.get(autocomplete_type)
        if not self.autocomplete:
            raise TechnicalError('impossible to find autocomplete system {} '
                                 'cannot initialize instance {}'.format(autocomplete_type, name))

        self.zmq_socket_type = zmq_socket_type


    def get_models(self):
        if self.name not in g.instances_model:
            g.instances_model[self.name] = self._get_models()
        return g.instances_model[self.name]

    @cache.memoize(app.config['CACHE_CONFIGURATION'].get('TIMEOUT_PARAMS', 300))
    def _get_models(self):
        if app.config['DISABLE_DATABASE']:
            return None
        return models.Instance.get_by_name(self.name)

    def scenario(self, override_scenario=None):
        if hasattr(g, 'scenario') and g.scenario:
            """
            once a scenario has been chosen for a request, we cannot change it
            """
            return g.scenario

        if override_scenario:
            logger = logging.getLogger(__name__)
            logger.debug('overriding the scenario for %s with %s', self.name, override_scenario)
            try:
                module = import_module('jormungandr.scenarios.{}'.format(override_scenario))
            except ImportError:
                logger.exception('sceneario not found')
                abort(404, message='invalid scenario: {}'.format(override_scenario))
            scenario = module.Scenario()
            g.scenario = scenario
            return scenario

        instance_db = self.get_models()
        scenario_name = instance_db.scenario if instance_db else 'new_default'
        if not self._scenario or scenario_name != self._scenario_name:
            logger = logging.getLogger(__name__)
            logger.info('loading of scenario %s for instance %s', scenario_name, self.name)
            self._scenario_name = scenario_name
            module = import_module('jormungandr.scenarios.{}'.format(scenario_name))
            self._scenario = module.Scenario()

        #we save the used scenario for future use
        g.scenario = self._scenario
        return self._scenario

    @property
    def journey_order(self):
        instance_db = self.get_models()
        return get_value_or_default('journey_order', instance_db, self.name)

    @property
    def max_walking_duration_to_pt(self):
        instance_db = self.get_models()
        return get_value_or_default('max_walking_duration_to_pt', instance_db, self.name)

    @property
    def max_bss_duration_to_pt(self):
        instance_db = self.get_models()
        return get_value_or_default('max_bss_duration_to_pt', instance_db, self.name)

    @property
    def max_bike_duration_to_pt(self):
        instance_db = self.get_models()
        return get_value_or_default('max_bike_duration_to_pt', instance_db, self.name)

    @property
    def max_car_duration_to_pt(self):
        instance_db = self.get_models()
        return get_value_or_default('max_car_duration_to_pt', instance_db, self.name)

    @property
    def walking_speed(self):
        instance_db = self.get_models()
        return get_value_or_default('walking_speed', instance_db, self.name)

    @property
    def bss_speed(self):
        instance_db = self.get_models()
        return get_value_or_default('bss_speed', instance_db, self.name)

    @property
    def bike_speed(self):
        instance_db = self.get_models()
        return get_value_or_default('bike_speed', instance_db, self.name)

    @property
    def car_speed(self):
        instance_db = self.get_models()
        return get_value_or_default('car_speed', instance_db, self.name)

    @property
    def max_nb_transfers(self):
        instance_db = self.get_models()
        return get_value_or_default('max_nb_transfers', instance_db, self.name)

    @property
    def min_tc_with_car(self):
        instance_db = self.get_models()
        return get_value_or_default('min_tc_with_car', instance_db, self.name)

    @property
    def min_tc_with_bike(self):
        instance_db = self.get_models()
        return get_value_or_default('min_tc_with_bike', instance_db, self.name)

    @property
    def min_tc_with_bss(self):
        instance_db = self.get_models()
        return get_value_or_default('min_tc_with_bss', instance_db, self.name)

    @property
    def min_bike(self):
        instance_db = self.get_models()
        return get_value_or_default('min_bike', instance_db, self.name)

    @property
    def min_bss(self):
        instance_db = self.get_models()
        return get_value_or_default('min_bss', instance_db, self.name)

    @property
    def min_car(self):
        instance_db = self.get_models()
        return get_value_or_default('min_car', instance_db, self.name)

    @property
    def factor_too_long_journey(self):
        instance_db = self.get_models()
        return get_value_or_default('factor_too_long_journey', instance_db, self.name)

    @property
    def successive_physical_mode_to_limit_id(self):
        instance_db = self.get_models()
        return get_value_or_default('successive_physical_mode_to_limit_id', instance_db, self.name)

    @property
    def min_duration_too_long_journey(self):
        instance_db = self.get_models()
        return get_value_or_default('min_duration_too_long_journey', instance_db, self.name)

    @property
    def max_duration_criteria(self):
        instance_db = self.get_models()
        return get_value_or_default('max_duration_criteria', instance_db, self.name)

    @property
    def max_duration_fallback_mode(self):
        instance_db = self.get_models()
        return get_value_or_default('max_duration_fallback_mode', instance_db, self.name)

    @property
    def priority(self):
        instance_db = self.get_models()
        return get_value_or_default('priority', instance_db, self.name)

    @property
    def bss_provider(self):
        instance_db = self.get_models()
        return get_value_or_default('bss_provider', instance_db, self.name)

    @property
    def max_additional_connections(self):
        instance_db = self.get_models()
        return get_value_or_default('max_additional_connections', instance_db, self.name)

    @property
    def is_free(self):
        instance_db = self.get_models()
        if not instance_db:
            return False
        else:
            return instance_db.is_free

    @property
    def is_open_data(self):
        instance_db = self.get_models()
        if not instance_db:
            return False
        else:
            return instance_db.is_open_data

    @property
    def max_duration(self):
        instance_db = self.get_models()
        return get_value_or_default('max_duration', instance_db, self.name)

    @property
    def walking_transfer_penalty(self):
        instance_db = self.get_models()
        return get_value_or_default('walking_transfer_penalty', instance_db, self.name)

    @property
    def night_bus_filter_max_factor(self):
        instance_db = self.get_models()
        return get_value_or_default('night_bus_filter_max_factor', instance_db, self.name)

    @property
    def night_bus_filter_base_factor(self):
        instance_db = self.get_models()
        return get_value_or_default('night_bus_filter_base_factor', instance_db, self.name)

    @contextmanager
    def socket(self, context):
        socket = None
        if self.zmq_socket_type == 'transient':
            socket = context.socket(zmq.REQ)
            socket.connect(self.socket_path)
            try:
                yield socket
            finally:
                if not socket.closed:
                    socket.close()
        else:
            try:
                socket = self._sockets.get(block=False)
            except queue.Empty:
                socket = context.socket(zmq.REQ)
                socket.connect(self.socket_path)
            try:
                yield socket
            finally:
                if not socket.closed:
                    self._sockets.put(socket)

    def send_and_receive(self, *args, **kwargs):
        """
        encapsulate all call to kraken in a circuit breaker, this way we don't loose time calling dead instance
        """
        try:
            return self.breaker.call(self._send_and_receive, *args, **kwargs)
        except pybreaker.CircuitBreakerError as e:
            raise DeadSocketException(self.name, self.socket_path)

    def _send_and_receive(self,
                         request,
                         timeout=app.config.get('INSTANCE_TIMEOUT', 10000),
                         quiet=False,
                         **kwargs):
        with self.socket(self.context) as socket:
            try:
                request.request_id = flask.request.id
            except RuntimeError:
                #we aren't in a flask context, so there is no request
                if 'request_id' in kwargs:
                    request.request_id = kwargs['request_id']
            socket.send(request.SerializeToString())
            if socket.poll(timeout=timeout) > 0:
                pb = socket.recv()
                resp = response_pb2.Response()
                resp.ParseFromString(pb)
                self.update_property(resp)#we update the timezone and geom of the instances at each request
                return resp
            else:
                socket.setsockopt(zmq.LINGER, 0)
                socket.close()
                if not quiet:
                    logger = logging.getLogger(__name__)
                    logger.error('request on %s failed: %s', self.socket_path, unicode(request))
                raise DeadSocketException(self.name, self.socket_path)

    def get_id(self, id_):
        """
        Get the pt_object that have the given id
        """
        req = request_pb2.Request()
        req.requested_api = type_pb2.place_uri
        req.place_uri.uri = id_
        return self.send_and_receive(req)

    def has_id(self, id_):
        """
        Does this instance has this id
        """
        try:
            return len(self.get_id(id_).places) > 0
        except DeadSocketException:
            return False

    def has_coord(self, lon, lat):
        return self.has_point(geometry.Point(lon, lat))

    def has_point(self, p):
        try:
            return self.geom and self.geom.contains(p)
        except DeadSocketException:
            return False

    def get_external_codes(self, type_, id_):
        """
        Get all pt_object with the given id
        """
        req = request_pb2.Request()
        req.requested_api = type_pb2.place_code
        if type_ not in type_to_pttype:
            raise ValueError("Can't call pt_code API with type: {}".format(type_))
        req.place_code.type = type_to_pttype[type_]
        req.place_code.type_code = "external_code"
        req.place_code.code = id_
        #we set the timeout to 1s
        return self.send_and_receive(req, 1000)

    def has_external_code(self, type_, id_):
        """
        Does this instance has the given id
        Returns None if it doesnt, the kraken uri otherwise
        """
        try:
            res = self.get_external_codes(type_, id_)
        except DeadSocketException:
            return False
        if len(res.places) > 0:
            return res.places[0].uri
        return None

    def update_property(self, response):
        """
        update the property of an instance from a response if the metadatas field if present
        """
        #after a successful call we consider the instance initialised even if no data were loaded
        self.is_initialized = True
        if response.HasField(str("metadatas")) and response.publication_date != self.publication_date:
            logging.getLogger(__name__).debug('updating metadata for %s', self.name)
            with self.lock as lock:
                self.publication_date = response.publication_date
                if response.metadatas.shape and response.metadatas.shape != "":
                    try:
                        self.geom = wkt.loads(response.metadatas.shape)
                    except ReadingError:
                        self.geom = None
                else:
                    self.geom = None
                self.timezone = response.metadatas.timezone
        set_request_instance_timezone(self)

    def init(self):
        """
        Get and store variables of the instance.
        Returns True if we need to clear the cache, False otherwise.
        """
        pub_date = self.publication_date
        req = request_pb2.Request()
        req.requested_api = type_pb2.METADATAS
        try:
            #we use _send_and_receive to avoid the circuit breaker, we don't want fast fail on init :)
            resp = self._send_and_receive(req, timeout=1000, quiet=True)
            #the instance is automatically updated on a call
            if self.publication_date != pub_date:
                return True
        except DeadSocketException:
            #we don't do anything on error, a new session will be established to an available kraken on
            # the next request. We don't want to purge all our cache for a small error.
            logging.getLogger(__name__).debug('timeout on init for %s', self.name)
        return False

    def get_street_network_routing_matrix(self, origins, destinations, mode, max_duration_to_pt, request, **kwargs):
        service = self.street_network_services.get(mode)
        if not service:
            return None
        return service.get_street_network_routing_matrix(origins,
                                                         destinations,
                                                         mode,
                                                         max_duration_to_pt,
                                                         request,
                                                         **kwargs)

    def direct_path(self, mode, pt_object_origin, pt_object_destination, fallback_extremity, request, direct_path_type):
        '''
        :param fallback_extremity: is a PeriodExtremity (a datetime and it's meaning on the fallback period)
        '''
        service = self.street_network_services.get(mode)
        if not service:
            return None
        return service.direct_path(mode,
                                   pt_object_origin,
                                   pt_object_destination,
                                   fallback_extremity,
                                   request,
                                   direct_path_type)

    def get_autocomplete(self, requested_autocomplete):
        if not requested_autocomplete:
            return self.autocomplete
        autocomplete = global_autocomplete.get(requested_autocomplete)
        if not autocomplete:
            raise TechnicalError('autocomplete {} not available'.format(requested_autocomplete))
        return autocomplete
