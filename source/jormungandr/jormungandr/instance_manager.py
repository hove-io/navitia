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

from shapely import geometry
import ConfigParser
import zmq
from threading import Thread, Event
from navitiacommon import type_pb2, request_pb2, models
import glob
from jormungandr.singleton import singleton
import logging
from jormungandr.protobuf_to_dict import protobuf_to_dict
from jormungandr.exceptions import ApiNotFound, RegionNotFound,\
    DeadSocketException, InvalidArguments
from jormungandr import authentication, cache, app
from jormungandr.instance import Instance


def instances_comparator(instance1, instance2):
    """
    compare the instances for journey computation

    we want first the non free instances then the free ones
    """
    jormun_bdd_instance1 = models.Instance.get_by_name(instance1)
    jormun_bdd_instance2 = models.Instance.get_by_name(instance2)
    #TODO the is_free should be in the instances, no need to fetch the bdd for this
    if not jormun_bdd_instance1 and not jormun_bdd_instance2:
        raise RegionNotFound(custom_msg="technical problem, impossible "
                                        "to find region {i} and region{j} in jormungandr database".format(
            i=jormun_bdd_instance1, j=jormun_bdd_instance2))
    if not jormun_bdd_instance1:
        return -1
    if not jormun_bdd_instance2:
        return 1

    if jormun_bdd_instance1.is_free != jormun_bdd_instance2.is_free:
        return jormun_bdd_instance1.is_free - jormun_bdd_instance2.is_free

    # TODO choose the smallest region ?
    # take the origin/destination coords into account and choose the region with the center nearest to those coords ?
    return 1


def choose_best_instance(instances):
    """
    get the best instance in term of the instances_comparator
    """
    best = None
    for i in instances:
        if not best or instances_comparator(i, best) > 0:
            best = i
    return best

