# encoding: utf-8
from flask_restful import reqparse
import flask_restful
from flask import current_app
from functools import wraps
from jormungandr import db
from jormungandr.exceptions import RegionNotFound
from jormungandr import i_manager
import datetime
import base64
from models import User, Instance


def authentification_required(func):
    """
    decorateur chargé de l'authentification des requetes
    fonctionne pour chaque API prenant un paramétre la région
    si la region est absente de la requéte la requete est automatique autorisé
    """
    @wraps(func)
    def wrapper(*args, **kwargs):
        region = None
        if 'region' in kwargs:
            region = kwargs['region']
        elif 'lon' in kwargs and 'lat' in kwargs:
            try:
                region = i_manager.key_of_coord(lon=kwargs['lon'],
                                                lat=kwargs['lat'])
            except RegionNotFound:
                pass

        if not region or authenticate(region, 'ALL', abort=True):
            return func(*args, **kwargs)

    return wrapper


def get_token():
    parser = reqparse.RequestParser()
    parser.add_argument('Authorization', type=str, location='headers')
    request_args = parser.parse_args()

    if not request_args['Authorization']:
        return None

    args = request_args['Authorization'].split(' ')
    if len(args) > 1:
        try:
            _, b64 = request_args['Authorization'].split(' ')
            decoded = base64.decodestring(b64)
            return decoded.split(':')[0]
        except ValueError:
            return None
    else:
        return args[0]


def authenticate(region, api, abort=False):
    if 'PUBLIC' in current_app.config \
            and current_app.config['PUBLIC']:
        # si jormungandr est en mode public: on zap l'authentification
        return True

    token = get_token()

    if not token:
        if abort:
            flask_restful.abort(401)
        else:
            instance = Instance.query.filter_by(name=region)
            if instance:
                return instance.is_free
            flask_restful.abort(404)

    user = User.get_from_token(token, datetime.datetime.now())

    if user:
        if user.has_access(region, api):
            return True
        else:
            if abort:
                flask_restful.abort(403)
            else:
                return False
    else:
        if abort:
            flask_restful.abort(401)
        else:
            return False
