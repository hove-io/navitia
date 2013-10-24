from sqlalchemy import Table, MetaData, select, create_engine
from app import app

__ALL__ = ['engine', 'meta', 'key', 'user', 'instance', 'api', 'authorization']

engine = create_engine(app.config['PG_CONNECTION_STRING'])
meta = MetaData(engine)
key = Table('key', meta, autoload=True, schema='jormungandr')
user = Table('user', meta, autoload=True, schema='jormungandr')
instance = Table('instance', meta, autoload=True, schema='jormungandr')
api = Table('api', meta, autoload=True, schema='jormungandr')
authorization = Table('authorization', meta, autoload=True,
        schema='jormungandr')

