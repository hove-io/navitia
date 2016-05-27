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
from flask_restful import fields, marshal_with, marshal, reqparse, inputs
import sqlalchemy
from validate_email import validate_email
from datetime import datetime
from itertools import combinations, chain
from tyr_user_event import TyrUserEvent
from tyr_end_point_event import EndPointEventMessage, TyrEventsRabbitMq
import logging

from navitiacommon import models, parser_args_type
from navitiacommon.default_traveler_profile_params import default_traveler_profile_params, acceptable_traveler_types
from navitiacommon import models, utils
from navitiacommon.models import db
from functools import wraps
from validations import datetime_format
from tasks import create_autocomplete_depot, remove_autocomplete_depot

__ALL__ = ['Api', 'Instance', 'User', 'Key']


class FieldDate(fields.Raw):
    def format(self, value):
        if value:
            return value.isoformat()
        else:
            return 'null'

end_point_fields = {
    'id': fields.Raw,
    'name': fields.Raw,
    'default': fields.Raw,
    'hostnames': fields.List(fields.String)
}

key_fields = {
    'id': fields.Raw,
    'app_name': fields.Raw,
    'token': fields.Raw,
    'valid_until': FieldDate
}

instance_fields = {
    'id': fields.Raw,
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
    'max_duration': fields.Raw,
    'walking_transfer_penalty': fields.Raw,
    'night_bus_filter_max_factor': fields.Raw,
    'night_bus_filter_base_factor': fields.Raw,
    'priority': fields.Raw,
    'bss_provider': fields.Boolean
}

api_fields = {
    'id': fields.Raw,
    'name': fields.Raw
}

billing_plan_fields = {
    'id': fields.Raw,
    'name': fields.Raw,
    'max_request_count': fields.Raw,
    'max_object_count': fields.Raw,
    'default': fields.Raw,
}

billing_plan_fields_full = {
    'id': fields.Raw,
    'name': fields.Raw,
    'max_request_count': fields.Raw,
    'max_object_count': fields.Raw,
    'default': fields.Raw,
    'end_point': fields.Nested(end_point_fields)
}

user_fields = {
    'id': fields.Raw,
    'login': fields.Raw,
    'email': fields.Raw,
    'block_until': FieldDate,
    'type': fields.Raw(),
    'end_point': fields.Nested(end_point_fields),
    'billing_plan': fields.Nested(billing_plan_fields)
}

user_fields_full = {
    'id': fields.Raw,
    'login': fields.Raw,
    'email': fields.Raw,
    'block_until': FieldDate,
    'type': fields.Raw(),
    'keys': fields.List(fields.Nested(key_fields)),
    'authorizations': fields.List(fields.Nested({
        'instance': fields.Nested(instance_fields),
        'api': fields.Nested(api_fields)
    })),
    'end_point': fields.Nested(end_point_fields),
    'billing_plan': fields.Nested(billing_plan_fields)
}

jobs_fields = {
    'jobs': fields.List(fields.Nested({
        'id': fields.Raw,
        'state': fields.Raw,
        'created_at': FieldDate,
        'updated_at': FieldDate,
        'data_sets': fields.List(fields.Nested({
            'type': fields.Raw,
            'name': fields.Raw
        })),
        'instance': fields.Nested(instance_fields)
    }))
}

poi_types_fields = {
    'poi_types': fields.List(fields.Nested({
        'uri': fields.Raw,
        'name': fields.Raw,
    }))
}

traveler_profile = {
    'traveler_type': fields.String,
    'walking_speed': fields.Raw,
    'bike_speed': fields.Raw,
    'bss_speed': fields.Raw,
    'car_speed': fields.Raw,
    'wheelchair': fields.Boolean,
    'max_walking_duration_to_pt': fields.Raw,
    'max_bike_duration_to_pt': fields.Raw,
    'max_bss_duration_to_pt': fields.Raw,
    'max_car_duration_to_pt': fields.Raw,
    'first_section_mode': fields.List(fields.String),
    'last_section_mode': fields.List(fields.String),
    'error': fields.String,
}

