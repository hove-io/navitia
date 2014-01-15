# encoding: utf-8
from flask import Flask, got_request_exception
from flask.ext.restful import Api
from logging.handlers import RotatingFileHandler, TimedRotatingFileHandler
from jormungandr.exceptions import log_exception

app = Flask(__name__)
app.config.from_object('jormungandr.default_settings')
app.config.from_envvar('JORMUNGANDR_CONFIG_FILE')

if 'ERROR_HANDLER_TYPE' in app.config and\
        'ERROR_HANDLER_FILE' in app.config:
    handler = None
    params = {} if not 'ERROR_HANDLER_PARAMS' in app.config\
        else app.config['ERROR_HANDLER_PARAMS']
    file_ = app.config['ERROR_HANDLER_FILE']
    if app.config['ERROR_HANDLER_TYPE'] == 'rotating':
        handler = RotatingFileHandler(file_, **params)
    if app.config['ERROR_HANDLER_TYPE'] == 'timedrotating':
        handler = TimedRotatingFileHandler(file_, **params)
    elif app.config['ERROR_HANDLER_FILE'] == 'file':
        handler = FileHandler(file_, **params)
    if handler:
        if 'ERROR_HANDLER_LEVEL' in app.config:
            handler.setLevel(app.config['ERROR_HANDLER_LEVEL'])
        app.logger.addHandler(handler)

app.logger.setLevel(app.config['LOG_LEVEL'])

got_request_exception.connect(log_exception, app)

rest_api = Api(app, catch_all_404s=True)

from navitiacommon.models import db
db.init_app(app)
if not app.config['CACHE_DISABLED']:
    from navitiacommon.cache import init_cache
    init_cache(host=app.config['REDIS_HOST'], port=app.config['REDIS_PORT'],
               db=app.config['REDIS_DB'],
               password=app.config['REDIS_PASSWORD'])


from jormungandr.instance_manager import InstanceManager
i_manager = InstanceManager()
i_manager.initialisation()

from jormungandr import api


def setup_package():
    i_manager.stop()
