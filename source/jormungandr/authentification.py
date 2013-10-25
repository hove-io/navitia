#encoding: utf-8
from flask.ext.restful import reqparse
import flask.ext.restful

from flask import current_app
from functools import wraps
import db
from instance_manager import NavitiaManager, RegionNotFound
import logging


def authentification_required(func):
    """
    decorateur chargé de l'authentification des requetes
    fonctionne pour chaque API prenant un paramétre la région
    si la region est absente de la requéte la requete est automatique autorisé
    """
    @wraps(func)
    def wrapper(*args, **kwargs):
        region = None
        if kwargs.has_key('region'):
            region = kwargs['region']
        elif kwargs.has_key('lon') and kwargs.has_key('lat'):
            try:
                region = NavitiaManager().key_of_coord(lon=kwargs['lon'],
                        lat=kwargs['lat'])
            except RegionNotFound:
                pass

        if not region or authenticate(region, 'ALL', abort=True):
            return func(*args, **kwargs)

    return wrapper

def authenticate(region, api, abort=False):
    if current_app.config.has_key('PUBLIC') \
            and current_app.config['PUBLIC']:
        #si jormungandr est en mode public: on zap l'authentification
        return True


    parser = reqparse.RequestParser()
    parser.add_argument('Authorization', type=str, location='headers')
    request_args = parser.parse_args()

    if not request_args['Authorization']:
        if abort:
            flask.ext.restful.abort(401)
        else:
            return False

    auth = authentication_with_db(region, request_args['Authorization'], api)
    if auth:
        return True
    else:
        if abort:
            flask.ext.restful.abort(403)
        else:
            return False


def authentication_with_db(region, token, api):
    conn = None
    try:
        conn = db.engine.connect()
        query = db.user.join(db.key) \
                .join(db.authorization) \
                .join(db.instance) \
                .join(db.api) \
                .select() \
                .where(db.api.c.name == api) \
                .where(db.instance.c.name == region) \
                .where(db.key.c.token == token)

        res = conn.execute(query)
        if res.rowcount > 0:
            return True
        else:
            return False
    finally:
        if conn:
            conn.close()
