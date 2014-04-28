# encoding: utf-8
import logging
import datetime
from sqlalchemy import Column, Table, MetaData, select, create_engine, \
    ForeignKey, bindparam, and_, or_, exc
import sqlalchemy
from stat_persistor.saver.stat_Request import persist_stat_request
from stat_persistor.saver.utils import FunctionalError, TechnicalError
from stat_persistor.config import Config


class StatSaver(object):
    """
    Class to consume stat elements from rabbitMQ and to save them
    in a database PostGRE using SQLAlChemy.
    """

    def __init__(self, config):
        self.file_name = config.stat_file

        self.__engine = create_engine(config.stat_connection_string)
        self.meta = MetaData(self.__engine)
        self.request_table = Table('requests', self.meta, autoload=True,
                                   schema='stat')

        self.coverage_table = Table('coverages', self.meta,
                                    autoload=True,
                                    schema='stat')

        self.parameter_table = Table('parameters', self.meta,
                                     autoload=True,
                                     schema='stat')

        self.error_table = Table('errors', self.meta,
                                     autoload=True,
                                     schema='stat')

        self.journey_table = Table('journeys', self.meta,
                                   autoload=True,
                                   schema='stat')

        self.journey_section_table = Table('journey_sections', self.meta,
                                           autoload=True,
                                           schema='stat')

    def persist_stat(self, config, stat_request):
        self.__persist(config, stat_request, persist_stat_request)

    def __persist(self, config, item, callback):
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
        try:
            conn = self.__engine.connect()
            transaction = conn.begin()
        except sqlalchemy.exc.SQLAlchemyError as e:
            logger.exception('error during transaction')
            raise TechnicalError('problem with databases: ' + str(e))

        try:
            callback(self.meta, conn, item)
            #callback(item, config)
            transaction.commit()
        except (sqlalchemy.exc.IntegrityError, sqlalchemy.exc.DataError) as e:
            logger.exception('error during transaction')
            transaction.rollback()
            raise FunctionalError(str(e))
        except sqlalchemy.exc.SQLAlchemyError as e:
            logger.exception('error during transaction')
            if not hasattr(e, 'connection_invalidated') \
                    or not e.connection_invalidated:
                transaction.rollback()
            raise TechnicalError('problem with databases: ' + str(e))
        except:
            logger.exception('error during transaction')
            try:
                transaction.rollback()
            except:
                pass
            raise TechnicalError('problem the rollback')
        finally:
            if conn:
                conn.close()

