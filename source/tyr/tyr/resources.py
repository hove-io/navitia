#coding: utf-8

# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
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

from flask import abort, current_app, url_for, request
import flask_restful
from flask_restful import fields, marshal_with, marshal, reqparse, types
import sqlalchemy
from validate_email import validate_email
from datetime import datetime

import logging

from navitiacommon import models
from navitiacommon.models import db

__ALL__ = ['Api', 'Instance', 'User', 'Key']


class FieldDate(fields.Raw):
    def format(self, value):
        if value:
            return value.isoformat()
        else:
            return 'null'

end_point_fields = {'id': fields.Raw,
                    'name': fields.Raw,
                    'default': fields.Raw,
                    'hostnames': fields.List(fields.String),
                   }

key_fields = {'id': fields.Raw, 'app_name': fields.Raw, 'token': fields.Raw, 'valid_until': FieldDate}

instance_fields = {'id': fields.Raw,
                   'name': fields.Raw,
                   'is_free': fields.Raw,
                   'scenario': fields.Raw,
                   'journey_order': fields.Raw,
                   'max_walking_duration_to_pt': fields.Raw,
                   'max_bike_duration_to_pt': fields.Raw,
                   'max_bss_duration_to_pt': fields.Raw,
                   'max_car_duration_to_pt': fields.Raw,
                   'max_nb_transfers': fields.Raw,
                   'walking_speed': fields.Raw,
                   'bike_speed': fields.Raw,
                   'bss_speed': fields.Raw,
                   'car_speed': fields.Raw,
                   'min_bike': fields.Raw,
                   'min_bss': fields.Raw,
                   'min_car': fields.Raw,
                   'min_tc_with_bike': fields.Raw,
                   'min_tc_with_bss': fields.Raw,
                   'min_tc_with_car': fields.Raw,
                   'max_duration_criteria': fields.Raw,
                   'max_duration_fallback_mode': fields.Raw,
}

api_fields = {'id': fields.Raw, 'name': fields.Raw}

user_fields = {'id': fields.Raw,
               'login': fields.Raw,
               'email': fields.Raw,
               'type': fields.Raw(),
                'end_point': fields.Nested(end_point_fields),
            }
user_fields_full = {'id': fields.Raw,
                    'login': fields.Raw,
                    'email': fields.Raw,
                    'type': fields.Raw(),
                    'keys': fields.List(fields.Nested(key_fields)),
                    'authorizations': fields.List(fields.Nested(
                        {'instance': fields.Nested(instance_fields),
                         'api': fields.Nested(api_fields)})),
                    'end_point': fields.Nested(end_point_fields),
                }

jobs_fields = {'jobs': fields.List(fields.Nested({
        'id': fields.Raw,
        'state': fields.Raw,
        'created_at': FieldDate,
        'updated_at': FieldDate,
        'data_sets': fields.List(fields.Nested({
            'type': fields.Raw,
            'name': fields.Raw
        })),
        'instance': fields.Nested(instance_fields)
}))}


class Api(flask_restful.Resource):
    def __init__(self):
        pass

    def get(self):
        return marshal(models.Api.query.all(), api_fields)


class Index(flask_restful.Resource):
    def get(self):
        return {'jobs': {'href': url_for('jobs', _external=True)}}

class Job(flask_restful.Resource):
    @marshal_with(jobs_fields)
    def get(self, instance_name=None):
        query = models.Job.query
        if instance_name:
            query = query.join(models.Instance)
            query = query.filter(models.Instance.name == instance_name)
        return {'jobs': query.order_by(models.Job.created_at.desc()).limit(30)}