@singleton
class InstanceManager(object):

    """ Permet de coordonner les différents NAViTiA gérés par le front-end
    Un identifiant correspond à une socket ZMQ Navitia où se connecter
    Plusieurs identifiants peuvent se connecter à une même socket

    De plus il est possible de définir la zone géographique (au format WKT)
    couverte par un identifiant
    """

    def __init__(self, ini_files=None, instances_dir=None, start_ping=False):
        # if a .ini file is defined in the settings we take it
        # else we load all .ini file found in the INSTANCES_DIR
        if ini_files is None and instances_dir is None:
            raise ValueError("ini_files or instance_dir has to be set")
        if ini_files:
            self.ini_files = ini_files
        else:
            self.ini_files = glob.glob(instances_dir + '/*.ini')
        self.start_ping = start_ping

    def initialisation(self):
        """ Charge la configuration à partir d'un fichier ini indiquant
            les chemins des fichiers contenant :
            - géométries de la région sur laquelle s'applique le moteur
            - la socket pour chaque identifiant navitia
        """

        self.instances = {}
        self.context = zmq.Context()
        self.default_socket = None

        for file_name in self.ini_files:
            logging.getLogger(__name__).info("Initialisation, reading file : " + file_name)
            conf = ConfigParser.ConfigParser()
            conf.read(file_name)
            instance = Instance(self.context, conf.get('instance', 'key'))
            instance.socket_path = conf.get('instance', 'socket')

            self.instances[conf.get('instance', 'key')] = instance

        #we fetch the krakens metadata first
        # not on the ping thread to always have the data available (for the tests for example)
        self.init_kraken_instances()

        self.thread_event = Event()
        self.thread = Thread(target=self.thread_ping)
        #daemon thread does'nt block the exit of a process
        self.thread.daemon = True
        if self.start_ping:
            self.thread.start()

    def _clear_cache(self):
        logging.getLogger(__name__).info('clear cache')
        cache.delete_memoized(self._all_keys_of_id)

    def dispatch(self, arguments, api, instance_name=None):
        if instance_name not in self.instances:
            raise RegionNotFound(instance_name)

        instance = self.instances[instance_name]

        scenario = instance.scenario(arguments.get('_override_scenario'))
        if not hasattr(scenario, api) or not callable(getattr(scenario, api)):
            raise ApiNotFound(api)

        api_func = getattr(scenario, api)
        resp = api_func(arguments, instance)
        if resp.HasField("publication_date") and\
          instance.publication_date != resp.publication_date:
            self._clear_cache()
            instance.publication_date = resp.publication_date
        return resp

    def init_kraken_instances(self):
        """
        Call all kraken instances (as found in the instances dir) and store it's metadata
        """
        req = request_pb2.Request()
        req.requested_api = type_pb2.METADATAS
        for instance in self.instances.itervalues():
            if instance.init():
                self._clear_cache()

    def thread_ping(self, timer=10):
        """
        fetch krakens metadata
        """
        while not self.thread_event.is_set():
            self.init_kraken_instances()
            self.thread_event.wait(timer)

    def stop(self):
        if not self.thread_event.is_set():
            self.thread_event.set()

    def _filter_authorized_instances(self, instances, api):
        if not instances:
            return None
        user = authentication.get_user(token=authentication.get_token())
        valid_regions = [i for i in instances if authentication.has_access(i,
            abort=False, user=user, api=api)]
        if not valid_regions:
            authentication.abort_request(user)
        return valid_regions

    @cache.memoize(app.config['CACHE_CONFIGURATION'].get('TIMEOUT_PTOBJECTS',None))
    def _all_keys_of_id(self, object_id):
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
            return self._all_keys_of_coord(flon, flat)
        instances = [i.name for i in self.instances.itervalues() if i.has_id(object_id)]
        if not instances:
            raise RegionNotFound(object_id=object_id)
        return instances

    def _all_keys_of_coord(self, lon, lat):
        p = geometry.Point(lon, lat)
        instances = [i.name for i in self.instances.itervalues() if i.has_point(p)]
        logging.getLogger(__name__).debug("all_keys_of_coord(self, {}, {}) returns {}".format(lon, lat, instances))
        if not instances:
            raise RegionNotFound(lon=lon, lat=lat)
        return instances

    def region_exists(self, region_str):
        if region_str in self.instances.keys():
            return True
        else:
            raise RegionNotFound(region=region_str)

    def get_region(self, region_str=None, lon=None, lat=None, object_id=None,
            api='ALL'):
        return self.get_regions(region_str, lon, lat, object_id, api,
                only_one=True)

    def get_regions(self, region_str=None, lon=None, lat=None, object_id=None,
            api='ALL', only_one=False):
        available_regions = []
        if region_str and self.region_exists(region_str):
            available_regions = [region_str]
        elif lon and lat:
            available_regions = self._all_keys_of_coord(lon, lat)
        elif object_id:
            available_regions = self._all_keys_of_id(object_id)
        else:
            available_regions = self.instances.keys()

        valid_regions = self._filter_authorized_instances(available_regions, api)
        if valid_regions:
            return choose_best_instance(valid_regions) if only_one else valid_regions
        elif available_regions:
            authentication.abort_request(user=authentication.get_user())
        raise RegionNotFound(region=region_str, lon=lon, lat=lat,
                             object_id=object_id)

    def regions(self, region=None, lon=None, lat=None):
        response = {'regions': []}
        regions = []
        if region or lon or lat:
            regions.append(self.get_region(region_str=region, lon=lon,
                                           lat=lat))
        else:
            regions = self.get_regions()
        for key_region in regions:
            req = request_pb2.Request()
            req.requested_api = type_pb2.METADATAS
            try:
                resp = self.instances[key_region].send_and_receive(req, timeout=1000)
                resp_dict = protobuf_to_dict(resp.metadatas)
            except DeadSocketException:
                resp_dict = {
                    "status": "dead",
                    "error": {
                        "code": "dead_socket",
                        "value": "The region {} is dead".format(key_region)
                    }
                }
            if resp_dict.get('status') == 'no_data' and not region and not lon and not lat:
                continue
            resp_dict['region_id'] = key_region
            response['regions'].append(resp_dict)
        return response
