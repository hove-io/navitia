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

from shapely import geometry, wkt
from shapely.geos import ReadingError
import ConfigParser
import zmq
from threading import Thread, Event
from navitiacommon import type_pb2, request_pb2, models
import glob
from jormungandr.singleton import singleton
from importlib import import_module
import logging
from jormungandr.protobuf_to_dict import protobuf_to_dict
from jormungandr.exceptions import ApiNotFound, RegionNotFound, DeadSocketException
from jormungandr import app, authentification
from jormungandr.instance import Instance
import traceback


def choose_best_instance(instances):
    """
    chose the best instance
    we want first the non free instances then the free ones
    """
    best = None
    for i in instances:
        jormun_bdd_instance = models.Instance.get_by_name(i)
        #TODO the is_free should be in the instances, no need to fetch the bdd for this
        if not jormun_bdd_instance:
            raise RegionNotFound(custom_msg="technical problem, impossible "
                                            "to find region {i} in jormungandr database".format(i=i))
        if not best or not jormun_bdd_instance.is_free:
            #TODO what to do if we have 2 free instances or 2 private instances ?
            best = jormun_bdd_instance

    return best.name


@singleton
class InstanceManager(object):

    """ Permet de coordonner les différents NAViTiA gérés par le front-end
    Un identifiant correspond à une socket ZMQ Navitia où se connecter
    Plusieurs identifiants peuvent se connecter à une même socket

    De plus il est possible de définir la zone géographique (au format WKT)
    couverte par un identifiant
    """

    def __init__(self):
        pass

    def initialisation(self, start_ping=True):
        """ Charge la configuration à partir d'un fichier ini indiquant
            les chemins des fichiers contenant :
            - géométries de la région sur laquelle s'applique le moteur
            - la socket pour chaque identifiant navitia
        """

        self.instances = {}
        self.context = zmq.Context()
        self.default_socket = None

        # if a .ini file is defined in the settings we take it
        # else we load all .ini file found in the INSTANCES_DIR
        if 'INI_FILES' in app.config:
            ini_files = app.config['INI_FILES']
        else:
            ini_files = glob.glob(app.config['INSTANCES_DIR'] + '/*.ini')

        for file_name in ini_files:
            logging.info("Initialisation, reading file : " + file_name)
            conf = ConfigParser.ConfigParser()
            conf.read(file_name)
            instance = Instance(self.context, conf.get('instance', 'key'))
            instance.socket_path = conf.get('instance', 'socket')

            if conf.has_option('instance', 'script'):
                module = import_module(conf.get('instance', 'script'))
                instance.script = module.Script()
            else:
                module = import_module("jormungandr.scripts.default")
                instance.script = module.Script()

            # we give all functional parameters to the script
            if conf.has_section('functional'):
                functional_params = dict(conf.items('functional'))
                instance.script.functional_params = functional_params

            self.instances[conf.get('instance', 'key')] = instance

        #we fetch the krakens metadata first
        # not on the ping thread to always have the data available (for the tests for example
        self.init_kraken_instances()

        self.thread_event = Event()
        self.thread = Thread(target=self.thread_ping)
        #daemon thread does'nt block the exit of a process
        self.thread.daemon = True
        if start_ping:
            self.thread.start()

    def dispatch(self, arguments, api, instance_obj=None, instance_name=None,
                 request=None):
        if instance_obj:
            instance_name = instance_obj.name
        if instance_name in self.instances:
            instance = self.instances[instance_name]
            if api in instance.script.apis:
                api_func = getattr(instance.script, api)
                return api_func(arguments, instance)
            else:
                raise ApiNotFound(api)
        else:
            raise RegionNotFound(instance_name)

    def init_kraken_instances(self):
        """
        Call all kraken instances (as found in the instances dir) and store it's metadata
        """
        req = request_pb2.Request()
        req.requested_api = type_pb2.METADATAS
        for key, instance in self.instances.iteritems():
            try:
                resp = instance.send_and_receive(req, timeout=1000)
            except DeadSocketException:
                instance.geom = None
                continue
            if resp.HasField("metadatas"):
                if resp.metadatas.shape and resp.metadatas.shape != "":
                    try:
                        instance.geom = wkt.loads(resp.metadatas.shape)
                    except ReadingError:
                        instance.geom = None
                else:
                    instance.geom = None
                instance.timezone = resp.metadatas.timezone

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

    def key_of_id(self, object_id, only_one=True):
        """ Retrieves the key of the region of a given id
            if it's a coord calls key_of_coord
            Return one region key if only_one param is true, or None if it doesn't exists
            and the list of possible regions if only_one is set to False
        """
        if object_id.count(";") == 1 or object_id[:6] == "coord:":
            if object_id.count(";") == 1:
                lon, lat = object_id.split(";")
            else:
                lon, lat = object_id[6:].split(":")
            try:
                flon = float(lon)
                flat = float(lat)
            except:
                raise RegionNotFound(object_id=object_id)
            return self.key_of_coord(flon, flat, only_one)

        ptobject = models.PtObject.get_from_uri(object_id)
        if ptobject:
            instances = ptobject.instances()
            if len(instances) > 0:
                available_instances = [i.name for i in instances if authentification.has_access(i, abort=False)]

                if not available_instances:
                    raise RegionNotFound(custom_msg="id {i} exists but not in regions available for user"
                                         .format(i=object_id))

                if only_one:
                    return choose_best_instance(available_instances)
                return available_instances

        raise RegionNotFound(object_id=object_id)

    def key_of_coord(self, lon, lat, only_one=True):
        """ For a given coordinate, return the corresponding Navitia key

        Raise RegionNotFound if nothing found

        if only_one param is true return only one key, else return the list of possible keys
        """
        p = geometry.Point(lon, lat)
        valid_instances = []
        # a valid instance is an instance containing the coord and accessible by the user
        found_one = False
        for key, instance in self.instances.iteritems():
            if instance.geom and instance.geom.contains(p):
                found_one = True
                jormun_instance = models.Instance.get_by_name(key)
                if not jormun_instance:
                    raise RegionNotFound(custom_msg="technical problem, impossible "
                                                    "to find region {r} in jormungandr database"
                                         .format(r=key))
                if authentification.has_access(jormun_instance, abort=False):  #TODO, pb how to check the api ?
                    valid_instances.append(key)

        if valid_instances:
            if only_one:
                #If we have only one instance we return the 'best one'
                return choose_best_instance(valid_instances)
            else:
                return valid_instances
        elif found_one:
            raise RegionNotFound(custom_msg="coord {lon};{lat} are covered, "
                                            "but not in regions available for user"
                                 .format(lon=lon, lat=lat))

        raise RegionNotFound(lon=lon, lat=lat)

    def region_exists(self, region_str):
        if region_str in self.instances.keys():
            return True
        else:
            raise RegionNotFound(region=region_str)

    def get_region(self, region_str=None, lon=None, lat=None, object_id=None):
        if region_str and self.region_exists(region_str):
            return region_str
        elif lon and lat:
            return self.key_of_coord(lon, lat)
        elif object_id:
            return self.key_of_id(object_id)
        else:
            raise RegionNotFound(region=region_str, lon=lon, lat=lat,
                                 object_id=object_id)

    def regions(self, region=None, lon=None, lat=None):
        response = {'regions': []}
        regions = []
        if region or lon or lat:
            regions.append(self.get_region(region_str=region, lon=lon,
                                           lat=lat))
        else:
            regions = self.instances.keys()
        for key_region in regions:
            req = request_pb2.Request()
            req.requested_api = type_pb2.METADATAS
            try:
                resp = self.instances[key_region].send_and_receive(req,
                                                               timeout=1000)
                resp_dict = protobuf_to_dict(resp.metadatas)
            except DeadSocketException:
                resp_dict = {
                    "status": "dead",
                    "error" : {
                        "code": "dead_socket",
                        "value": "The region %s is dead".format(key_region)
                    }
                }
            resp_dict['region_id'] = key_region
            response['regions'].append(resp_dict)
        return response
