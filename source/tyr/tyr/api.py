#!/usr/bin/env python
#coding: utf-8

from flask import Flask
from flask.ext import restful
from flask.ext.sqlalchemy import SQLAlchemy
from tyr import resources

from tyr.app import app, db, api

api.add_resource(resources.Instance, '/20131022/instances/')
api.add_resource(resources.Api, '/20131022/apis/')
api.add_resource(resources.User, '/20131022/users/', '/20131022/user/',
        '/20131022/user/<int:user_id>/', '/20131022/user/<string:login>/')
api.add_resource(resources.Key, '/20131022/user/<int:user_id>/keys/',
        '/20131022/user/<int:user_id>/key/',
        '/20131022/user/<int:user_id>/key/<int:key_id>/')

api.add_resource(resources.Authorization,
        '/20131022/user/<int:user_id>/authorization/',
        '/20131022/user/<int:user_id>/authorizations/')

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0')


@app.errorhandler(Exception)
def error_handler(exception):
    """
    se charge de générer la réponse d'erreur au bon format
    """
    app.logger.exception('')
