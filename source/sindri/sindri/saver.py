#encoding: utf-8
import logging
import datetime
from sqlalchemy import Table, MetaData, select, create_engine
import sqlalchemy
from sindri.saver.message import persist_message

class FunctionalError(ValueError):
    """
    Exception lancé lorsque que la donnée à traiter n'est pas valide
    """
    pass

class TechnicalError(ValueError):
    """
    Exception lancé lors d'un probléme technique
    typiquement la base de données est inaccessible
    """
    pass



class EdRealtimeSaver(object):
    """
    Classe responsable de l'enregistrement en base de donnée des événements
    temps réel.
    """
    def __init__(self, config):
        self.__engine = create_engine(config.ed_connection_string)
        self.meta = MetaData(self.__engine)
        self.message_table = Table('message', self.meta, autoload=True,
                schema='realtime')
        self.localized_message_table = Table('localized_message', self.meta,
                autoload=True, schema='realtime')

    #def test(self, f):
    #    def wrapper(*args):
    #        print 'before'
    #        return f(*args)
    #    return wrapper

    #@self.test
    def persist_message(self, message):
        self.__persist(message, persist_message)


    def persist_at_perturbation(self, perturbation):
        print 'ok'
        #self.__persist(, persist_message)

    def __persist(self, item, callback):
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
        logger = logging.getLogger('sindri')
        conn = None
        try:
            conn = self.__engine.connect()
            transaction = conn.begin()
        except sqlalchemy.exc.SQLAlchemyError, e:
            logger.exception('error durring transaction')
            raise TechnicalError('problem with databases: ' + str(e))

        try:
            callback(self.meta, conn, item)
            transaction.commit()
        except (sqlalchemy.exc.IntegrityError, sqlalchemy.exc.DataError), e:
            logger.exception('error durring transaction')
            transaction.rollback()
            raise FunctionalError(str(e))
        except sqlalchemy.exc.SQLAlchemyError, e:
            logger.exception('error durring transaction')
            if e.connection_invalidated:
                transaction.rollback()
            raise TechnicalError('problem with databases: ' + str(e))
        except:
            logger.exception('error durring transaction')
            if e.connection_invalidated:
                transaction.rollback()
            raise
        finally:
            if conn:
                conn.close()


