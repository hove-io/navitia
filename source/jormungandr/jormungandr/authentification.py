# encoding: utf-8

#  Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Canal TP (www.canaltp.fr).
# Help us simplify mobility and open public transport:
#     a non ending quest to the responsive locomotion way of traveling!
#
# LICENCE: This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Stay tuned using
# twitter @navitia
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from flask_restful import reqparse
import flask_restful
from flask import current_app, request, g
from functools import wraps
from jormungandr.exceptions import RegionNotFound
from jormungandr import i_manager
import datetime, time
import base64
from navitiacommon.models import User, Instance, db
#from jormungandr.stat_manager import StatManager


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
    """
    find the Token in the "Authorization" HTTP header
    two cases are handle:
        - the token is the only value in the header
        - Basic Authentication is used and the token is in the username part
          In this case the Value of the header look like this:
          "BASIC 54651a4ae4rae"
          The second part is the username and the password separate by a ":"
          and encoded in base64
    """
    if 'Authorization' not in request.headers:
        return None

    args = request.headers['Authorization'].split(' ')
    if len(args) == 2:
        try:
            b64 = args[1]
            decoded = base64.decodestring(b64)
            return decoded.split(':')[0]
        except ValueError:
            return None
    else:
        return request.headers['Authorization']


def authenticate(region, api, abort=False):
    """
    Check the Authorization of the current user for this region and this API.
    If abort is True, the request is aborted with the appropriate HTTP code.
    """
    if 'PUBLIC' in current_app.config \
            and current_app.config['PUBLIC']:
        #if jormungandr is on public mode we skip the authentification process
        return True

    #Hack to allow user not logged in...
    token = get_token()
    if not token:
        instance = Instance.get_by_name(region)
        if abort:
            if instance and instance.is_free:
                return True
            else:
                abort_request()
        else:
            return False if not instance else instance.first().is_free

    user = get_user()
    if user:
        #STAT-1 Ajouter flask variable (global) pour pouvoir garder les valeurs
        if user.has_access(region, api):
            return True
        else:
            if abort:
                abort_request()
            else:
                return False
    else:
        if abort:
           abort_request()
        else:
            return False

def abort_request():
    """
    abort a request with the proper http status in case of authentification issues
    """
    if get_user():
        flask_restful.abort(403)
    else:
        flask_restful.abort(401)

def has_access(instance, abort=False):
    res = instance.is_accessible_by(get_user())
    if abort and not res:
        abort_request()
    else:
        return res

def get_user():
    """
    return the current authentificated User or None
    """
    if hasattr(g, 'user'):
        return g.user
    else:
        g.user = User.get_from_token(get_token(), datetime.datetime.now())
        return g.user
