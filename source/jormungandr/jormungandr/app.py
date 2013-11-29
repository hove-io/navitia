#encoding: utf-8
from flask import Flask, got_request_exception
from flask.ext.restful import Api
from logging.handlers import RotatingFileHandler, TimedRotatingFileHandler
from exceptions import log_exception
app = Flask(__name__)
app.config.from_object('jormungandr.default_settings')
app.config.from_envvar('JORMUNGANDR_CONFIG_FILE')

if app.config.has_key('ERROR_HANDLER_TYPE') and\
        app.config.has_key('ERROR_HANDLER_FILE'):
    handler = None
    params = {} if not 'ERROR_HANDLER_PARAMS' in app.config\
                else app.config['ERROR_HANDLER_PARAMS']
    file_ = app.config['ERROR_HANDLER_FILE']
    if app.config['ERROR_HANDLER_TYPE'] == 'rotating':
        handler = RotatingFileHandler(file_ , **params)
    if app.config['ERROR_HANDLER_TYPE'] == 'timedrotating':
        handler = TimedRotatingFileHandler(file_, **params)
    elif app.config['ERROR_HANDLER_FILE'] == 'file':
        handler = FileHandler(file_, **params)
    if handler:
        if app.config.has_key('ERROR_HANDLER_LEVEL'):
            handler.setLevel(app.config['ERROR_HANDLER_LEVEL'])
        app.logger.addHandler(handler)

app.logger.setLevel(app.config['LOG_LEVEL'])

got_request_exception.connect(log_exception, app)

api = Api(app, catch_all_404s=True)
