#coding: utf-8

from flask import Flask
import flask_restful
from tyr.helper import configure_logger, make_celery
from redis import Redis
from flask_sqlalchemy import SQLAlchemy

app = Flask(__name__)
app.config.from_object('tyr.default_settings')
#app.config.from_envvar('PYED_CONFIG_FILE')

configure_logger(app)

api = flask_restful.Api(app, catch_all_404s=True)
celery = make_celery(app)
db = SQLAlchemy(app)


redis = Redis(host=app.config['REDIS_HOST'],
        port=app.config['REDIS_PORT'], db=app.config['REDIS_DB'],
        password=app.config['REDIS_PASSWORD'])
