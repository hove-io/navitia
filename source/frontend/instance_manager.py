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
        self.socket_path = None
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

        self.context = zmq.Context()
        self.default_socket = None

        self.poller = zmq.Poller()

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
            self.poller.register(instance.socket, zmq.POLLIN)
            self.instances[conf.get('instance' , 'key')] = instance


        self.thread_event = Event()
        self.thread = Thread(target = self.thread_ping)
        self.thread.start()

    def send_and_receive(self, request, region = None):
        if region in self.instances:
            instance = self.instances[region]
        else:
            return None
        instance.lock.acquire()
        instance.socket.send(request.SerializeToString())#, zmq.NOBLOCK, copy=False)
        socks = dict(self.poller.poll(1000))
        if socks.get(instance.socket) == zmq.POLLIN:
            pb = instance.socket.recv()
            instance.lock.release()
            resp = type_pb2.Response()
            resp.ParseFromString(pb)
            return resp
        else :
            print region+" is a dead socket (" + instance.socket_path + ")"
            instance.socket.setsockopt(zmq.LINGER, 0)
            instance.socket.close()
            self.poller.unregister(instance.socket)
            instance.socket = self.context.socket(zmq.REQ)
            instance.socket.connect(instance.socket_path)
            self.poller.register(instance.socket)
            instance.lock.release()
            return None


    def thread_ping(self, timer=1):
        req = type_pb2.Request()
        req.requested_api = type_pb2.METADATAS
        while not self.thread_event.is_set():
            for key, instance in self.instances.iteritems():
                resp = self.send_and_receive(req, key)
                if resp:
                    try:
                        instance.geom = wkt.loads(resp.metadatas.shape)
                    except:
                        instance.geom = None
            self.thread_event.wait(timer)
        print "fin thread ping"




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