autocomplete_parameter_fields = {
    'id': fields.Raw,
    'name': fields.Raw,
    'created_at': FieldDate,
    'updated_at': FieldDate,
    'street': fields.Raw,
    'address': fields.Raw,
    'poi': fields.Raw,
    'admin': fields.Raw,
    'admin_level': fields.List(fields.Integer),
}


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

class PoiType(flask_restful.Resource):
    @marshal_with(poi_types_fields)
    def get(self, instance_name):
        instance = models.Instance.query.filter_by(name=instance_name).first_or_404()
        return {'poi_types': instance.poi_types}

    @marshal_with(poi_types_fields)
    def post(self, instance_name, uri):
        instance = models.Instance.query.filter_by(name=instance_name).first_or_404()
        parser = reqparse.RequestParser()
        parser.add_argument('name', type=unicode,  case_sensitive=False,
                help='name displayed for this type of poi', location=('json', 'values'))
        args = parser.parse_args()

        try:
            poi_type = models.PoiType(uri, args['name'], instance)
            db.session.add(poi_type)
            db.session.commit()
        except Exception:
            logging.exception("fail")
            raise

        return {'poi_types': instance.poi_types}, 201

    @marshal_with(poi_types_fields)
    def put(self, instance_name, uri):
        instance = models.Instance.query.filter_by(name=instance_name).first_or_404()
        poi_type = instance.poi_types.filter_by(uri=uri).first_or_404()
        parser = reqparse.RequestParser()
        parser.add_argument('name', type=unicode, case_sensitive=False, default=poi_type.name,
                help='name displayed for this type of poi', location=('json', 'values'))
        args = parser.parse_args()

        try:
            poi_type.name = args['name']
            db.session.commit()
        except Exception:
            logging.exception("fail")
            raise

        return {'poi_types': instance.poi_types}

    @marshal_with(poi_types_fields)
    def delete(self, instance_name, uri):
        instance = models.Instance.query.filter_by(name=instance_name).first_or_404()
        poi_type = instance.poi_types.filter_by(uri=uri).first_or_404()
        try:
            db.session.delete(poi_type)
            db.session.commit()
        except Exception:
            logging.exception("fail")
            raise

        return {'poi_types': instance.poi_types}


