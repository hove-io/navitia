#!/usr/bin/env python
#coding: utf-8

from pyed import resources

from pyed.app import app, api

api.add_resource(resources.Instance, '/v0/instance/')

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0')


@app.errorhandler(Exception)
def error_handler(exception):
    """
    se charge de générer la réponse d'erreur au bon format
    """
    app.logger.exception('')
