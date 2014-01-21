#!/usr/bin/env python
#coding: utf-8

from tyr import resources

from tyr import app, api

api.add_resource(resources.Instance, '/v0/instances/')
api.add_resource(resources.Api, '/v0/api/')

api.add_resource(resources.User, '/v0/users/',
        '/v0/users/<int:user_id>/')

api.add_resource(resources.Key, '/v0/users/<int:user_id>/keys/',
        '/v0/users/<int:user_id>/keys/<int:key_id>/')

api.add_resource(resources.Authorization,
        '/v0/users/<int:user_id>/authorizations/')


@app.errorhandler(Exception)
def error_handler(exception):
    """
    se charge de générer la réponse d'erreur au bon format
    """
    app.logger.exception('')
