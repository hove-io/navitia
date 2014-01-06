from jormungandr import app
from jormungandr.cache import Cache
from flask_sqlalchemy import SQLAlchemy
from sqlalchemy.schema import CreateSchema
from sqlalchemy.sql import exists, select


__ALL__ = ['db', 'cache']


app.config['SQLALCHEMY_DATABASE_URI'] = app.config['PG_CONNECTION_STRING']

db = SQLAlchemy(app)

cache = Cache(host=app.config['REDIS_HOST'],
              port=app.config['REDIS_PORT'],
              db=app.config['REDIS_DB'],
              password=app.config['REDIS_PASSWORD'],
              disabled=app.config['CACHE_DISABLED'])


def syncdb():
    from jormungandr import models
    q = exists(select([("schema_name")])
               .select_from("information_schema.schemata")
               .where("schema_name = 'jormungandr'"))
    if not db.session.query(q).scalar():
        db.engine.execute(CreateSchema('jormungandr'))
    db.create_all()
    all_name = "ALL"
    if not models.Api.query.filter_by(name=all_name).count():
        api_all = models.Api(all_name)
        db.session.add(api_all)
    db.session.commit()
