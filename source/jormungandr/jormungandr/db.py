from jormungandr import app
from jormungandr.cache import Cache
from flask_sqlalchemy import SQLAlchemy


__ALL__ = ['db', 'cache']


app.config['SQLALCHEMY_DATABASE_URI'] = app.config['PG_CONNECTION_STRING']

db = SQLAlchemy(app)

cache = Cache(host=app.config['REDIS_HOST'],
              port=app.config['REDIS_PORT'],
              db=app.config['REDIS_DB'],
              password=app.config['REDIS_PASSWORD'],
              disabled=app.config['CACHE_DISABLED'])