class Instance(flask_restful.Resource):
    def __init__(self):
        pass

    @marshal_with(instance_fields)
    def get(self, id=None, name=None):
        parser = reqparse.RequestParser()
        parser.add_argument('is_free', type=inputs.boolean, required=False,
                case_sensitive=False, help='boolean for returning only free or private instances')
        args = parser.parse_args()
        args.update({'id': id, 'name': name})
        if any(v is not None for v in args.values()):
            return models.Instance.query.filter_by(**{k: v for k, v in args.items() if v is not None}).all()
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
        parser.add_argument('scenario', type=str, case_sensitive=False,
                help='the name of the scenario used by jormungandr', choices=['default',
                                                                              'keolis',
                                                                              'destineo',
                                                                              'new_default',
                                                                              'stif'],
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

        parser.add_argument('max_duration', type=int, help='latest time point of research, in second',
                            location=('json', 'values'), default=instance.max_duration)

        parser.add_argument('walking_transfer_penalty', type=int, help='transfer penalty, in second',
                            location=('json', 'values'), default=instance.walking_transfer_penalty)

        parser.add_argument('night_bus_filter_max_factor', type=float, help='night bus filter param',
                            location=('json', 'values'), default=instance.night_bus_filter_max_factor)

        parser.add_argument('night_bus_filter_base_factor', type=int, help='night bus filter param',
                            location=('json', 'values'), default=instance.night_bus_filter_base_factor)
        parser.add_argument('priority', type=int, help='instance priority',
                            location=('json', 'values'), default=instance.priority)
        parser.add_argument('bss_provider', type=bool, help='bss provider activation',
                            location=('json', 'values'), default=instance.bss_provider)
        args = parser.parse_args()

        try:
            def map_args_to_instance(attr_name):
                setattr(instance, attr_name, args[attr_name])

            map(map_args_to_instance, ['scenario',
                                       'journey_order',
                                       'max_walking_duration_to_pt',
                                       'max_bike_duration_to_pt',
                                       'max_bss_duration_to_pt',
                                       'max_car_duration_to_pt',
                                       'max_nb_transfers',
                                       'walking_speed',
                                       'bike_speed',
                                       'bss_speed',
                                       'car_speed',
                                       'min_tc_with_car',
                                       'min_tc_with_bike',
                                       'min_tc_with_bss',
                                       'min_bike',
                                       'min_bss',
                                       'min_car',
                                       'min_duration_too_long_journey',
                                       'factor_too_long_journey',
                                       'max_duration_criteria',
                                       'max_duration_fallback_mode',
                                       'max_duration',
                                       'walking_transfer_penalty',
                                       'night_bus_filter_max_factor',
                                       'night_bus_filter_base_factor',
                                       'priority',
                                       'bss_provider'])
            db.session.commit()
        except Exception:
            logging.exception("fail")
            raise
        return marshal(instance, instance_fields)


class User(flask_restful.Resource):
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
            parser.add_argument('block_until', type=datetime_format, required=False,
                    case_sensitive=False)
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
        parser.add_argument('block_until', type=datetime_format, required=False,
                            help='end block date access', location=('json', 'values'))
        parser.add_argument('end_point_id', type=int, required=False,
                            help='id of the end_point', location=('json', 'values'))
        parser.add_argument('billing_plan_id', type=int, required=False,
                            help='id of the billing_plan', location=('json', 'values'))
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
            end_point = models.EndPoint.query.get(args['end_point_id'])
        else:
            end_point = models.EndPoint.get_default()

        if not end_point:
            return ({'error': 'end_point doesn\'t exist'}, 400)

        if args['billing_plan_id']:
            billing_plan = models.BillingPlan.query.get(args['billing_plan_id'])
        else:
            billing_plan = models.BillingPlan.get_default(end_point)

        if not billing_plan:
            return ({'error': 'billing plan doesn\'t exist'}, 400)

        try:
            user = models.User(login=args['login'], email=args['email'], block_until=args['block_until'])
            user.type = args['type']
            user.end_point = end_point
            user.billing_plan = billing_plan
            db.session.add(user)
            db.session.commit()

            tyr_user_event = TyrUserEvent()
            tyr_user_event.request(user, "create_user")

            return marshal(user, user_fields_full)
        except (sqlalchemy.exc.IntegrityError, sqlalchemy.orm.exc.FlushError):
            return ({'error': 'duplicate user'}, 409)
        except Exception:
            logging.exception("fail")
            raise

    def put(self, user_id):
        user = models.User.query.get_or_404(user_id)
        parser = reqparse.RequestParser()
        parser.add_argument('login', type=unicode, required=False, default=user.login,
                case_sensitive=False, help='user identifier', location=('json', 'values'))
        parser.add_argument('email', type=unicode, required=False, default=user.email,
                case_sensitive=False, help='email is required', location=('json', 'values'))
        parser.add_argument('type', type=str, required=False, default=user.type, location=('json', 'values'),
                            help='type of user: [with_free_instances, without_free_instances, super_user]',
                            choices=['with_free_instances', 'without_free_instances', 'super_user'])
        parser.add_argument('end_point_id', type=int, default=user.end_point_id,
                            help='id of the end_point', location=('json', 'values'))
        parser.add_argument('block_until', type=datetime_format, required=False,
                            help='block until argument is not correct', location=('json', 'values'))
        parser.add_argument('billing_plan_id', type=int, default=user.billing_plan_id,
                            help='billing id of the end_point', location=('json', 'values'))
        args = parser.parse_args()

        if not validate_email(args['email'],
                          check_mx=current_app.config['EMAIL_CHECK_MX'],
                          verify=current_app.config['EMAIL_CHECK_SMTP']):
            return ({'error': 'email invalid'}, 400)

        end_point = models.EndPoint.query.get(args['end_point_id'])
        billing_plan = models.BillingPlan.query.get_or_404(args['billing_plan_id'])

        if not end_point:
            return ({'error': 'end_point doesn\'t exist'}, 400)

        if not billing_plan:
            return ({'error': 'billing_plan doesn\'t exist'}, 400)

        try:
            last_login = user.login
            user.email = args['email']
            user.login = args['login']
            user.type = args['type']
            user.block_until = args['block_until']
            user.end_point = end_point
            user.billing_plan = billing_plan
            db.session.commit()

            tyr_user_event = TyrUserEvent()
            tyr_user_event.request(user, "update_user", last_login)

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

            tyr_user_event = TyrUserEvent()
            tyr_user_event.request(user, "delete_user")

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
        parser.add_argument('valid_until', type=inputs.date, required=False,
                            help='end validity date of the key', location=('json', 'values'))
        parser.add_argument('app_name', type=str, required=True,
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
        parser.add_argument('valid_until', type=inputs.date, required=False,
                help='end validity date of the key', location=('json', 'values'))
        parser.add_argument('app_name', type=str, required=True,
                help='app name associated to this key', location=('json', 'values'))
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

            tyr_end_point_event = EndPointEventMessage(EndPointEventMessage.CREATE, end_point)
            tyr_events_rabbit_mq = TyrEventsRabbitMq()
            tyr_events_rabbit_mq.request(tyr_end_point_event)

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
            old_name = end_point.name
            end_point.name = args['name']
            if 'hostnames' in request.json:
                end_point.hosts = []
                for host in request.json['hostnames']:
                    end_point.hosts.append(models.Host(host))

            db.session.commit()

            tyr_end_point_event = EndPointEventMessage(EndPointEventMessage.UPDATE, end_point, old_name)
            tyr_events_rabbit_mq = TyrEventsRabbitMq()
            tyr_events_rabbit_mq.request(tyr_end_point_event)

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

            tyr_end_point_event = EndPointEventMessage(EndPointEventMessage.DELETE, end_point)
            tyr_events_rabbit_mq = TyrEventsRabbitMq()
            tyr_events_rabbit_mq.request(tyr_end_point_event)

        except Exception:
            logging.exception("fail")
            raise
        return ({}, 204)


class TravelerProfile(flask_restful.Resource):
    """
    Traveler profile api for creating updating and removing
    """
    def __init__(self):
        # fallback modes
        fb_modes = ['walking', 'car', 'bss', 'bike']

        parser = reqparse.RequestParser()
        parser.add_argument('walking_speed', type=parser_args_type.float_gt_0, required=False,
                            location=('json', 'values'))
        parser.add_argument('bike_speed', type=parser_args_type.float_gt_0, required=False,
                            location=('json', 'values'))
        parser.add_argument('bss_speed', type=parser_args_type.float_gt_0, required=False,
                            location=('json', 'values'))
        parser.add_argument('car_speed', type=parser_args_type.float_gt_0, required=False,
                            location=('json', 'values'))
        parser.add_argument('wheelchair', type=parser_args_type.true_false, required=False,
                            location=('json', 'values'))
        parser.add_argument('max_walking_duration_to_pt', type=parser_args_type.float_gt_0, required=False,
                            help='in second', location=('json', 'values'))
        parser.add_argument('max_bike_duration_to_pt', type=parser_args_type.float_gt_0, required=False,
                            help='in second', location=('json', 'values'))
        parser.add_argument('max_bss_duration_to_pt', type=parser_args_type.float_gt_0, required=False,
                            help='in second', location=('json', 'values'))
        parser.add_argument('max_car_duration_to_pt', type=parser_args_type.float_gt_0, required=False,
                            help='in second', location=('json', 'values'))
        parser.add_argument('first_section_mode[]',
                            type=parser_args_type.option_value(fb_modes), case_sensitive=False,
                            required=False, action='append', dest='first_section_mode', location='values')
        parser.add_argument('last_section_mode[]',
                            type=parser_args_type.option_value(fb_modes), case_sensitive=False,
                            required=False, action='append', dest='last_section_mode', location='values')

        # flask parser returns a list for first_section_mode and last_section_mode
        parser.add_argument('first_section_mode',
                            type=parser_args_type.option_value(fb_modes), required=False, location='json')
        parser.add_argument('last_section_mode',
                            type=parser_args_type.option_value(fb_modes), required=False, location='json')

        self.args = parser.parse_args()

    def check_resources(f):
        @wraps(f)
        def wrapper(*args, **kwds):
            tp = kwds.get('traveler_type')
            if tp in acceptable_traveler_types:
                return f(*args, **kwds)
            return {'error': 'traveler profile: {0} is not one of in {1}'.format(tp, acceptable_traveler_types)}, 400
        return wrapper

    @marshal_with(traveler_profile)
    @check_resources
    def get(self, name=None, traveler_type=None):
        try:
            traveler_profiles = []
            # If traveler_type is not specified, we return all existent traveler profiles of this instance
            if traveler_type is None:
                traveler_profiles += models.TravelerProfile.get_all_by_coverage(coverage=name)
            else:
                profile = models.TravelerProfile.get_by_coverage_and_type(coverage=name, traveler_type=traveler_type)
                if profile:
                    traveler_profiles.append(profile)

            if traveler_profiles:
                return traveler_profiles
            return {'error': 'No matching traveler profiles are found in db'}, 404
        except (sqlalchemy.exc.IntegrityError, sqlalchemy.orm.exc.FlushError), e:
            return {'error': str(e)}, 409
        except Exception:
            logging.exception("fail")
            raise

    @marshal_with(traveler_profile)
    @check_resources
    def post(self, name=None, traveler_type=None):
        try:
            instance = models.Instance.get_by_name(name)
            if instance is None:
                return {'error': "Coverage: {0} doesn't exist".format(name)}
            profile = models.TravelerProfile()
            profile.coverage_id = instance.id
            for (attr, default_value) in default_traveler_profile_params[traveler_type].iteritems():
                # override hardcoded values by args if args are not None
                value = default_value if self.args.get(attr) is None else self.args.get(attr)
                setattr(profile, attr, value)
            db.session.add(profile)
            db.session.commit()
            return profile
        except (sqlalchemy.exc.IntegrityError, sqlalchemy.orm.exc.FlushError), e:
            return {'error': str(e)}, 409
        except Exception:
            logging.exception("fail")
            raise

    @marshal_with(traveler_profile)
    @check_resources
    def put(self, name=None, traveler_type=None):
        profile = models.TravelerProfile.get_by_coverage_and_type(name, traveler_type)
        if profile is None:
            return {'error': 'Non profile is found to update'}, 404
        try:
            for (attr, args_value) in self.args.iteritems():
                # override hardcoded values by args if args are not None
                if args_value is not None:
                    setattr(profile, attr, args_value)
            db.session.commit()
            return profile
        except (sqlalchemy.exc.IntegrityError, sqlalchemy.orm.exc.FlushError), e:
            return {'error': str(e)}, 409
        except Exception:
            logging.exception("fail")
            raise

    @check_resources
    def delete(self, name=None, traveler_type=None):
        profile = models.TravelerProfile.get_by_coverage_and_type(name, traveler_type)
        if profile is None:
            return {'error': 'Instance: {0} has no such profile: {1} in db to delete'.format(name, traveler_type)}, 400
        try:
            db.session.delete(profile)
            db.session.commit()
            return '', 204
        except sqlalchemy.orm.exc.FlushError, e:
            return {'error': str(e)}, 409
        except sqlalchemy.orm.exc.UnmappedInstanceError:
            return {'error': 'no such profile in db to delete'}, 400
        except Exception:
            logging.exception("fail")
            raise


class BillingPlan(flask_restful.Resource):
    def get(self, billing_plan_id=None):
        if billing_plan_id:
            billing_plan = models.BillingPlan.query.get_or_404(billing_plan_id)
            return marshal(billing_plan, billing_plan_fields_full)
        else:
            billing_plans = models.BillingPlan.query.all()
            return marshal(billing_plans, billing_plan_fields_full)

    def post(self):
        parser = reqparse.RequestParser()
        parser.add_argument('name', type=unicode, required=True,
                            case_sensitive=False, help='name is required',
                            location=('json', 'values'))
        parser.add_argument('max_request_count', type=int, required=False,
                            help='max request count for this billing plan', location=('json', 'values'))
        parser.add_argument('max_object_count', type=int, required=False,
                            help='max object count for this billing plan', location=('json', 'values'))
        parser.add_argument('default', type=bool, required=False, default=True,
                            help='if this plan is the default one', location=('json', 'values'))
        parser.add_argument('end_point_id', type=int, required=False,
                            help='id of the end_point', location=('json', 'values'))
        args = parser.parse_args()

        if args['end_point_id']:
            end_point = models.EndPoint.query.get(args['end_point_id'])
        else:
            end_point = models.EndPoint.get_default()

        if not end_point:
            return ({'error': 'end_point doesn\'t exist'}, 400)

        try:
            billing_plan = models.BillingPlan(name=args['name'], max_request_count=args['max_request_count'],
                                              max_object_count=args['max_object_count'], default=args['default'])
            billing_plan.end_point = end_point
            db.session.add(billing_plan)
            db.session.commit()
            return marshal(billing_plan, billing_plan_fields_full)
        except Exception:
            logging.exception("fail")
            raise

    def put(self, billing_plan_id=None):
        billing_plan = models.BillingPlan.query.get_or_404(billing_plan_id)
        parser = reqparse.RequestParser()
        parser.add_argument('name', type=unicode, required=False, default=billing_plan.name,
                            case_sensitive=False, location=('json', 'values'))
        parser.add_argument('max_request_count', type=int, required=False, default=billing_plan.max_request_count,
                            help='max request count for this billing plan', location=('json', 'values'))
        parser.add_argument('max_object_count', type=int, required=False, default=billing_plan.max_object_count,
                            help='max object count for this billing plan', location=('json', 'values'))
        parser.add_argument('default', type=bool, required=False, default=billing_plan.default,
                            help='if this plan is the default one', location=('json', 'values'))
        parser.add_argument('end_point_id', type=int, default=billing_plan.end_point_id,
                            help='id of the end_point', location=('json', 'values'))
        args = parser.parse_args()

        end_point = models.EndPoint.query.get(args['end_point_id'])
        if not end_point:
            return ({'error': 'end_point doesn\'t exist'}, 400)

        try:
            billing_plan.name = args['name']
            billing_plan.max_request_count = args['max_request_count']
            billing_plan.max_object_count = args['max_object_count']
            billing_plan.default = args['default']
            billing_plan.end_point = end_point
            db.session.commit()
            return marshal(billing_plan, billing_plan_fields_full)
        except Exception:
            logging.exception("fail")
            raise

    def delete(self, billing_plan_id=None):
        billing_plan = models.BillingPlan.query.get_or_404(billing_plan_id)
        try:
            db.session.delete(billing_plan)
            db.session.commit()
        except Exception:
            logging.exception("fail")
            raise
        return ({}, 204)

class AutocompleteParameter(flask_restful.Resource):
    def get(self, name=None):
        if name:
            autocomplete_param = models.AutocompleteParameter.query.filter_by(name=name).first_or_404()
            return marshal(autocomplete_param, autocomplete_parameter_fields)
        else:
            autocomplete_params = models.AutocompleteParameter.query.all()
            return marshal(autocomplete_params, autocomplete_parameter_fields)

    def post(self):
        parser = reqparse.RequestParser()
        parser.add_argument('name', type=unicode, required=True,
                            case_sensitive=False, help='name is required',
                            location=('json', 'values'))
        parser.add_argument('street', type=str, required=False, default='OSM',
                            help='source for street: [OSM]',
                            location=('json', 'values'),
                            choices=utils.street_source_types)
        parser.add_argument('address', type=str, required=False, default='BANO',
                            help='source for address: [BANO, OpenAddresses]',
                            location=('json', 'values'),
                            choices=utils.address_source_types)
        parser.add_argument('poi', type=str, required=False, default='FUSIO',
                            help='source for poi: [FUSIO, OSM]',
                            location=('json', 'values'),
                            choices=utils.poi_source_types)
        parser.add_argument('admin', type=str, required=False, default='OSM',
                            help='source for admin: [FUSIO, OSM]',
                            location=('json', 'values'),
                            choices=utils.admin_source_types)
        parser.add_argument('admin_level', type=int, action='append', required=True)


        args = parser.parse_args()

        try:
            autocomplete_parameter = models.AutocompleteParameter()
            autocomplete_parameter.name = args['name']
            autocomplete_parameter.street = args['street']
            autocomplete_parameter.address = args['address']
            autocomplete_parameter.poi = args['poi']
            autocomplete_parameter.admin = args['admin']
            autocomplete_parameter.admin_level = args['admin_level']
            db.session.add(autocomplete_parameter)
            db.session.commit()
            create_autocomplete_depot.delay(autocomplete_parameter.name)

        except (sqlalchemy.exc.IntegrityError, sqlalchemy.orm.exc.FlushError):
            return ({'error': 'duplicate name'}, 409)
        except Exception:
            logging.exception("fail")
            raise
        return marshal(autocomplete_parameter, autocomplete_parameter_fields)

    def put(self, name=None):
        autocomplete_param = models.AutocompleteParameter.query.filter_by(name=name).first_or_404()
        parser = reqparse.RequestParser()
        parser.add_argument('street', type=str, required=False, default='OSM',
                            help='source for street: [BANO, OSM]',
                            location=('json', 'values'),
                            choices= utils.street_source_types)
        parser.add_argument('address', type=str, required=False, default='BANO',
                            help='source for address: [BANO, OpenAddresses]',
                            location=('json', 'values'),
                            choices=utils.address_source_types)
        parser.add_argument('poi', type=str, required=False, default='FUSIO',
                            help='source for poi: [FUSIO, OSM, PagesJaunes]',
                            location=('json', 'values'),
                            choices=utils.poi_source_types)
        parser.add_argument('admin', type=str, required=False, default='OSM',
                            help='source for admin: [FUSIO, OSM]',
                            location=('json', 'values'),
                            choices=utils.admin_source_types)
        parser.add_argument('admin_level', type=int, action='append', required=True)

        args = parser.parse_args()

        try:
            autocomplete_param.street = args['street']
            autocomplete_param.address = args['address']
            autocomplete_param.poi = args['poi']
            autocomplete_param.admin = args['admin']
            autocomplete_param.admin_level = args['admin_level']
            db.session.commit()
            create_autocomplete_depot.delay(autocomplete_param.name)

        except Exception:
            logging.exception("fail")
            raise
        return marshal(autocomplete_param, autocomplete_parameter_fields)

    def delete(self, name=None):
        autocomplete_param = models.AutocompleteParameter.query.filter_by(name=name).first_or_404()
        try:
            remove_autocomplete_depot.delay(name)
            db.session.delete(autocomplete_param)
            db.session.commit()
        except Exception:
            logging.exception("fail")
            raise
        return ({}, 204)

