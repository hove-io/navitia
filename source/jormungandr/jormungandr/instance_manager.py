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
from flask import json

from shapely import geometry
from zmq import green as zmq
from navitiacommon import type_pb2, request_pb2
import glob
import logging
from jormungandr.protobuf_to_dict import protobuf_to_dict
from jormungandr.exceptions import ApiNotFound, RegionNotFound, DeadSocketException, InvalidArguments
from jormungandr.authentication import abort_request, can_read_user
from jormungandr import authentication, cache, memory_cache, app
from jormungandr.instance import Instance
import gevent
import os


def instances_comparator(instance1, instance2):
    """
    Compare the instances for journey computation
    :return :   <0 if instance1 has priority over instance2
                >0 if instance2 has priority over instance1
    """
    # Choose the instance with greater priority
    if instance1.priority != instance2.priority:
        return instance2.priority - instance1.priority

    # Choose the non-free instance
    if instance1.is_free != instance2.is_free:
        return instance1.is_free - instance2.is_free

    # TODO choose the smallest region ?
    # take the origin/destination coords into account and choose the region with the center nearest to those coords ?

    # Sort by alphabetical order
    return 1 if instance1.name > instance2.name else -1


def choose_best_instance(instances):
    """
    Get the best instance in term of the instances_comparator
    If instances are equals, first instance from the list is returned
    """
    best = None
    for i in instances:
        if not best or instances_comparator(i, best) < 0:
            best = i
    return best


