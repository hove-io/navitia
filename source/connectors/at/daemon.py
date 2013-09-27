#encoding: utf-8
from config import Config
from selector.atreader import AtRealtimeReader
import logging


class ConnectorAT(object):

    def __init__(self):
        self.connection = None
        self.channel = None
        self.at_realtime_reader = None
        self._init_logger()
        self.config = Config()

    def init(self, filename):
        """
        initialise le service via le fichier de conf passer en paramétre
        """
        self.config.load(filename)
#la DB doit etre prete avant d'initialiser rabbitmq, on peut recevoir des
#tache avant d'avoir lancer la boucle d'evenement
        self.at_realtime_reader = AtRealtimeReader(self.config)


    def _init_logger(self, filename='', level='debug'):
        """
        initialise le logger, par défaut level=Debug
        et affichage sur la sortie standard
        """
        level = getattr(logging, level.upper(), logging.DEBUG)
        logging.basicConfig(filename=filename, level=level)

        if level == logging.DEBUG:
            #on active les logs de sqlalchemy si on est en debug:
            #log des requetes et des resultats
            logging.getLogger('sqlalchemy.engine').setLevel(logging.DEBUG)
            logging.getLogger('sqlalchemy.pool').setLevel(logging.DEBUG)
            logging.getLogger('sqlalchemy.dialects.postgresql')\
                    .setLevel(logging.INFO)
    def run(self):
        self.at_realtime_reader.run()

