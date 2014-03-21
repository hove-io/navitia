# encoding: utf-8
from flask_restful import reqparse, abort
import flask_restful
from flask import current_app, request
from functools import wraps
from jormungandr.exceptions import RegionNotFound
from jormungandr import i_manager
import datetime
import base64
from navitiacommon.models import User, Instance, db


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
        elif 'from' in request.args:
            #used for journeys api
            try:
                region = i_manager.key_of_id(request.args['from'])
                if 'to' in request.args:
                    region_to = i_manager.key_of_id(request.args['to'])
                    if region != region_to:
                        abort(503, message="Unable to compute journeys "
                              "between to different instances (%s, %s) " %
                              (region, region_to))
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
        instance = Instance.query.filter_by(name=region)
        if abort:
            if instance.first() and instance.first().is_free:
                return True
            else:
                flask_restful.abort(401)
        else:
            return False if not instance else instance.first().is_free

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
