#encoding: utf-8
import logging
import datetime
from sqlalchemy import Table, MetaData, select, create_engine, insert, update
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


class EdRealtimeSaver(object):
    """
    Classe responsable de l'enregistrement en base de donnée des événements
    temps réel.
    """
    def __init__(self, config):
        self.connection_string = config.ed_connection_string
        self.engine = create_engine(self.connection_string)
        meta = MetaData(self.engine)
        self.message_table = Table('message', meta, autoload=True,
                schema='realtime')
        self.localized_message_table = Table('localized_message', meta,
                autoload=True, schema='realtime')


    def persist_message(self, message):
        """
        enregistre ou met a jour un message dans ED
        """
        conn = None
        logging.getLogger('sindri').info('enter')
        try:
            conn = self.engine.connect()
            transaction = conn.begin()
        except sqlalchemy.exc.SQLAlchemyError, e:
            logging.getLogger('sindri').exception(
                    'error durring transaction')
            raise TechnicalError('problem with databases: ' + str(e))


        try:
            row_id = self.find_message_id(conn, message.uri)
            self.save_message(conn, row_id, message)
            transaction.commit()
        except (sqlalchemy.exc.IntegrityError, sqlalchemy.exc.DataError), e:
            logging.getLogger('sindri').exception(
                    'error durring transaction')
            transaction.rollback()
            raise FunctionalError(str(e))
        except sqlalchemy.exc.SQLAlchemyError, e:
            logging.getLogger('sindri').exception(
                    'error durring transaction')
            transaction.rollback()
            raise TechnicalError('problem with databases: ' + str(e))
        except:
            transaction.rollback()
            raise
        finally:
            if conn:
                conn.close()

    def find_message_id(self, conn, message_uri):
        """
        retourne l'id en base du message correspondant à cette URI
        si celui ci est présent dans ED sinon retourne None
        """
        query = select([self.message_table.c.id],
                self.message_table.c.uri == message_uri)
        result = conn.execute(query)
        row = result.fetchone()
        return row[0] if row else None

    def save_message(self, conn, message_id, message):
        """
        retourne l'id en base du message correspondant à cette URI
        si celui ci est présent dans ED sinon retourne None
        """
        if not message_id:
            query = self.message_table.insert()
        else:
            query = self.message_table.update().where(
                    self.message_table.c.id == message_id)

        result = conn.execute(query.values(build_message_dict(message)))
        if result.is_insert:
            message_id = result.inserted_primary_key[0]
        else:
            conn.execute(self.localized_message_table.delete().where(
                self.localized_message_table.c.message_id == message_id))
        #on insére les messages text
        query = self.localized_message_table.insert()
        for localized_message in message.localized_messages:
            conn.execute(query.values(message_id=message_id,
                language=localized_message.language,
                body=localized_message.body, title=localized_message.title))