class Instance(flask_restful.Resource):
    def __init__(self):
        pass

    @marshal_with(instance_fields)
    def get(self, id=None, name=None):
        parser = reqparse.RequestParser()
        parser.add_argument('is_free', type=types.boolean, required=False,
                case_sensitive=False, help='boolean for returning only free or private instances')
        args = parser.parse_args()
        if args['is_free'] != None:
            return models.Instance.query.filter_by(**args).all()
        else:
            return models.Instance.query.all()

    def put(self, id=None, name=None):
        if id:
            instance = models.Instance.query.get_or_404(id)
        elif name:
            instance = models.Instance.query.filter_by(name=name).first_or_404()
        else:
            return ({'error': 'instance is required'}, 400)


        parser = reqparse.RequestParser()
        parser.add_argument('scenario', type=str,  case_sensitive=False,
                help='the name of the scenario used by jormungandr', choices=['default',
                                                                              'keolis',
                                                                              'destineo',
                                                                              'new_default'],
                location=('json', 'values'), default=instance.scenario)
        parser.add_argument('journey_order', type=str, case_sensitive=False,
                help='the sort order of the journeys in jormungandr', choices=['arrival_time', 'departure_time'],
                location=('json', 'values'), default=instance.journey_order)
        parser.add_argument('max_walking_duration_to_pt', type=int,
                help='the maximum duration of walking in fallback section', location=('json', 'values'),
                default=instance.max_walking_duration_to_pt)
        parser.add_argument('max_bike_duration_to_pt', type=int,
                help='the maximum duration of bike in fallback section', location=('json', 'values'),
                default=instance.max_bike_duration_to_pt)
        parser.add_argument('max_bss_duration_to_pt', type=int,
                help='the maximum duration of bss in fallback section', location=('json', 'values'),
                default=instance.max_bss_duration_to_pt)
        parser.add_argument('max_car_duration_to_pt', type=int,
                help='the maximum duration of car in fallback section', location=('json', 'values'),
                default=instance.max_car_duration_to_pt)
        parser.add_argument('max_nb_transfers', type=int,
                help='the maximum number of transfers in a journey', location=('json', 'values'),
                default=instance.max_nb_transfers)
        parser.add_argument('walking_speed', type=float,
                help='the walking speed', location=('json', 'values'), default=instance.walking_speed)
        parser.add_argument('bike_speed', type=float,
                help='the biking speed', location=('json', 'values'), default=instance.bike_speed)
        parser.add_argument('bss_speed', type=float,
                help='the speed of bss', location=('json', 'values'), default=instance.bss_speed)
        parser.add_argument('car_speed', type=float,
                help='the speed of car', location=('json', 'values'), default=instance.car_speed)
        parser.add_argument('min_tc_with_car', type=int,
                help='minimum duration of tc when a car fallback is used', location=('json', 'values'),
                default=instance.min_tc_with_car)
        parser.add_argument('min_tc_with_bike', type=int,
                help='minimum duration of tc when a bike fallback is used', location=('json', 'values'),
                default=instance.min_tc_with_bike)
        parser.add_argument('min_tc_with_bss', type=int,
                help='minimum duration of tc when a bss fallback is used', location=('json', 'values'),
                default=instance.min_tc_with_bss)
        parser.add_argument('min_bike', type=int,
                help='minimum duration of bike fallback', location=('json', 'values'),
                default=instance.min_bike)
        parser.add_argument('min_bss', type=int,
                help='minimum duration of bss fallback', location=('json', 'values'),
                default=instance.min_bss)
        parser.add_argument('min_car', type=int,
                help='minimum duration of car fallback', location=('json', 'values'),
                default=instance.min_car)
        parser.add_argument('factor_too_long_journey', type=float,
                help='if a journey is X time longer than the earliest one we remove it', location=('json', 'values'),
                default=instance.factor_too_long_journey)
        parser.add_argument('min_duration_too_long_journey', type=int,
                help='all journeys with a duration fewer than this value will be kept no matter what even if they ' \
                        'are 20 times slower than the earliest one', location=('json', 'values'),
                default=instance.min_duration_too_long_journey)
        parser.add_argument('max_duration_criteria', type=str, choices=['time', 'duration'],
                help='', location=('json', 'values'),
                default=instance.max_duration_criteria)

        parser.add_argument('max_duration_fallback_mode', type=str, choices=['walking', 'bss', 'bike', 'car'],
                help='', location=('json', 'values'),
                default=instance.max_duration_fallback_mode)

        args = parser.parse_args()

        try:
            instance.scenario = args['scenario']
            instance.journey_order = args['journey_order']
            instance.max_walking_duration_to_pt = args['max_walking_duration_to_pt']
            instance.max_bike_duration_to_pt = args['max_bike_duration_to_pt']
            instance.max_bss_duration_to_pt = args['max_bss_duration_to_pt']
            instance.max_car_duration_to_pt = args['max_car_duration_to_pt']
            instance.max_nb_transfers = args['max_nb_transfers']
            instance.walking_speed = args['walking_speed']
            instance.bike_speed = args['bike_speed']
            instance.bss_speed = args['bss_speed']
            instance.car_speed = args['car_speed']
            instance.min_tc_with_car = args['min_tc_with_car']
            instance.min_tc_with_bike = args['min_tc_with_bike']
            instance.min_tc_with_bss = args['min_tc_with_bss']
            instance.min_bike = args['min_bike']
            instance.min_bss = args['min_bss']
            instance.min_car = args['min_car']
            instance.min_duration_too_long_journey = args['min_duration_too_long_journey']
            instance.factor_too_long_journey = args['factor_too_long_journey']
            instance.max_duration_criteria = args['max_duration_criteria']
            instance.max_duration_fallback_mode = args['max_duration_fallback_mode']
            db.session.commit()
        except Exception:
            logging.exception("fail")
            raise
        return marshal(instance, instance_fields)



