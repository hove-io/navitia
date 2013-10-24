#encoding: utf-8
from flask import Flask
from flask.ext.restful import Api

app = Flask(__name__)
app.config.from_object('default_settings')
app.config.from_envvar('JORMUNGANDR_CONFIG_FILE')
app.debug = True
app.config.update(PROPAGATE_EXCEPTIONS=True)
api = Api(app, catch_all_404s=True)
