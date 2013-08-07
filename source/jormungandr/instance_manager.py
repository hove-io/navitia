# coding=utf-8

import json
from shapely import geometry, geos
import ConfigParser
import zmq
from threading import Lock, Thread, Event
import type_pb2
import request_pb2
import response_pb2
import glob
from singleton import singleton
import importlib
from renderers import render_from_protobuf
from error import generate_error
import logging
import sys

class Instance:
    def __init__(self):
        self.geom = None
        self.socket = None
        self.socket_path = None
        self.poller = zmq.Poller()
        self.lock = Lock()
        self.script = None

class DeadSocketException(Exception):
    def __init__(self, message):
        Exception.__init__(self, message)


        
class RegionNotFound(Exception):
    def __init__(self, message):
        Exception.__init__(self, message)

class ApiNotFound(Exception):
    def __init__(self, message):
        Exception.__init__(self, message)

class InvalidArguments(Exception):
    def __init__(self, message):
        Exception.__init__(self, message)

@singleton
class NavitiaManager:
    """ Permet de coordonner les différents NAViTiA gérés par le front-end
    Un identifiant correspond à une socket ZMQ Navitia où se connecter
    Plusieurs identifiants peuvent se connecter à une même socket

    De plus il est possible de définir la zone géographique (au format WKT) couverte par un identifiant
    """

    def initialisation(self, ini_file=None):
        """ Charge la configuration à partir d'un fichier ini indiquant les chemins des fichiers contenant :
        - géométries de la région sur laquelle s'applique le moteur
        - la socket pour chaque identifiant navitia
        """

        self.instances = {}
        self.contributors = {}
        self.context = zmq.Context()
        self.default_socket = None

        ini_files = []
        if ini_file == None :
            ini_files = glob.glob("/etc/jormungandr.d/*.ini")
        else:
            ini_files = [ini_file]

        for file_name in ini_files:
            conf = ConfigParser.ConfigParser()
            conf.read(file_name)
            instance =  Instance()
            instance.socket_path = conf.get('instance' , 'socket')
            instance.socket = self.context.socket(zmq.REQ)
            instance.socket.connect(instance.socket_path)
            instance.poller.register(instance.socket, zmq.POLLIN)
            if conf.has_option('instance', 'script') :
                instance.script = importlib.import_module(conf.get('instance','script')).Script()
            else:
                instance.script = importlib.import_module("scripts.default").Script()

            for section in conf.sections():
                if section in instance.script.apis:
                    api = instance.script.apis[section]
                    for option in conf.options(section):
                        if option in api["arguments"]:
                            argument = api["arguments"][option]
                            if type(argument.defaultValue) == type(True):
                                argument.defaultValue = conf.getboolean(section, option)
                            elif type(argument.defaultValue) == type(1.0):
                                argument.defaultValue = conf.getfloat(section, option)
                            elif type(argument.defaultValue) == type(1):
                                argument.defaultValue = conf.getint(section, option)
                            else:
                                argument.defaultValue = conf.get(section, option)

            self.instances[conf.get('instance' , 'key')] = instance
        self.thread_event = Event()
        self.thread = Thread(target = self.thread_ping)
        self.thread.start()

    def dispatch(self, arguments, region, api, request=None):
        if region in self.instances:
            if api in self.instances[region].script.apis:
                try:
                    api_func = getattr(self.instances[region].script, api)
                    api_answer = api_func(arguments, region)
                    return api_func(arguments, region)
                except DeadSocketException, e:
                    if request:
                        log = logging.getLogger("werkzeug")
                        log.error(request.url)
                    return generate_error(e.message, status=503)
#except AttributeError:
#                    return generate_error("Unknown api : " + api, status=404)
            else:
                return generate_error("Unknown api : " + api, status=404)
        else:
             return generate_error(region + " not found", status=404)

    def send_and_receive(self, request, region = None, timeout=10000):
        if region in self.instances:
            instance = self.instances[region]
            instance.lock.acquire()
            instance.socket.send(request.SerializeToString())#, zmq.NOBLOCK, copy=False)
            socks = dict(instance.poller.poll(timeout))
            if socks.get(instance.socket) == zmq.POLLIN:
                pb = instance.socket.recv()
                instance.lock.release()
                resp = response_pb2.Response()
                resp.ParseFromString(pb)
                return resp
            else :
                instance.socket.setsockopt(zmq.LINGER, 0)
                instance.socket.close()
                instance.poller.unregister(instance.socket)
                instance.socket = self.context.socket(zmq.REQ)
                instance.socket.connect(instance.socket_path)
                instance.poller.register(instance.socket)
                instance.lock.release()
                raise DeadSocketException(region+" is a dead socket (" + instance.socket_path + ")")
        else:
            raise RegionNotFound(region +" not found ")


    def thread_ping(self, timer=1):
        req = request_pb2.Request()
        req.requested_api = type_pb2.METADATAS
        while not self.thread_event.is_set():
            for key, instance in self.instances.iteritems():
                try:
                    resp = self.send_and_receive(req, key, timeout=1000)
                    if resp:
                        try:
                            parsed = json.loads(resp.metadatas.shape)
                            check = True
                            if 'contributors' in parsed:
                                for contributor in parser.contributors:
                                    self.contributors[contributor] = key
                            if 'coordinates' in parsed:
                                if len(parsed['coordinates']) > 0:
                                    for coords in parsed['coordinates']:
                                        for pair_coord in coords:
                                            for coord in pair_coord:
                                                if int(coord) == 0:
                                                    check = False
                            if check:
                                instance.geom = geometry.shape(parsed)
                        except:
                            pass
                except DeadSocketException:
                    pass
                    #print e
                except geos.ReadingError:
                    instance.geom = None

            self.thread_event.wait(timer)


    def stop(self) :
        self.thread_event.set()

    def key_of_id(self, object_id):
        """ Retrieves the key of the region of a given id
            if it's a coord calls key_of_coord
            Return the region key, or None if it doesn't exists
        """
        if len(object_id)>=6 and object_id[:6] == "coord:":
            if object_id.count(":") == 2:
                lon, lat = object_id.split(":")[1:]
                return self.key_of_coord(lon, lat)
            else:
                return None
        else:
            try:
                contributor = object_id[:object_id.find(":")]
            except ValueError:
                return None
            if contributor in self.contributors:
                return self.contributors[contributor]
        return None

    def key_of_coord(self, lon, lat):
        """ Étant donné une coordonnée, retourne à quelle clef NAViTiA cela correspond

        Retourne None si on a rien trouvé
        """
        p = geometry.Point(lon,lat)
        for key, instance in self.instances.iteritems():
            if instance.geom and instance.geom.contains(p):
                return key
        return None

