#coding: utf-8

from flask import Flask
import flask_restful
from pyed.helper import configure_logger, make_celery, load_instances
from redis import Redis

app = Flask(__name__)
app.config.from_object('pyed.default_settings')
#app.config.from_envvar('PYED_CONFIG_FILE')

configure_logger(app)
load_instances(app)

api = flask_restful.Api(app, catch_all_404s=True)
celery = make_celery(app)


redis = Redis(host=app.config['REDIS_HOST'],
        port=app.config['REDIS_PORT'], db=app.config['REDIS_DB'],
        password=app.config['REDIS_PASSWORD'])
