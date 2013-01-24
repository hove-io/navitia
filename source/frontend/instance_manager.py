# coding=utf-8

from shapely.wkt import loads
from shapely.geometry import Point
from shapely.prepared import prep
import ConfigParser
import csv
import zmq

class NavitiaManager:
    """ Permet de coordonner les différents NAViTiA gérés par le front-end
    Un identifiant correspond à une socket ZMQ Navitia où se connecter
    Plusieurs identifiants peuvent se connecter à une même socket

    De plus il est possible de définir la zone géographique (au format WKT) couverte par un identifiant
    """

    def __init__(self, ini_file=None, default_zmq_socket='ipc:///tmp/default_navitia'):
        """ Charge la configuration à partir d'un fichier ini indiquant les chemins des fichiers contenant :
        - géométries de la région sur laquelle s'applique le moteur
        - la socket pour chaque identifiant navitia
        
        S'il n'y a pas de fichier ini de configuration, on considère qu'il n'y a qu'un seul NAViTiA connecté à la socket par défaut
        """

        self.geom = {}
        self.keys_navitia = {}
        context = zmq.Context()
        self.default_socket = None
        
        if(ini_file):
            config = ConfigParser.ConfigParser({'geometry' : 'geometry.csv', 'key_navitia': 'key_navitia.csv'})
            config.read(ini_file)
    
            with open(config.get('instances', 'geometry'), 'rb') as csvfile:
                csvreader = csv.reader(csvfile, delimiter=';')
                for row in csvreader:
                    self.geom[row[0]] = prep(loads(row[1]))
    
            with open(config.get('instances', 'keys_navitia'), 'rb') as csvfile:
                csvreader = csv.reader(csvfile, delimiter=';')
                for row in csvreader:
                    socket = context.socket(zmq.REQ)
                    socket.connect(row[1])
                    self.keys_navitia[row[0]] = socket
        else:
            self.default_socket = context.socket(zmq.REQ)
            self.default_socket.connect(default_zmq_socket)
    

    def key_of_coord(self, lon, lat):
        """ Étant donné une coordonnée, retourne à quelle clef NAViTiA cela correspond

        Retourne None si on a rien trouvé
        """
        p = Point(lon,lat)
        for key, geom in self.iteritems():
            if geom.contains(p):
                return key
        return None

    def socket_of_key(self, key = None):
        """ Retourne la socket ZMQ vers le NAViTiA correspondant à la clef

        Si key vaut None et qu'aucune socket n'a été dans les fichiers de configuration, retourne la socket par défaut
        """
        if key in self.key_navitia:
            return self.key_navitia[key]
        else:
            return self.default_socket