class User(flask_restful.Resource):
    def __init__(self):
        pass

    def get(self, user_id=None):
        if user_id:
            user = models.User.query.get_or_404(user_id)
            return marshal(user, user_fields_full)
        else:
            parser = reqparse.RequestParser()
            parser.add_argument('login', type=unicode, required=False,
                    case_sensitive=False, help='login')
            parser.add_argument('email', type=unicode, required=False,
                    case_sensitive=False, help='email')
            parser.add_argument('key', type=unicode, required=False,
                    case_sensitive=False, help='key')
            parser.add_argument('end_point_id', type=int)
            args = parser.parse_args()

            if args['key']:
                logging.debug(args['key'])
                users = models.User.get_from_token(args['key'], datetime.now())
                return marshal(users, user_fields)
            else:
                # dict comprehension would be better, but it's not in python 2.6
                filter_params = dict((k, v) for k, v in args.items() if v)

                if filter_params:
                    users = models.User.query.filter_by(**filter_params).all()
                    return marshal(users, user_fields)
                else:
                    return marshal(models.User.query.all(), user_fields)

    def post(self):
        user = None
        parser = reqparse.RequestParser()
        parser.add_argument('login', type=unicode, required=True,
                case_sensitive=False, help='login is required', location=('json', 'values'))
        parser.add_argument('email', type=unicode, required=True,
                case_sensitive=False, help='email is required', location=('json', 'values'))
        parser.add_argument('end_point_id', type=int, required=False,
                            help='id of the end_point', location=('json', 'values'))
        parser.add_argument('type', type=str, required=False, default='with_free_instances',
                            help='type of user: [with_free_instances, without_free_instances, super_user]',
                            location=('json', 'values'),
                            choices=['with_free_instances', 'without_free_instances', 'super_user'])
        args = parser.parse_args()

        if not validate_email(args['email'],
                          check_mx=current_app.config['EMAIL_CHECK_MX'],
                          verify=current_app.config['EMAIL_CHECK_SMTP']):
            return ({'error': 'email invalid'}, 400)

        end_point = None
        if args['end_point_id']:
            end_point = models.EndPoint.query.get_or_404(args['end_point_id'])
        else:
            end_point = models.EndPoint.get_default()

        try:
            user = models.User(login=args['login'], email=args['email'])
            user.type = args['type']
            user.end_point = end_point
            db.session.add(user)
            db.session.commit()
            return marshal(user, user_fields_full)
        except (sqlalchemy.exc.IntegrityError, sqlalchemy.orm.exc.FlushError):
            return ({'error': 'duplicate user'}, 409)
        except Exception:
            logging.exception("fail")
            raise

    def put(self, user_id):
        user = models.User.query.get_or_404(user_id)
        parser = reqparse.RequestParser()
        parser.add_argument('email', type=unicode, required=False, default=user.email,
                case_sensitive=False, help='email is required', location=('json', 'values'))
        parser.add_argument('type', type=str, required=False, default=user.type, location=('json', 'values'),
                            help='type of user: [with_free_instances, without_free_instances, super_user]',
                            choices=['with_free_instances', 'without_free_instances', 'super_user'])
        parser.add_argument('end_point_id', type=int, default=user.end_point_id,
                            help='id of the end_point', location=('json', 'values'))
        args = parser.parse_args()

        if not validate_email(args['email'],
                          check_mx=current_app.config['EMAIL_CHECK_MX'],
                          verify=current_app.config['EMAIL_CHECK_SMTP']):
            return ({'error': 'email invalid'}, 400)

        end_point = models.EndPoint.query.get_or_404(args['end_point_id'])

        try:
            user.email = args['email']
            user.type = args['type']
            user.end_point = end_point
            db.session.commit()
            return marshal(user, user_fields_full)
        except (sqlalchemy.exc.IntegrityError, sqlalchemy.orm.exc.FlushError):
            return ({'error': 'duplicate user'}, 409)  # Conflict
        except Exception:
            logging.exception("fail")
            raise

    def delete(self, user_id):
        user = models.User.query.get_or_404(user_id)
        try:
            db.session.delete(user)
            db.session.commit()
        except Exception:
            logging.exception("fail")
            raise
        return ({}, 204)


