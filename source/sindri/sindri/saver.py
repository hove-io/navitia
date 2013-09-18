#encoding: utf-8
import logging
import psycopg2
import datetime

class InvalidMessage(ValueError):
    """
    Exception lancé lorsque que le message à traiter n'est pas valide
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

def build_message_dict(message):
    """
    construit à partir d'un object protobuf pbnavitia.realtime.Message le
    dictionnaire utilisé pour l'insertion en base
    """
    result = {}
    result['uri'] = message.uri
    result['object_uri'] = message.object.object_uri
    result['object_type_id'] = message.object.object_type

    if not message.active_days:
        result['active_days'] = '11111111'
    else:
        if (message.active_days.count('0')
                + message.active_days.count('1')) != 8:
            raise InvalidMessage('not valid: ' + message.active_days)
        result['active_days'] = message.active_days

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

def insert_localized_message(cursor, localized_message, message_id):
    query = ("""INSERT INTO realtime.localized_message """
    """(message_id, language, body, title)
VALUES (%(message_id)s, %(language)s, %(body)s, %(title)s);
    """)
    cursor.execute(query, {'message_id': message_id,
        'language': localized_message.language, 'body': localized_message.body,
        'title': localized_message.title})
    logging.getLogger('sindri').debug(cursor.query)


def update_message(cursor, row_id, message):
    query = """UPDATE realtime.message
    SET start_publication_date = %(start_publication_date)s,
        end_publication_date = %(end_publication_date)s,
        start_application_date = %(end_application_date)s,
        end_application_date = %(end_application_date)s,
        start_application_daily_hour = %(end_application_daily_hour)s,
        end_application_daily_hour = %(end_application_daily_hour)s,
        active_days = %(active_days)s,
        object_uri = %(object_uri)s,
        object_type_id = %(object_type_id)s,
        uri = %(uri)s
    where id = %(id)s;
    """
    m_dict = build_message_dict(message)
    m_dict['id'] = row_id
    cursor.execute(query, m_dict)
    logging.getLogger('sindri').debug(cursor.query)

    cursor.execute('DELETE from realtime.localized_message '
            'where message_id = %(id)s', {'id': row_id})
    logging.getLogger('sindri').debug(cursor.query)

    for localized_message in message.localized_messages:
        insert_localized_message(cursor, localized_message, row_id)


def insert_message(cursor, message):
    query = """INSERT INTO realtime.message
    (start_publication_date, end_publication_date, start_application_date,
    end_application_date, start_application_daily_hour,
    end_application_daily_hour, active_days, object_uri, object_type_id, uri)
VALUES
    (%(start_application_date)s, %(end_publication_date)s,
    %(start_application_date)s, %(end_application_date)s,
    %(start_application_daily_hour)s, %(end_application_daily_hour)s,
    %(active_days)s, %(object_uri)s, %(object_type_id)s, %(uri)s)
RETURNING id;
    """
    m_dict = build_message_dict(message)
    cursor.execute(query, m_dict)
    logging.getLogger('sindri').debug(cursor.query)
    row_id = cursor.fetchone()

    for localized_message in message.localized_messages:
        insert_localized_message(cursor, localized_message, row_id)



def find_message_id(cursor, message_uri):
    """
    retourne l'id en base du message correspondant à cette URI
    si celui ci est présent dans ED sinon retourne None
    """
    cursor.execute('SELECT id from realtime.message where uri = %(uri)s',
            {'uri': message_uri})
    row = cursor.fetchone()
    return row[0] if row else None


class EdRealtimeSaver(object):
    """
    Classe responsable de l'enregistrement en base de donnée des événements
    temps réel.
    """
    def __init__(self, config):
        self.connection_string = config.ed_connection_string
        self.connection = None
        self.__connect()

    def persist_message(self, message):
        """
        enregistre ou met a jour un message dans ED
        """
        try:
            self.__connect()
            cursor = self.connection.cursor()
            row_id = find_message_id(cursor, message.uri)
            if row_id:
                update_message(cursor, row_id, message)
            else:
                insert_message(cursor, message)
            self.connection.commit()
        except (psycopg2.IntegrityError, psycopg2.DataError), e:
            logging.getLogger('sindri').exception(
                    'error durring transaction')
            self.connection.rollback()
        except psycopg2.Error, e:
            logging.getLogger('sindri').exception(
                    'error durring transaction')
            self.connection.rollback()
            raise TechnicalError('problem with databases: ' + str(e))

    def __connect(self):
        """
        ouvre la connection à la base de données si besoin
        """
        if not self.connection:
            logging.getLogger('sindri').info('connection à ED')
            self.connection = psycopg2.connect(self.connection_string)


    def __del__(self):
        self.close()

    def close(self):
        if self.connection:
            self.connection.close()
