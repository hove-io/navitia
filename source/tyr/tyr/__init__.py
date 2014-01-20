#coding: utf-8

from flask import Flask
import flask_restful
from tyr.helper import configure_logger, make_celery
from redis import Redis

app = Flask(__name__)
app.config.from_object('tyr.default_settings')
app.config.from_envvar('TYR_CONFIG_FILE')

configure_logger(app)

from navitiacommon.models import db
db.init_app(app)

api = flask_restful.Api(app, catch_all_404s=True)
celery = make_celery(app)


redis = Redis(host=app.config['REDIS_HOST'],
        port=app.config['REDIS_PORT'], db=app.config['REDIS_DB'],
        password=app.config['REDIS_PASSWORD'])

import tyr.api
