#encoding: utf-8
import logging
import datetime
from sqlalchemy import Table, MetaData, select, create_engine
import sqlalchemy

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

def from_timestamp(timestamp):
    #@TODO: pour le moment on remet à l'heure local
    #à virer le jour ou kraken géreras tout en UTC
    return datetime.datetime.fromtimestamp(timestamp)

def from_time(time):
    return datetime.datetime.utcfromtimestamp(time).time()


def parse_active_days(active_days):
    """
    permet de parser les champs active_days
    retourne la valeur par défaut si non initialisé: "11111111"
    sinon vérifie que c'est bien une chaine de 8 0 ou 1 et la retourne
    raise une FunctionalError si le format n'est pas valide
    """
    if not active_days:
        return '11111111'
    else:
        if (active_days.count('0') + active_days.count('1')) != 8:
            raise FunctionalError('active_days not valid: ' + active_days)
        return  active_days


def build_message_dict(message):
    """
    construit à partir d'un object protobuf pbnavitia.realtime.Message le
    dictionnaire utilisé pour l'insertion en base
    """
    result = {}
    result['uri'] = message.uri
    result['object_uri'] = message.object.object_uri
    result['object_type_id'] = message.object.object_type

    result['active_days'] = parse_active_days(message.active_days)

    result['start_publication_date'] = from_timestamp(
            message.start_publication_date)
    result['end_publication_date'] = from_timestamp(
            message.end_publication_date)

    result['start_application_date'] = from_timestamp(
            message.start_application_date)
    result['end_application_date'] = from_timestamp(
            message.end_application_date)

    result['start_application_daily_hour'] = from_time(
            message.start_application_daily_hour)
    result['end_application_daily_hour'] = from_time(
            message.end_application_daily_hour)


    return result

def build_localized_message_dict(localized_msg, message_id):
    """
    construit à partir d'un object protobuf pbnavitia.realtime.LocalizeMessage
    le dictionnaire utilisé pour l'insertion en base
    """
    result = {}
    result['message_id'] = message_id
    result['language'] = localized_msg.language
    result['body'] = localized_msg.body if localized_msg.body else None
    result['title'] = localized_msg.title if localized_msg.title else None
    return result


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


    def persist_message(self, message):
        self.__persist(message, persist_message)

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

def persist_message(meta, conn, message):
    """
    enregistre en base le message
    """
    row_id = find_message_id(meta, conn, message.uri)
    save_message(meta, conn, row_id, message)


def find_message_id(meta, conn, message_uri):
    """
    retourne l'id en base du message correspondant à cette URI
    si celui ci est présent dans ED sinon retourne None
    """
    msg_table = meta.tables['realtime.message']
    query = select([msg_table.c.id],
            msg_table.c.uri == message_uri)
    result = conn.execute(query)
    row = result.fetchone()
    return row[0] if row else None

def save_message(meta, conn, message_id, message):
    """
    retourne l'id en base du message correspondant à cette URI
    si celui ci est présent dans ED sinon retourne None
    """
    msg_table = meta.tables['realtime.message']
    local_msg_table = meta.tables['realtime.localized_message']
    if not message_id:
        query = msg_table.insert()
    else:
        query = msg_table.update().where(msg_table.c.id == message_id)

    result = conn.execute(query.values(build_message_dict(message)))
    if result.is_insert:
        message_id = result.inserted_primary_key[0]
    else:
#si c'est un update on supprime les messages text associés avant de les recréer
        conn.execute(local_msg_table.delete().where(
            local_msg_table.c.message_id == message_id))

    #on insére les messages text
    query = local_msg_table.insert()
    for localized_message in message.localized_messages:
        conn.execute(query.values(
            build_localized_message_dict(localized_message, message_id)))

