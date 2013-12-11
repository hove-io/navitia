#!/usr/bin/env python
#coding: utf-8

from tyr import resources

from tyr.app import app, api

api.add_resource(resources.Instance, '/v0/instances/')
api.add_resource(resources.Api, '/v0/api/')

api.add_resource(resources.User, '/v0/user/',
        '/v0/user/<int:user_id>/', '/v0/user/<string:login>/')

api.add_resource(resources.Key, '/v0/user/<int:user_id>/key/',
        '/v0/user/<int:user_id>/key/<int:key_id>/')

api.add_resource(resources.Authorization,
        '/v0/user/<int:user_id>/authorization/')


if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0')


@app.errorhandler(Exception)
def error_handler(exception):
    """
    se charge de générer la réponse d'erreur au bon format
    """
    app.logger.exception('')
