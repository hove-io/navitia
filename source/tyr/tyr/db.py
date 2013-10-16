#encoding: utf-8

from sqlalchemy import create_engine, MetaData, Table

_DB_INSTANCES = {}


def get_db(connection_string):
    """
    factory pour les objets DB
    """
    if connection_string not in _DB_INSTANCES:
        _DB_INSTANCES[connection_string] = Db(connection_string)
    return _DB_INSTANCES[connection_string]


class Db(object):
    def __init__(self, connection_string):
        self.engine = create_engine(connection_string, convert_unicode=True)
        self.metadata = MetaData(bind=self.engine)

        self.instance = Table('instance', self.metadata, autoload=True,
                schema='jormungandr')
        self.user = Table('user', self.metadata, autoload=True,
                schema='jormungandr')
