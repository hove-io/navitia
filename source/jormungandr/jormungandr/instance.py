# coding=utf-8

#  Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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

try:
    from typing import Dict, Text, Deque, List, Tuple
except ImportError:
    pass
from contextlib import contextmanager
from threading import Lock
from flask_restful import abort
from zmq import green as zmq
import copy

from jormungandr.exceptions import TechnicalError
from navitiacommon import response_pb2, request_pb2, type_pb2
from navitiacommon.default_values import get_value_or_default
from jormungandr.timezone import set_request_instance_timezone, get_instance_str_timezone
import logging
from jormungandr.exceptions import DeadSocketException
from navitiacommon import models
from importlib import import_module
from jormungandr import cache, memory_cache, app, global_autocomplete
from shapely import wkt, geometry
from shapely.geos import PredicateError, ReadingError, TopologicalError
from flask import g
import flask
import pybreaker
from jormungandr import georef, schedule, realtime_schedule, ptref, street_network, fallback_modes
from jormungandr.scenarios.ridesharing.ridesharing_service_manager import RidesharingServiceManager
import six
from collections import namedtuple
from datetime import datetime, timedelta
from navitiacommon import default_values
from jormungandr.equipments import EquipmentProviderManager
from jormungandr.external_services import ExternalServiceManager
from jormungandr.parking_space_availability.bss.bss_provider_manager import BssProviderManager
from jormungandr.parking_space_availability.car.car_park_provider_manager import CarParkingProviderManager
from jormungandr.utils import (
    can_connect_to_database,
    make_origin_destination_key,
    read_best_boarding_positions,
    get_pt_object_coord,
)
from jormungandr.olympic_site_params_manager import OlympicSiteParamsManager
from jormungandr import pt_planners_manager, transient_socket
from jormungandr.pt_journey_fare import PtJourneyFareBackendManager
import os

type_to_pttype = {
    "stop_area": request_pb2.PlaceCodeRequest.StopArea,  # type: ignore
    "network": request_pb2.PlaceCodeRequest.Network,  # type: ignore
    "company": request_pb2.PlaceCodeRequest.Company,  # type: ignore
    "line": request_pb2.PlaceCodeRequest.Line,  # type: ignore
    "route": request_pb2.PlaceCodeRequest.Route,  # type: ignore
    "vehicle_journey": request_pb2.PlaceCodeRequest.VehicleJourney,  # type: ignore
    "stop_point": request_pb2.PlaceCodeRequest.StopPoint,  # type: ignore
    "calendar": request_pb2.PlaceCodeRequest.Calendar,  # type: ignore
}

OlympicsForbiddenUris = namedtuple(
    'OlympicsForbiddenUris',
    ['pt_object_olympics_forbidden_uris', 'poi_property_key', 'poi_property_value', 'min_pt_duration'],
)


@app.before_request
def _init_g():
    g.instances_model = {}


# TODO: use this helper function for all properties if possible
# Warning: it breaks static type deduction
def _make_property_getter(attr_name):
    """
    a helper function.

    return a getter for Instance's attr
    :param attr_name:
    :return:
    """

    def _getter(self):
        return get_value_or_default(attr_name, self.get_models(), self.name)

    return property(_getter)


def parse_and_get_olympics_forbidden_uris(dict_olympics_forbidden_uris):
    if not dict_olympics_forbidden_uris:
        return None
    if not isinstance(dict_olympics_forbidden_uris, dict):
        logging.getLogger(__name__).error('olympic_criteria: invalid parameter type.')
        return None
    if "pt_object_olympics_forbidden_uris" not in dict_olympics_forbidden_uris or not isinstance(
        dict_olympics_forbidden_uris["pt_object_olympics_forbidden_uris"], list
    ):
        logging.getLogger(__name__).error(
            'olympic_criteria: invalid parameter, pt_object_olympics_forbidden_uris not found or invalid'
        )
        return None
    for p in ["poi_property_key", "poi_property_value"]:
        if p not in dict_olympics_forbidden_uris:
            logging.getLogger(__name__).error('olympic_criteria: invalid parameter, {} not found'.format(p))
            return None
    if not isinstance(dict_olympics_forbidden_uris.get('min_pt_duration'), int):
        logging.getLogger(__name__).error(
            'olympic_criteria: invalid parameter, min_pt_duration is not an integer'
        )
        return None

    return OlympicsForbiddenUris(
        pt_object_olympics_forbidden_uris=dict_olympics_forbidden_uris["pt_object_olympics_forbidden_uris"],
        poi_property_key=dict_olympics_forbidden_uris["poi_property_key"],
        poi_property_value=dict_olympics_forbidden_uris["poi_property_value"],
        min_pt_duration=dict_olympics_forbidden_uris["min_pt_duration"],
    )


