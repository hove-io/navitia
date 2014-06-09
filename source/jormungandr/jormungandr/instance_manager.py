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


def sort_instances(instances):
    """
    sort a list of possible instances

    we want first the non free instances then the free ones
    """
    def instance_compare(a, b):
        if a.is_free != b.is_free:
            # if the boolean is different, we want 1 if a is free, else -1
            return a.is_free - b.is_free
        #TODO what to do if we have 2 free instances or 2 private instances ?
        #TODO it would be better to just return the best one because it is always the wanted behavior
        return 1  # for the moment we keep the same order

    instances.sort(instance_compare)
    return instances


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

    def thread_ping(self, timer=10):
        req = request_pb2.Request()
        req.requested_api = type_pb2.METADATAS
        while not self.thread_event.is_set():
            for key, instance in self.instances.iteritems():
                try:
                    resp = instance.send_and_receive(req, timeout=1000)
                except DeadSocketException:
                    instance.geom = None
                    continue
                if resp.HasField("metadatas"):
                    try:
                        instance.geom = wkt.loads(resp.metadatas.shape)
                    except ReadingError:
                        instance.geom = None
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
                if only_one:
                    return sort_instances(instances)[0].name
                return [i.name for i in instances]
        raise RegionNotFound(object_id=object_id)

    def key_of_coord(self, lon, lat, only_one=True):
        """ For a given coordinate, return the corresponding Navitia key

        Raise RegionNotFound if nothing found

        if only_one param is true return only one key, else return the list of possible keys
        """
        p = geometry.Point(lon, lat)
        valid_instances = []
        # a valid instance is an instance containing the coord and accessible by the user
        for key, instance in self.instances.iteritems():
            if instance.geom and instance.geom.contains(p) \
                    and authentification.has_access(instance, abort=False):  #TODO, pb how to check the api ?
                valid_instances.append(key)

        if valid_instances:
            if only_one:
                #If we have only one instance we return the 'best one'
                return sort_instances(valid_instances)[0]
            else:
                return valid_instances

        raise RegionNotFound(lon=lon, lat=lat)

    def region_exists(self, region_str):
        if (region_str in self.instances.keys()):
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