class Key(flask_restful.Resource):
    def __init__(self):
        pass

    @marshal_with(key_fields)
    def get(self, user_id, key_id=None):
        try:
            return models.User.query.get_or_404(user_id).keys.all()
        except Exception:
            logging.exception("fail")
            raise

    @marshal_with(user_fields_full)
    def post(self, user_id):
        parser = reqparse.RequestParser()
        parser.add_argument('valid_until', type=types.date, required=False,
                            help='end validity date of the key', location=('json', 'values'))
        parser.add_argument('app_name', type=str, required=True, case_sensitive=False,
                            help='app name associated to this key', location=('json', 'values'))
        args = parser.parse_args()
        user = models.User.query.get_or_404(user_id)
        try:
            user.add_key(args['app_name'], valid_until=args['valid_until'])
            db.session.commit()
        except Exception:
            logging.exception("fail")
            raise
        return user

    @marshal_with(user_fields_full)
    def delete(self, user_id, key_id):
        user = models.User.query.get_or_404(user_id)
        try:
            key = user.keys.filter_by(id=key_id).first()
            if not key:
                abort(404)
            db.session.delete(key)
            db.session.commit()
        except Exception:
            logging.exception("fail")
            raise
        return user

    @marshal_with(user_fields_full)
    def put(self, user_id, key_id):
        parser = reqparse.RequestParser()
        parser.add_argument('valid_until', type=types.date, required=False,
                case_sensitive=False, help='end validity date of the key', location=('json', 'values'))
        parser.add_argument('app_name', type=str, required=True,
                case_sensitive=False, help='eapp name associated to this key', location=('json', 'values'))
        args = parser.parse_args()
        user = models.User.query.get_or_404(user_id)
        try:
            key = user.keys.filter_by(id=key_id).first()
            if not key:
                abort(404)
            if args['valid_until']:
                key.valid_until = args['valid_until']
            key.app_name = args['app_name']
            db.session.commit()
        except Exception:
            logging.exception("fail")
            raise
        return user


class Authorization(flask_restful.Resource):
    def __init__(self):
        pass

    def delete(self, user_id):
        parser = reqparse.RequestParser()
        parser.add_argument('api_id', type=int, required=True,
                            help='api_id is required', location=('json', 'values'))
        parser.add_argument('instance_id', type=int, required=True,
                            help='instance_id is required', location=('json', 'values'))
        args = parser.parse_args()

        try:
            user = models.User.query.get_or_404(user_id)
            authorizations = [a for a in user.authorizations if a.api_id == args['api_id'] and  a.instance_id == args['instance_id']]
            if not authorizations:
                abort(404)
            for authorization in authorizations:
                db.session.delete(authorization)
            db.session.commit()
        except Exception:
            logging.exception("fail")
            raise
        return marshal(user, user_fields_full)

    def post(self, user_id):
        parser = reqparse.RequestParser()
        parser.add_argument('api_id', type=int, required=True,
                            help='api_id is required', location=('json', 'values'))
        parser.add_argument('instance_id', type=int, required=True,
                            help='instance_id is required', location=('json', 'values'))
        args = parser.parse_args()

        user = models.User.query.get_or_404(user_id)
        api = models.Api.query.get_or_404(args['api_id'])
        instance = models.Instance.query.get_or_404(args['instance_id'])

        try:
            authorization = models.Authorization()
            authorization.user = user
            authorization.api = api
            authorization.instance = instance
            user.authorizations.append(authorization)
            db.session.add(authorization)
            db.session.commit()
        except (sqlalchemy.exc.IntegrityError, sqlalchemy.orm.exc.FlushError):
            return ({'error': 'duplicate entry'}, 409)
        except Exception:
            logging.exception("fail")
            raise
        return marshal(user, user_fields_full)

class EndPoint(flask_restful.Resource):

    @marshal_with(end_point_fields)
    def get(self):
        return models.EndPoint.query.all()

    def post(self):
        parser = reqparse.RequestParser()
        parser.add_argument('name', type=unicode, required=True,
                            help='name of the endpoint', location=('json'))
        args = parser.parse_args()

        try:
            end_point = models.EndPoint()
            end_point.name = args['name']
            if 'hostnames' in request.json:
                for host in request.json['hostnames']:
                    end_point.hosts.append(models.Host(host))

            db.session.add(end_point)
            db.session.commit()
        except (sqlalchemy.exc.IntegrityError, sqlalchemy.orm.exc.FlushError), e:
            return ({'error': str(e)}, 409)
        except Exception:
            logging.exception("fail")
            raise
        return marshal(end_point, end_point_fields)

    def put(self, id):
        end_point = models.EndPoint.query.get_or_404(id)
        parser = reqparse.RequestParser()
        parser.add_argument('name', type=unicode, default=end_point.name,
                            help='name of the endpoint', location=('json'))
        args = parser.parse_args()

        try:
            end_point.name = args['name']
            if 'hostnames' in request.json:
                end_point.hosts = []
                for host in request.json['hostnames']:
                    end_point.hosts.append(models.Host(host))

            db.session.commit()
        except (sqlalchemy.exc.IntegrityError, sqlalchemy.orm.exc.FlushError), e:
            return ({'error': str(e)}, 409)
        except Exception:
            logging.exception("fail")
            raise
        return marshal(end_point, end_point_fields)

    @marshal_with(user_fields_full)
    def delete(self, id):
        end_point = models.EndPoint.query.get_or_404(id)
        try:
            db.session.delete(end_point)
            db.session.commit()
        except Exception:
            logging.exception("fail")
            raise
        return ({}, 204)
