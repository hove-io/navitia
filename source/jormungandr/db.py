from sqlalchemy import Table, MetaData, select, create_engine, join
from app import app
from sqlalchemy.sql import and_, or_, not_

__ALL__ = ['engine', 'meta', 'key', 'user', 'instance', 'api', 'authorization' \
        'get_user']

engine = create_engine(app.config['PG_CONNECTION_STRING'])
meta = MetaData(engine)
_key = Table('key', meta, autoload=True, schema='jormungandr')
_user = Table('user', meta, autoload=True, schema='jormungandr')
_instance = Table('instance', meta, autoload=True, schema='jormungandr')
_api = Table('api', meta, autoload=True, schema='jormungandr')
_authorization = Table('authorization', meta, autoload=True,
        schema='jormungandr')

class User(object):
    def __init__(self):
        self.id = None
        self.login = None
        self.email = None

    def has_access(self, instance_name, api_name):
        conn = None
        try:
            conn = engine.connect()

            query = select([_instance], from_obj=[join(_authorization, _api)]) \
                .where(_instance.c.name == instance_name) \
                .where(_api.c.name == api_name) \
                .where(_authorization.c.user_id == self.id)\
                .union(select([_instance]) \
                        .where(_instance.c.name == instance_name) \
                        .where(_instance.c.is_free == True))

            res = conn.execute(query)
            if res.rowcount > 0:
                return True
            else:
                return False
        finally:
            if conn:
                conn.close()



def get_user(token, valid_until):
    conn = None
    try:
        conn = engine.connect()
        query = _user.join(_key) \
                .select(use_labels=True) \
                .where(_key.c.token == token)\
                .where((_key.c.valid_until > valid_until) | (_key.c.valid_until == None))

        res = conn.execute(query)
        if res.rowcount > 0:
            row = res.fetchone()
            u = User()
            u.id = row['jormungandr_user_id']
            u.login = row['jormungandr_user_login']
            u.email = row['jormungandr_user_email']
            return u
        else:
            return None
    finally:
        if conn:
            conn.close()
