# coding=utf-8
from shapely import geometry, geos, wkt
import ConfigParser
import zmq
from threading import Lock, Thread, Event
import type_pb2
import request_pb2
import response_pb2
import glob
from singleton import singleton
import importlib
import logging
from renderers import protobuf_to_dict
from jormungandr_exceptions import ApiNotFound, RegionNotFound, DeadSocketException
from app import app


class Instance:
    def __init__(self):
        self.geom = None
        self.socket = None
        self.socket_path = None
        self.poller = zmq.Poller()
        self.lock = Lock()
        self.script = None


@singleton
class NavitiaManager(object):
    """ Permet de coordonner les différents NAViTiA gérés par le front-end
    Un identifiant correspond à une socket ZMQ Navitia où se connecter
    Plusieurs identifiants peuvent se connecter à une même socket

    De plus il est possible de définir la zone géographique (au format WKT) couverte par un identifiant
    """

    def __init__(self):
        pass


    def initialisation(self):
        """ Charge la configuration à partir d'un fichier ini indiquant
            les chemins des fichiers contenant :
            - géométries de la région sur laquelle s'applique le moteur
            - la socket pour chaque identifiant navitia
        """

        self.instances = {}
        self.contributors = {}
        self.context = zmq.Context()
        self.default_socket = None

        ini_files = []
        ini_files = glob.glob(app.config['INSTANCES_DIR'] + '/*.ini')

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

            self.instances[conf.get('instance' , 'key')] = instance
        self.thread_event = Event()
        self.thread = Thread(target = self.thread_ping)
        self.thread.start()


    def dispatch(self, arguments, region, api, request=None):
        if region in self.instances:
            if api in self.instances[region].script.apis:
                api_func = getattr(self.instances[region].script, api)
                return api_func(arguments, region)
            else:
                raise ApiNotFound(api)
        else:
            raise RegionNotFound(region)

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
                logging.error("La requête : " + str(request) + " a echoue")
                raise DeadSocketException(region, instance.socket_path)
        else:
            raise RegionNotFound(region=region)


    def thread_ping(self, timer=1):
        req = request_pb2.Request()
        req.requested_api = type_pb2.METADATAS
        while not self.thread_event.is_set():
            for key, instance in self.instances.iteritems():
                try:
                    resp = self.send_and_receive(req, key, timeout=1000)
                    if resp.HasField("metadatas"):
                        metadatas = resp.metadatas
                        for contributor in metadatas.contributors:
                            self.contributors[str(contributor)] = key
                        instance.geom = wkt.loads(metadatas.shape)
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
#Il s'agit d'une coordonnée
        if object_id.count(";") == 1:
            lon, lat = object_id.split(";")
            try:
                flon = float(lon)
                flat = float(lat)
            except:
                raise RegionNotFound(object_id=object_id)
            return self.key_of_coord(flon, flat)
        else:
            try:
                contributor = object_id.split(":")[1]
            except ValueError:
                raise RegionNotFound(object_id=object_id)
            if contributor in self.contributors:
                return self.contributors[contributor]
            elif contributor in self.instances.keys():
                return contributor
        raise RegionNotFound(object_id=object_id)

    def key_of_coord(self, lon, lat):
        """ Étant donné une coordonnée, retourne à quelle clef NAViTiA cela correspond

        Retourne None si on a rien trouvé
        """
        p = geometry.Point(lon,lat)
        for key, instance in self.instances.iteritems():
            if instance.geom and instance.geom.contains(p):
                return key

        raise RegionNotFound(lon=lon, lat=lat)

    def is_region_exists(self, region_str):
        if (region_str in self.instances.keys()):
            return True
        else:
            raise RegionNotFound(region=region_str)


    def get_region(self, region_str=None, lon=None, lat=None, object_id = None):
        if region_str and self.is_region_exists(region_str):
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
            regions.append(self.get_region(region_str=region, lon=lon, lat=lat))
        else:
            regions = self.instances.keys()

        for key_region in regions :
            req = request_pb2.Request()
            req.requested_api = type_pb2.METADATAS
            resp = self.send_and_receive(req, key_region)
            resp_dict = protobuf_to_dict(resp)
            if 'metadatas' in resp_dict.keys():
                resp_dict['metadatas']['region_id'] = key_region
                response['regions'].append(resp_dict['metadatas'])
        return response