class InstanceManager(object):
    """
    Handle the different Kraken instances
    A Kraken instance id is associated to a zmq socket and possibly some custom configuration
    """

    def __init__(
        self,
        streetnetwork_backend_manager,
        instances_dir=None,
        instance_filename_pattern='*.json',
        start_ping=False,
    ):
        self._streetnetwork_backend_manager = streetnetwork_backend_manager
        # loads all json files found in 'instances_dir' that matches 'instance_filename_pattern'
        self.configuration_files = (
            glob.glob(instances_dir + '/' + instance_filename_pattern) if instances_dir else []
        )
        self.start_ping = start_ping
        self.instances = {}
        self.context = zmq.Context()
        self.is_ready = False  # type: bool

    def __repr__(self):
        return '<InstanceManager>'

    def register_instance(self, config):
        logging.getLogger(__name__).debug("instance configuration: %s", config)
        name = config['key']
        instance = Instance(
            self.context,
            name,
            config['zmq_socket'],
            config.get('street_network'),
            config.get('ridesharing', []),
            config.get('realtime_proxies', []),
            config.get('zmq_socket_type', app.config.get('ZMQ_DEFAULT_SOCKET_TYPE', 'persistent')),
            config.get('default_autocomplete', None),
            config.get('equipment_details_providers', []),
            self._streetnetwork_backend_manager,
            config.get('external_services_providers', []),
            config.get('pt_planners', {}),
            config.get('pt_journey_fares', {}),
            config.get('ghost_words', []),
            best_boarding_positions_dir=app.config.get(str('BEST_BOARDING_POSITIONS_DIR'), None),
            olympics_forbidden_uris=config.get('olympics_forbidden_uris', None),
            additional_params_period=config.get('additional_parameters_activation_period', None),
            use_multi_reverse=config.get('use_multi_reverse', False),
            resp_content_limit_bytes=config.get('resp_content_limit_bytes', None),
            resp_content_limit_endpoints_whitelist=config.get('resp_content_limit_endpoints_whitelist', None),
            individual_bss_provider=config.get('individual_bss_provider', []),
        )

        self.instances[instance.name] = instance

    def initialization(self):
        """
        Load  configuration from ini file providing files paths to:
            - region geometry
            - zmq socket
        """
        self.instances.clear()
        for key, value in os.environ.items():
            if key.startswith('JORMUNGANDR_INSTANCE_'):
                logging.getLogger(__name__).info("Initialisation, reading: %s", key)
                config_data = json.loads(value)
                self.register_instance(config_data)

        for file_name in self.configuration_files:
            logging.getLogger(__name__).info("Initialisation, reading file: %s", file_name)
            with open(file_name) as f:
                config_data = json.load(f)
                self.register_instance(config_data)

        # we fetch the krakens metadata first
        # not on the ping thread to always have the data available (for the tests for example)
        self.init_kraken_instances()

        if self.start_ping:
            gevent.spawn(self.thread_ping)

    def get_instance_scenario_name(self, instance_name, override_scenario):
        if override_scenario:
            return override_scenario

        instance = self.instances[instance_name]
        instance_db = instance.get_models()
        scenario_name = instance_db.scenario if instance_db else 'new_default'
        return scenario_name

    def dispatch(self, arguments, api, instance_name=None):
        if instance_name not in self.instances:
            raise RegionNotFound(instance_name)

        instance = self.instances[instance_name]

        scenario = instance.scenario(arguments.get('_override_scenario'))
        if not hasattr(scenario, api) or not callable(getattr(scenario, api)):
            raise ApiNotFound(api)

        api_func = getattr(scenario, api)
        resp = api_func(arguments, instance)
        return resp

    def init_kraken_instances(self):
        """
        Call all kraken instances (as found in the instances dir) and store it's metadata
        """
        futures = []
        for instance in self.instances.values():
            if not instance.is_initialized:
                futures.append(gevent.spawn(instance.init))

        gevent.wait(futures)

    def thread_ping(self, timer=10):
        """
        fetch krakens metadata
        """
        while [i for i in self.instances.values() if not i.is_initialized]:
            self.init_kraken_instances()
            gevent.sleep(timer)
        logging.getLogger(__name__).debug('end of ping thread')

    def stop(self):
        if not self.thread_event.is_set():
            self.thread_event.set()

    def _get_authorized_instances(self, user, api):
        authorized_instances = [
            i
            for name, i in self.instances.items()
            if authentication.has_access(name, abort=False, user=user, api=api)
        ]

        if not authorized_instances:
            context = 'User has no access to any instance'
            authentication.abort_request(user, context)
        return authorized_instances

    def _find_coverage_by_object_id_in_instances(self, instances, object_id):
        if object_id.count(";") == 1 or object_id[:6] == "coord:":
            if object_id.count(";") == 1:
                lon, lat = object_id.split(";")
            else:
                lon, lat = object_id[6:].split(":")
            try:
                flon = float(lon)
                flat = float(lat)
            except:
                raise InvalidArguments(object_id)
            return self._all_keys_of_coord_in_instances(instances, flon, flat)

        return self._all_keys_of_id_in_instances(instances, object_id)

    def _all_keys_of_id_in_instances(self, instances, object_id):
        valid_instances = []
        for instance in instances:
            if self._exists_id_in_instance(instance.name, instance.publication_date, object_id):
                valid_instances.append(instance)
        if not valid_instances:
            raise RegionNotFound(object_id=object_id)

        return valid_instances

    @cache.memoize(app.config[str('CACHE_CONFIGURATION')].get(str('TIMEOUT_PTOBJECTS'), None))
    def _exists_id_in_instance(self, instance_name, instance_publication_date, object_id):
        """
        instance's published_date is usually provided as extra_cache_key to invalidate the cache when updating the ntfs
        """
        instance = self.instances.get(instance_name)
        if not instance:
            logging.getLogger(__name__).error("Instance {} not found".format(instance_name))
            return False
        return instance.has_id(object_id)

    def _all_keys_of_coord_in_instances(self, instances, lon, lat):
        p = geometry.Point(lon, lat)
        valid_instances = [i for i in instances if i.has_point(p)]
        logging.getLogger(__name__).debug(
            "_all_keys_of_coord_in_instances(self, {}, {}) returns {}".format(lon, lat, instances)
        )
        if not valid_instances:
            raise RegionNotFound(lon=lon, lat=lat)
        return valid_instances

    def get_region(self, region_str=None, lon=None, lat=None, object_id=None, api='ALL'):
        return self.get_regions(region_str, lon, lat, object_id, api, only_one=True)

    def get_regions(self, region_str=None, lon=None, lat=None, object_id=None, api='ALL', only_one=False):
        valid_instances = self.get_instances(region_str, lon, lat, object_id, api)
        if not valid_instances:
            raise RegionNotFound(region=region_str, lon=lon, lat=lat, object_id=object_id)
        if only_one:
            return choose_best_instance(valid_instances).name
        else:
            return [i.name for i in valid_instances]

    def get_instances(self, name=None, lon=None, lat=None, object_id=None, api='ALL'):
        if name and name not in self.instances:
            raise RegionNotFound(region=name)

        # Request without token or bad token makes a request exception and exits with a message
        # get_user is cached hence access to database only once when cache expires.
        user = authentication.get_user(token=authentication.get_token())
        valid_instances = []
        if name:
            # Requests with a coverage
            if authentication.has_access(name, abort=False, user=user, api=api):
                valid_instances = [self.instances[name]]
        else:
            # Requests without any coverage
            # fetch all the authorized instances (free + private) using cached function has_access()
            authorized_instances_name = self.get_all_available_instances_names(user)
            authorized_instances = [self.instances[i_name] for i_name in authorized_instances_name]
            if not authorized_instances:
                # user doesn't have access to any of the instances
                context = 'User has no access to any instance'
                authentication.abort_request(user=user, context=context)

            if lon and lat:
                valid_instances = self._all_keys_of_coord_in_instances(authorized_instances, lon, lat)
            elif object_id:
                valid_instances = self._find_coverage_by_object_id_in_instances(authorized_instances, object_id)
            else:
                valid_instances = authorized_instances

        if not valid_instances:
            # user doesn't have access to any of the instances
            context = "User has no access to any instance or instance doesn't exist"
            authentication.abort_request(user=user, context=context)
        else:
            return valid_instances

    def get_kraken_coverages(self, regions, request_id=None):
        response = []

        for key_region in regions:
            req = request_pb2.Request()
            req.requested_api = type_pb2.METADATAS

            try:
                resp = self.instances[key_region].send_and_receive(req, request_id=request_id)
                resp_dict = protobuf_to_dict(resp.metadatas)
            except DeadSocketException:
                resp_dict = {
                    "status": "dead",
                    "error": {"code": "dead_socket", "value": "The region {} is dead".format(key_region)},
                }
            resp_dict['region_id'] = key_region
            response.append(resp_dict)
        return response

    def regions(self, region=None, lon=None, lat=None, request_id=None):
        response = {'regions': []}
        regions = []
        if region or lon or lat:
            regions.append(self.get_region(region_str=region, lon=lon, lat=lat))
            kraken_coverages = self.get_kraken_coverages(regions, request_id=request_id)
        else:
            regions = self.get_regions()
            if regions:
                regions.sort()

            @cache.memoize(app.config.get(str('CACHE_CONFIGURATION'), {}).get(str('TIMEOUT_KRAKEN_COVERAGES'), 60))
            def get_cached_kraken_coverages(regions_list):
                return self.get_kraken_coverages(regions_list, request_id=request_id)

            kraken_coverages = get_cached_kraken_coverages(regions)
        for kraken_coverage in kraken_coverages:
            if kraken_coverage.get('status') == 'no_data' and not region and not lon and not lat:
                continue
            response['regions'].append(kraken_coverage)
        return response

    @memory_cache.memoize(app.config[str('MEMORY_CACHE_CONFIGURATION')].get(str('TIMEOUT_AUTHENTICATION'), 30))
    @cache.memoize(app.config[str('CACHE_CONFIGURATION')].get(str('TIMEOUT_AUTHENTICATION'), 300))
    def get_all_available_instances_names(self, user):
        result = []
        if app.config.get('PUBLIC', False) or app.config.get('DISABLE_DATABASE', False):
            return [key for key in self.instances]

        if not user:
            logging.getLogger(__name__).warning('get all available instances no user')
            # for not-public navitia a user is mandatory
            # To manage database error of the following type we should fetch one more time from database
            # Can connect to database but at least one table/attribute is not accessible due to transaction problem
            if can_read_user():
                abort_request(user=user)
            else:
                return result

        bdd_instances = user.get_all_available_instances()
        for bdd_instance in bdd_instances:
            if bdd_instance.name in self.instances:
                result.append(bdd_instance.name)
        return result
