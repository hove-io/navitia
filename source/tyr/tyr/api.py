#!/usr/bin/env python
#coding: utf-8

from flask import Flask
from flask.ext import restful
from tyr import resources

app = Flask(__name__)
api = restful.Api(app)


api.add_resource(resources.Instance, '/')
api.add_resource(resources.User, '/user')

if __name__ == '__main__':
    app.run(debug=True)
