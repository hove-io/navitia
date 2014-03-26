# encoding: utf-8
import logging
import datetime
from stat_persistor.saver.stat_Request import persist_stat_request
from stat_persistor.saver.utils import FunctionalError, TechnicalError
from stat_persistor.config import Config


class StatSaver(object):

    """
    Classe responsable de l'enregistrement en base de donnée des statistiques
    (dans un fichier pour le moments)
    """

    def __init__(self, config):
        self.file_name = config.stat_file
    def persist_stat(self, config, stat_request):
        self.__persist(stat_request, config, persist_stat_request)

    def __persist(self, item, config, callback):
        """
        fonction englobant toute la gestion d'erreur lié à la base de donnée
        et la gestion de la transaction associé

        :param item l'objet à enregistré
        :param callback fonction charger de l'enregistrement de l'objet
        à proprement parler dont la signature est (meta, conn, item)
        meta etant un objet MetaData
        conn la connection à la base de donnée
        item etant l'objet à enregistrer
        """
        logger = logging.getLogger('stat_persistor')
        conn = None
        callback(item, config)
