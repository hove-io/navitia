#coding: utf-8

from flask import Flask
from flask.ext import restful
from flask.ext.sqlalchemy import SQLAlchemy

app = Flask(__name__)
api = restful.Api(app, catch_all_404s=True)
app.config['SQLALCHEMY_DATABASE_URI'] = 'postgresql://navitia:navitia@localhost/jormun'
db = SQLAlchemy(app)