class Instance(transient_socket.TransientSocket):
    name = None  # type: Text

    def __init__(
        self,
        context,  # type: zmq.Context
        name,  # type: Text
        zmq_socket,  # type: Text
        street_network_configurations,
        ridesharing_configurations,
        realtime_proxies_configuration,
        zmq_socket_type,
        autocomplete_type,
        instance_equipment_providers,  # type: List[Text]
        streetnetwork_backend_manager,
        external_service_provider_configurations,
        pt_planners_configurations,
        pt_journey_fare_configurations,
        ghost_words=None,
        instance_db=None,
        best_boarding_positions_dir=None,
        olympics_forbidden_uris=None,
        use_multi_reverse=False,
        resp_content_limit_bytes=None,
        resp_content_limit_endpoints_whitelist=None,
        individual_bss_provider=[],
        individual_car_parking_provider=[],
        timezone=None,
    ):
        super(Instance, self).__init__(
            name=name,
            zmq_context=context,
            zmq_socket=zmq_socket,
            socket_ttl=app.config.get(str('ZMQ_SOCKET_TTL_SECONDS'), 10),
        )

        self.geom = None
        self.geojson = None
        self.socket_path = zmq_socket
        self._scenario = None
        self._scenario_name = None
        self.lock = Lock()
        self.context = context
        self.name = name
        self.timezone = get_instance_str_timezone(timezone, name)
        self.publication_date = -1
        self.is_initialized = False  # kraken hasn't been called yet we don't have geom nor timezone
        self.breaker = pybreaker.CircuitBreaker(
            fail_max=app.config.get(str('CIRCUIT_BREAKER_MAX_INSTANCE_FAIL'), 5),
            reset_timeout=app.config.get(str('CIRCUIT_BREAKER_INSTANCE_TIMEOUT_S'), 60),
        )
        self.georef = georef.Kraken(self)
        self._streetnetwork_backend_manager = streetnetwork_backend_manager

        disable_database = app.config[str('DISABLE_DATABASE')]
        if disable_database:
            self._streetnetwork_backend_manager.init_streetnetwork_backends_legacy(
                self, street_network_configurations
            )
            self.ridesharing_services_manager = RidesharingServiceManager(self, ridesharing_configurations)
        else:
            self.ridesharing_services_manager = RidesharingServiceManager(
                self, ridesharing_configurations, self.get_ridesharing_services_from_db
            )

        self.olympics_forbidden_uris = parse_and_get_olympics_forbidden_uris(olympics_forbidden_uris)

        self._pt_planner_manager = pt_planners_manager.PtPlannersManager(
            pt_planners_configurations,
            name,
            zmq_socket_type,
            context,
            default_socket_path=zmq_socket,
            db_configs_getter=lambda: self.pt_planners_configurations,
        )
        # Init Ridesharing services from config file
        self.ridesharing_services_manager.init_ridesharing_services()

        self.ptref = ptref.PtRef(self)

        self.schedule = schedule.MixedSchedule(self)
        providers_getter = None if disable_database else self.get_realtime_proxies_from_db
        self.realtime_proxy_manager = realtime_schedule.RealtimeProxyManager(
            realtime_proxies_configuration, self, providers_getter
        )

        self._autocomplete_type = autocomplete_type
        if self._autocomplete_type is not None and self._autocomplete_type not in global_autocomplete:
            raise RuntimeError(
                'impossible to find autocomplete system {} '
                'cannot initialize instance {}'.format(autocomplete_type, name)
            )

        self.zmq_socket_type = zmq_socket_type

        if disable_database:
            self.equipment_provider_manager = EquipmentProviderManager(
                app.config[str('EQUIPMENT_DETAILS_PROVIDERS')]
            )
        else:
            self.equipment_provider_manager = EquipmentProviderManager(
                app.config[str('EQUIPMENT_DETAILS_PROVIDERS')], self.get_providers_from_db
            )

        # Init equipment providers from config
        self.equipment_provider_manager.init_providers(instance_equipment_providers)

        # Init free-floating providers from config
        if disable_database:
            self.external_service_provider_manager = ExternalServiceManager(
                self, external_service_provider_configurations
            )
        else:
            self.external_service_provider_manager = ExternalServiceManager(
                self, external_service_provider_configurations, self.get_external_service_providers_from_db
            )

        # Init BSS provider manager from config from external services in bdd
        if disable_database:
            self.bss_provider_manager = BssProviderManager(individual_bss_provider)
        else:
            self.bss_provider_manager = BssProviderManager(
                individual_bss_provider, self.get_bss_stations_services_from_db
            )

        # Init CAR provider manager from config from external services in bdd
        if disable_database:
            self.car_parking_provider_manager = CarParkingProviderManager(individual_car_parking_provider)
        else:
            self.car_parking_provider_manager = CarParkingProviderManager(
                individual_car_parking_provider, self.get_car_parking_services_from_db
            )

        self.external_service_provider_manager.init_external_services()
        self.instance_db = instance_db
        self._ghost_words = ghost_words or []
        self.best_boarding_positions = None
        self.use_multi_reverse = use_multi_reverse
        self.olympic_site_params_manager = None
        self.resp_content_limit_bytes = resp_content_limit_bytes
        # a list of endpoints that are not affected by the resp_content_limit_bytes
        self.resp_content_limit_endpoints_whitelist = set(resp_content_limit_endpoints_whitelist or [])

        # Read the best_boarding_positions files if any
        if best_boarding_positions_dir is not None:
            file_path = os.path.join(best_boarding_positions_dir, "{}.csv".format(self.name))
            self.best_boarding_positions = read_best_boarding_positions(file_path)

        # load stop_point attractivities, the feature is only available when loki is selected as pt_planner
        self.olympic_site_params_manager = OlympicSiteParamsManager(
            self, app.config.get(str('OLYMPIC_SITE_PARAMS_BUCKET'), {})
        )
        self.olympic_site_params_manager.fill_olympic_site_params_from_s3()

        # TODO: use db
        self._pt_journey_fare_backend_manager = PtJourneyFareBackendManager(
            self, pt_journey_fare_configurations, None
        )

    def get_providers_from_db(self):
        """
        :return: a callable query of equipment providers associated to the current instance in db
        """
        models = self._get_models()
        return models.equipment_details_providers if models else None

    def get_ridesharing_services_from_db(self):
        """
        :return: a callable query of ridesharing services associated to the current instance in db
        """
        models = self._get_models()
        return models.ridesharing_services if models else None

    def get_external_service_providers_from_db(self):
        """
        :return: a callable query of external services associated to the current instance in db
        """
        models = self._get_models()
        result = models.external_services if models else []
        return [
            res
            for res in result
            if res.navitia_service in ['free_floatings', 'vehicle_occupancies', "vehicle_positions", "obstacles"]
        ]

    def get_realtime_proxies_from_db(self):
        """
        :return: a callable query of external services associated to the current instance in db
        """
        models = self._get_models()
        result = models.external_services if models else None
        return [res for res in result if res.navitia_service == 'realtime_proxies']

    def get_bss_stations_services_from_db(self):
        """
        :return: a callable query of external services associated to the current instance in db
        """
        models = self._get_models()
        result = models.external_services if models else []
        return [res for res in result if res.navitia_service == 'bss_stations']

    def get_car_parking_services_from_db(self):
        """
        :return: a callable query of external services associated to the current instance in db
        """
        models = self._get_models()
        result = models.external_services if models else []
        return [res for res in result if res.navitia_service == 'car_parkings']

    @property
    def autocomplete(self):
        if self._autocomplete_type:
            # retrocompat: we need to continue to read configuration from file
            # while we migrate to database configuration
            return global_autocomplete.get(self._autocomplete_type)
        backend = global_autocomplete.get(self.autocomplete_backend)
        if backend is None:
            raise RuntimeError(
                'impossible to find autocomplete {} for instance {}'.format(self.autocomplete_backend, self.name)
            )
        return backend

    def stop_point_fallbacks(self):
        return [a for a in global_autocomplete.values() if a.is_handling_stop_points()]

    def get_models(self):
        if self.name not in g.instances_model:
            g.instances_model[self.name] = self._get_models()
        return g.instances_model[self.name]

    def __repr__(self):
        return 'instance.{}'.format(self.name)

    @memory_cache.memoize(app.config[str('MEMORY_CACHE_CONFIGURATION')].get(str('TIMEOUT_PARAMS'), 30))
    @cache.memoize(app.config[str('CACHE_CONFIGURATION')].get(str('TIMEOUT_PARAMS'), 300))
    def _get_models(self):
        if app.config['DISABLE_DATABASE']:
            return None
        if not can_connect_to_database():
            return self.instance_db
        try:
            instance_db = models.Instance.get_by_name(self.name)
        except Exception as e:
            logging.getLogger(__name__).exception('No access to table instance (error: {})'.format(e))
            g.can_connect_to_database = False
            return self.instance_db

        self.instance_db = instance_db
        return self.instance_db

    def scenario(self, override_scenario=None):
        """
        once a scenario has been chosen for a request for an instance (coverage), we cannot change it
        """
        if hasattr(g, 'scenario') and g.scenario.get(self.name):
            return g.scenario[self.name]

        def replace_experimental_scenario(s):
            return 'distributed' if s == 'experimental' else s

        if override_scenario:
            logger = logging.getLogger(__name__)
            logger.debug('overriding the scenario for %s with %s', self.name, override_scenario)
            try:
                # for the sake of backwards compatibility... some users may still be using experimental...
                override_scenario = replace_experimental_scenario(override_scenario)
                module = import_module('jormungandr.scenarios.{}'.format(override_scenario))
            except ImportError:
                logger.exception('scenario not found')
                abort(404, message='invalid scenario: {}'.format(override_scenario))
            scenario = module.Scenario()
            # Save scenario_name and scenario
            self._scenario_name = override_scenario
            self._scenario = scenario
            if not hasattr(g, 'scenario'):
                g.scenario = {}
            g.scenario[self.name] = scenario
            return scenario

        instance_db = self.get_models()
        scenario_name = instance_db.scenario if instance_db else 'new_default'
        # for the sake of backwards compatibility... some users may still be using experimental...
        scenario_name = replace_experimental_scenario(scenario_name)
        if not self._scenario or scenario_name != self._scenario_name:
            logger = logging.getLogger(__name__)
            logger.info('loading of scenario %s for instance %s', scenario_name, self.name)
            self._scenario_name = scenario_name
            module = import_module('jormungandr.scenarios.{}'.format(scenario_name))
            self._scenario = module.Scenario()

        # we save the used scenario for future use
        if not hasattr(g, 'scenario'):
            g.scenario = {}
        g.scenario[self.name] = self._scenario
        return self._scenario

    @property
    def max_waiting_duration(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('max_waiting_duration', instance_db, self.name)

    @property
    def journey_order(self):
        # type: () -> Text
        instance_db = self.get_models()
        return get_value_or_default('journey_order', instance_db, self.name)

    @property
    def autocomplete_backend(self):
        # type: () -> Text
        instance_db = self.get_models()
        return get_value_or_default('autocomplete_backend', instance_db, self.name)

    @property
    def max_walking_duration_to_pt(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('max_walking_duration_to_pt', instance_db, self.name)

    @property
    def max_bss_duration_to_pt(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('max_bss_duration_to_pt', instance_db, self.name)

    @property
    def max_bike_duration_to_pt(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('max_bike_duration_to_pt', instance_db, self.name)

    @property
    def max_car_duration_to_pt(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('max_car_duration_to_pt', instance_db, self.name)

    @property
    def max_car_no_park_duration_to_pt(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('max_car_no_park_duration_to_pt', instance_db, self.name)

    @property
    def max_ridesharing_duration_to_pt(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('max_ridesharing_duration_to_pt', instance_db, self.name)

    @property
    def max_taxi_duration_to_pt(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('max_taxi_duration_to_pt', instance_db, self.name)

    @property
    def walking_speed(self):
        # type: () -> float
        instance_db = self.get_models()
        return get_value_or_default('walking_speed', instance_db, self.name)

    @property
    def bss_speed(self):
        # type: () -> float
        instance_db = self.get_models()
        return get_value_or_default('bss_speed', instance_db, self.name)

    @property
    def bike_speed(self):
        # type: () -> float
        instance_db = self.get_models()
        return get_value_or_default('bike_speed', instance_db, self.name)

    @property
    def car_speed(self):
        # type: () -> float
        instance_db = self.get_models()
        return get_value_or_default('car_speed', instance_db, self.name)

    @property
    def car_no_park_speed(self):
        # type: () -> float
        instance_db = self.get_models()
        return get_value_or_default('car_no_park_speed', instance_db, self.name)

    @property
    def ridesharing_speed(self):
        # type: () -> float
        instance_db = self.get_models()
        return get_value_or_default('ridesharing_speed', instance_db, self.name)

    @property
    def max_nb_transfers(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('max_nb_transfers', instance_db, self.name)

    @property
    def min_bike(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('min_bike', instance_db, self.name)

    @property
    def min_bss(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('min_bss', instance_db, self.name)

    @property
    def min_car(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('min_car', instance_db, self.name)

    @property
    def min_taxi(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('min_taxi', instance_db, self.name)

    @property
    def successive_physical_mode_to_limit_id(self):
        # type: () -> Text
        instance_db = self.get_models()
        return get_value_or_default('successive_physical_mode_to_limit_id', instance_db, self.name)

    @property
    def priority(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('priority', instance_db, self.name)

    @property
    def bss_provider(self):
        # type: () -> bool
        instance_db = self.get_models()
        return get_value_or_default('bss_provider', instance_db, self.name)

    @property
    def car_park_provider(self):
        # type: () -> bool
        instance_db = self.get_models()
        return get_value_or_default('car_park_provider', instance_db, self.name)

    @property
    def max_additional_connections(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('max_additional_connections', instance_db, self.name)

    @property
    def is_free(self):
        # type: () -> bool
        instance_db = self.get_models()
        if not instance_db:
            return False
        else:
            return instance_db.is_free

    @property
    def is_open_data(self):
        # type: () -> bool
        instance_db = self.get_models()
        if not instance_db:
            return False
        else:
            return instance_db.is_open_data

    @property
    def max_duration(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('max_duration', instance_db, self.name)

    @property
    def walking_transfer_penalty(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('walking_transfer_penalty', instance_db, self.name)

    @property
    def arrival_transfer_penalty(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('arrival_transfer_penalty', instance_db, self.name)

    @property
    def night_bus_filter_max_factor(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('night_bus_filter_max_factor', instance_db, self.name)

    @property
    def night_bus_filter_base_factor(self):
        # type: () -> float
        instance_db = self.get_models()
        return get_value_or_default('night_bus_filter_base_factor', instance_db, self.name)

    @property
    def realtime_pool_size(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('realtime_pool_size', instance_db, self.name)

    @property
    def min_nb_journeys(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('min_nb_journeys', instance_db, self.name)

    @property
    def max_nb_journeys(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('max_nb_journeys', instance_db, self.name)

    @property
    def min_journeys_calls(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('min_journeys_calls', instance_db, self.name)

    @property
    def max_successive_physical_mode(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('max_successive_physical_mode', instance_db, self.name)

    @property
    def final_line_filter(self):
        instance_db = self.get_models()
        return get_value_or_default('final_line_filter', instance_db, self.name)

    @property
    def max_extra_second_pass(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('max_extra_second_pass', instance_db, self.name)

    @property
    def asynchronous_ridesharing(self):
        # type: () -> bool
        instance_db = self.get_models()
        return get_value_or_default('asynchronous_ridesharing', instance_db, self.name)

    @property
    def ridesharing_greenlet_pool_size(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('ridesharing_greenlet_pool_size', instance_db, self.name)

    @property
    def greenlet_pool_for_ridesharing_services(self):
        # type: () -> bool
        instance_db = self.get_models()
        return get_value_or_default('greenlet_pool_for_ridesharing_services', instance_db, self.name)

    @property
    def max_nb_crowfly_by_mode(self):
        # type: () -> Dict[Text, int]
        instance_db = self.get_models()
        # the value by default is a dict...
        d = copy.deepcopy(get_value_or_default('max_nb_crowfly_by_mode', instance_db, self.name))
        # In case we add a new max_nb_crowfly for an other mode than
        # the ones already present in the database.
        for mode, duration in default_values.max_nb_crowfly_by_mode.items():
            if mode not in d:
                d[mode] = duration

        return d

    @property
    def poi_dataset(self):
        # type: () -> Text
        instance_db = self.get_models()
        return instance_db.poi_dataset if instance_db else None

    @property
    def max_car_no_park_direct_path_duration(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('max_car_no_park_direct_path_duration', instance_db, self.name)

    @property
    def min_ridesharing(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('min_ridesharing', instance_db, self.name)

    @property
    def max_bike_direct_path_distance(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('max_bike_direct_path_distance', instance_db, self.name)

    @property
    def max_bss_direct_path_distance(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('max_bss_direct_path_distance', instance_db, self.name)

    @property
    def max_walking_direct_path_distance(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('max_walking_direct_path_distance', instance_db, self.name)

    @property
    def max_car_direct_path_distance(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('max_car_direct_path_distance', instance_db, self.name)

    @property
    def max_car_no_park_direct_path_distance(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('max_car_no_park_direct_path_distance', instance_db, self.name)

    @property
    def max_ridesharing_direct_path_distance(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('max_ridesharing_direct_path_distance', instance_db, self.name)

    @property
    def max_taxi_direct_path_distance(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('max_taxi_direct_path_distance', instance_db, self.name)

    @property
    def ghost_words(self):
        # type: () -> List
        if self._ghost_words:
            return self._ghost_words
        instance_db = self.get_models()
        return get_value_or_default('ghost_words', instance_db, self.name)

    @property
    def additional_parameters(self):
        # type: () -> bool
        instance_db = self.get_models()
        return get_value_or_default('additional_parameters', instance_db, self.name)

    # TODO: refactorise all properties
    taxi_speed = _make_property_getter('taxi_speed')
    additional_time_after_first_section_taxi = _make_property_getter('additional_time_after_first_section_taxi')
    additional_time_before_last_section_taxi = _make_property_getter('additional_time_before_last_section_taxi')

    max_walking_direct_path_duration = _make_property_getter('max_walking_direct_path_duration')
    max_bike_direct_path_duration = _make_property_getter('max_bike_direct_path_duration')
    max_bss_direct_path_duration = _make_property_getter('max_bss_direct_path_duration')
    max_car_direct_path_duration = _make_property_getter('max_car_direct_path_duration')
    max_taxi_direct_path_duration = _make_property_getter('max_taxi_direct_path_duration')
    max_ridesharing_direct_path_duration = _make_property_getter('max_ridesharing_direct_path_duration')

    street_network_car = _make_property_getter('street_network_car')
    street_network_car_no_park = _make_property_getter('street_network_car_no_park')
    street_network_walking = _make_property_getter('street_network_walking')
    street_network_bike = _make_property_getter('street_network_bike')
    street_network_bss = _make_property_getter('street_network_bss')
    street_network_ridesharing = _make_property_getter('street_network_ridesharing')
    street_network_taxi = _make_property_getter('street_network_taxi')

    stop_points_nearby_duration = _make_property_getter('stop_points_nearby_duration')

    bss_rent_duration = _make_property_getter('bss_rent_duration')
    bss_rent_penalty = _make_property_getter('bss_rent_penalty')
    bss_return_duration = _make_property_getter('bss_return_duration')
    bss_return_penalty = _make_property_getter('bss_rent_penalty')
    language = _make_property_getter('language')

    transfer_path = _make_property_getter('transfer_path')
    access_points = _make_property_getter('access_points')
    poi_access_points = _make_property_getter('poi_access_points')

    default_pt_planner = _make_property_getter('default_pt_planner')
    pt_planners_configurations = _make_property_getter('pt_planners_configurations')

    loki_pt_journey_fare = _make_property_getter('loki_pt_journey_fare')
    loki_compute_pt_journey_fare = _make_property_getter('loki_compute_pt_journey_fare')
    loki_pt_journey_fare_configurations = _make_property_getter('loki_pt_journey_fare_configurations')

    filter_odt_journeys = _make_property_getter('filter_odt_journeys')

    co2_emission_car_value = _make_property_getter('co2_emission_car_value')
    co2_emission_car_unit = _make_property_getter('co2_emission_car_unit')

    def get_pt_planner(self, pt_planner_id=None):
        pt_planner_id = pt_planner_id or self.default_pt_planner
        return self._pt_planner_manager.get_pt_planner(pt_planner_id)

    def get_pt_journey_fare(self, loki_pt_journey_fare_id=None):
        pt_journey_fare_id = loki_pt_journey_fare_id or self.loki_pt_journey_fare
        return self._pt_journey_fare_backend_manager.get_pt_journey_fare(pt_journey_fare_id)

    @property
    def places_proximity_radius(self):
        # type: () -> int
        instance_db = self.get_models()
        return get_value_or_default('places_proximity_radius', instance_db, self.name)

    def send_and_receive(self, *args, **kwargs):
        """
        encapsulate all call to kraken in a circuit breaker, this way we don't loose time calling dead instance
        """
        try:
            return self.breaker.call(self._send_and_receive, *args, **kwargs)
        except pybreaker.CircuitBreakerError as e:
            raise DeadSocketException(self.name, self.socket_path)

    def _send_and_receive(self, request, timeout=app.config.get('INSTANCES_TIMEOUT', 10), quiet=False, **kwargs):
        deadline = datetime.utcnow() + timedelta(milliseconds=timeout * 1000)
        request.deadline = deadline.strftime('%Y%m%dT%H%M%S,%f')

        if 'request_id' in kwargs and kwargs['request_id']:
            request.request_id = kwargs['request_id']
        else:
            try:
                request.request_id = flask.request.id
            except RuntimeError:
                # we aren't in a flask context, so there is no request
                if 'flask_request_id' in kwargs and kwargs['flask_request_id']:
                    request.request_id = kwargs['flask_request_id']

        pb = self.call(request.SerializeToString(), timeout, debug_cb=lambda: six.text_type(request))
        resp = response_pb2.Response()
        resp.ParseFromString(pb)
        self.update_property(resp)
        return resp

    def get_id(self, id_):
        """
        Get the pt_object that have the given id
        """
        req = request_pb2.Request()
        req.requested_api = type_pb2.place_uri
        req.place_uri.uri = id_
        return self.send_and_receive(req, timeout=app.config.get(str('PLACE_FAST_TIMEOUT'), 1))

    def has_id(self, id_):
        """
        Does this instance has this id
        """
        try:
            return len(self.get_id(id_).places) > 0
        except DeadSocketException:
            return False

    def get_coord_by_id(self, id_):
        """
        If this instance has this id then get coordinate
        """
        try:
            pt_objects = self.get_id(id_).places
            pt_object = pt_objects[0] if len(pt_objects) > 0 else None
            if pt_object:
                coord = get_pt_object_coord(pt_object)
                return coord if (coord and coord.lon != 0 and coord.lat != 0) else None
            else:
                return None
        except DeadSocketException:
            return None

    def has_coord(self, lon, lat):
        return self.has_point(geometry.Point(lon, lat))

    def has_point(self, p):
        try:
            return self.geom and self.geom.contains(p)
        except DeadSocketException:
            return False
        except PredicateError:
            logging.getLogger(__name__).exception("has_coord failed")
            return False
        except TopologicalError:
            logging.getLogger(__name__).exception("check_topology failed")
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
        # we set the timeout to 1s
        return self.send_and_receive(req, timeout=app.config.get(str('PLACE_FAST_TIMEOUT'), 1))

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
        # after a successful call we consider the instance initialised even if no data were loaded
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
                self._update_geojson()
        set_request_instance_timezone(self)

    def _update_geojson(self):
        """construct the geojson object from the shape"""
        if not self.geom or not self.geom.is_valid:
            self.geojson = None
            return
        # simplify the geom to prevent slow query on bragi
        geom = self.geom.simplify(tolerance=0.01)
        self.geojson = geometry.mapping(geom)

    def init(self):
        """
        Get and store variables of the instance.
        Returns True if we need to clear the cache, False otherwise.
        """
        pub_date = self.publication_date
        req = request_pb2.Request()
        req.requested_api = type_pb2.METADATAS
        request_id = "instance_init"
        try:
            # we use _send_and_receive to avoid the circuit breaker, we don't want fast fail on init :)
            resp = self._send_and_receive(req, request_id=request_id, timeout=1, quiet=True)
            # the instance is automatically updated on a call
            if self.publication_date != pub_date:
                return True
        except DeadSocketException:
            # we don't do anything on error, a new session will be established to an available kraken on
            # the next request. We don't want to purge all our cache for a small error.
            logging.getLogger(__name__).debug('timeout on init for %s', self.name)
        return False

    def _get_street_network(self, mode, request):
        if app.config[str('DISABLE_DATABASE')]:
            return self._streetnetwork_backend_manager.get_street_network_legacy(self, mode, request)
        else:
            # We get the name of the column in the database corresponding to the mode used in the request
            # And we get the value of this column for this instance
            column_in_db = "street_network_{}".format(mode)
            streetnetwork_backend_conf = getattr(self, column_in_db)
            return self._streetnetwork_backend_manager.get_street_network_db(self, streetnetwork_backend_conf)

    def get_street_network(self, mode, request):
        if mode != fallback_modes.FallbackModes.car.name:
            return self._get_street_network(mode, request)

        walking_service = self._get_street_network(fallback_modes.FallbackModes.walking.name, request)
        car_service = self._get_street_network(fallback_modes.FallbackModes.car.name, request)
        return street_network.CarWithPark(
            instance=self, walking_service=walking_service, car_service=car_service
        )

    def get_all_street_networks(self):
        if app.config[str('DISABLE_DATABASE')]:
            return self._streetnetwork_backend_manager.get_all_street_networks_legacy(self)
        else:
            return self._streetnetwork_backend_manager.get_all_street_networks_db(self)

    def get_all_ridesharing_services(self):
        return self.ridesharing_services_manager.get_all_ridesharing_services()

    def get_all_bss_providers(self):
        return self.bss_provider_manager.get_providers()

    def get_all_car_parking_providers(self):
        return self.car_parking_provider_manager.get_providers()

    def get_autocomplete(self, requested_autocomplete):
        if not requested_autocomplete:
            return self.autocomplete
        autocomplete = global_autocomplete.get(requested_autocomplete)
        if not autocomplete:
            raise TechnicalError('autocomplete {} not available'.format(requested_autocomplete))
        return autocomplete

    def get_best_boarding_position(self, from_id, to_id):
        if not self.best_boarding_positions:
            return []
        key = make_origin_destination_key(from_id, to_id)
        return self.best_boarding_positions.get(key, [])
