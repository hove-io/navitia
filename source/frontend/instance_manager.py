# coding=utf-8

from shapely import wkt
from shapely.geometry import Point
import ConfigParser
import zmq
from threading import Lock, Thread, Event
import type_pb2
import glob

class Instance:
    def __init__(self):
        self.geom = None
        self.socket = None
        self.lock = Lock()
        

class NavitiaManager:
    """ Permet de coordonner les différents NAViTiA gérés par le front-end
    Un identifiant correspond à une socket ZMQ Navitia où se connecter
    Plusieurs identifiants peuvent se connecter à une même socket

    De plus il est possible de définir la zone géographique (au format WKT) couverte par un identifiant
    """

    def __init__(self, ini_file=None):
        """ Charge la configuration à partir d'un fichier ini indiquant les chemins des fichiers contenant :
        - géométries de la région sur laquelle s'applique le moteur
        - la socket pour chaque identifiant navitia
        """
        
        self.instances = {}
        
        context = zmq.Context()
        self.default_socket = None

        ini_files = []
        if ini_file == None :
            ini_files = glob.glob("/etc/jormungandr.d/*.ini")
        else:
            ini_files = [ini_file]

        for file_name in ini_files:
            conf = ConfigParser.ConfigParser()
            conf.read(file_name)
            socket = context.socket(zmq.REQ)
            socket.connect(conf.get('instance' , 'socket'))
            self.instances[conf.get('instance' , 'key')] = Instance()
            self.instances[conf.get('instance' , 'key')].socket = socket

        self.thread_event = Event()
        self.thread = Thread(target = self.thread_ping)
        self.thread.start()

    def send_and_receive(self, request, region = None):
        if region in self.instances:
            instance = self.instances[region]
        else:
            return None
        instance.lock.acquire()
        instance.socket.send(request.SerializeToString())
        pb = instance.socket.recv()
        instance.lock.release()
        resp = type_pb2.Response()
        resp.ParseFromString(pb)
        return resp


    def thread_ping(self, timer=10):
        req = type_pb2.Request()
        req.requested_api = type_pb2.METADATAS
        while not self.thread_event.is_set():
            for key, instance in self.instances.iteritems():
                resp = self.send_and_receive(req, key)
                instance.shape = wkt.loads(resp.metadatas.shape)
            self.thread_event.wait(timer)




    def stop(self) : 
        self.thread_event.set()



    

    def key_of_coord(self, lon, lat):
        """ Étant donné une coordonnée, retourne à quelle clef NAViTiA cela correspond

        Retourne None si on a rien trouvé
        """
        p = Point(lon,lat)
        for key, instance in self.instances.iteritems():
            if instance.geom and instance.geom.contains(p):
                return key
        return None

