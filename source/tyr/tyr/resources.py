# coding: utf-8

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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, division, unicode_literals
from flask import current_app, request
import flask_restful
from flask_restful import marshal_with, marshal, reqparse, inputs, abort

import sqlalchemy
from validate_email import validate_email
from datetime import datetime
from tyr.tyr_user_event import TyrUserEvent
from tyr.tyr_end_point_event import EndPointEventMessage, TyrEventsRabbitMq
from tyr.helper import load_instance_config
import logging
import os
import shutil
import json
from jsonschema import validate, ValidationError
from tyr.formats import (
    poi_type_conf_format,
    parse_error,
    equipments_provider_format,
    streetnetwork_backend_format,
)
from navitiacommon.default_traveler_profile_params import (
    default_traveler_profile_params,
    acceptable_traveler_types,
)
from navitiacommon import models, utils
from navitiacommon.models import db
from navitiacommon.parser_args_type import CoordFormat, PositiveFloat, BooleanType, OptionValue, geojson_argument
from functools import wraps
from tyr.validations import datetime_format, InputJsonValidator
from tyr.tasks import (
    create_autocomplete_depot,
    remove_autocomplete_depot,
    import_autocomplete,
    cities,
    COSMOGONY_REGEXP,
)
from tyr import api
from tyr.helper import get_instance_logger, save_in_tmp
from tyr.fields import *
from werkzeug.exceptions import BadRequest
import werkzeug
import six
from collections import deque


class Api(flask_restful.Resource):
    def __init__(self):
        pass

    def get(self, version=0):
        resp = marshal(models.Api.query.all(), api_fields)
        if version == 1:
            return {'api': resp}
        return resp


class Index(flask_restful.Resource):
    def get(self):
        response = {}
        for endpoint in api.endpoints:
            try:
                response[endpoint] = {'href': url_for(endpoint, _external=True)}
            except werkzeug.routing.BuildError:
                logging.warning('Could not build url for endpoint \'{}\' '.format(endpoint))
        return response


class Status(flask_restful.Resource):
    def get(self):
        def check_db():
            try:
                return db.engine.scalar('select version_num from alembic_version;')
            except Exception as e:
                logging.exception("Tyr db not reachable : {}".format(e.message))
                raise

        return {'db version': check_db()}


class Job(flask_restful.Resource):
    @marshal_with(jobs_fields)
    def get(self, instance_name=None):
        query = models.Job.query
        if instance_name:
            query = query.join(models.Instance)
            query = query.filter(models.Instance.name == instance_name)
        return {'jobs': query.order_by(models.Job.created_at.desc()).limit(30)}

    def post(self, instance_name):
        instance = models.Instance.query_existing().filter_by(name=instance_name).first_or_404()

        if not request.files:
            return {'message': 'the Data file is missing'}, 400
        content = request.files['file']
        logger = get_instance_logger(instance)
        logger.info('content received: %s', content)

        instance = load_instance_config(instance_name)
        if not os.path.exists(instance.source_directory):
            return {'error': 'input folder unavailable'}, 500

        full_file_name = os.path.join(os.path.realpath(instance.source_directory), content.filename)
        content.save(full_file_name + ".tmp")
        shutil.move(full_file_name + ".tmp", full_file_name)

        return {'message': 'OK'}, 200

    def delete(self, id=None, instance_name=None):
        parser = reqparse.RequestParser()
        parser.add_argument('state', type=str, required=False, case_sensitive=True, help='filter by job state')
        parser.add_argument('confirm', type=str, required=True, case_sensitive=True, help='confirm the deletion')
        args = parser.parse_args()

        if args["confirm"] == "yes":
            if id:
                jobs_to_delete = [models.Job.get(id)]
            else:

                query = models.Job.query
                if "state" in args and args["state"]:
                    query = query.filter_by(state=args["state"])
                if instance_name:
                    query = query.join(models.Instance).filter(models.Instance.name == instance_name)
                jobs_to_delete = query.all()

            if not jobs_to_delete:
                return json.loads('{}'), 204

            for job in jobs_to_delete:
                try:
                    db.session.delete(job)
                except Exception:
                    logging.exception("Failed to delete Job {} - Operation aborted".format(job.id))
                    raise

            db.session.commit()
            return json.loads('{}'), 200
        else:
            return {'message': 'confirm the deletion with parameter "confirm=yes"'}, 400


def _validate_poi_types_json(poi_types_json):
    """
    poi_types configuration must follow some rules so that the binarisation is OK.
    It's checked at bina, but tyr must also reject broken config directly so that the user knows.
    """

    # Check that the conf is a valid json
    try:
        validate(poi_types_json, poi_type_conf_format)
    except ValidationError as e:
        abort(400, status="error", message='{}'.format(parse_error(e)))

    # Check that poi_type.id defined are unique
    poi_types_map = {}
    for p in poi_types_json.get('poi_types', []):
        if p.get('id') in poi_types_map:
            abort(400, status="error", message='POI type id {} is defined multiple times'.format(p.get('id')))
        poi_types_map[p.get('id')] = p.get('name')

    # Check that poi_type.id 'amenity:parking' and 'amenity:bicycle_rental' are defined.
    # Those are mandatory as they are used for journey processing (BSS and car).
    if not 'amenity:parking' in poi_types_map or not 'amenity:bicycle_rental' in poi_types_map:
        abort(
            400,
            status="error",
            message='The 2 POI types id=amenity:parking and id=amenity:bicycle_rental must be defined',
        )

    # Check that rules to affect poi_types to OSM object are using a poi_type.id defined in "poi_types" list.
    for r in poi_types_json.get('rules', []):
        pt_id = r.get('poi_type_id')
        if not pt_id in poi_types_map:
            abort(
                400,
                status="error",
                message='Using an undefined POI type id ({}) forbidden in rules'.format(pt_id),
            )


class InstancePoiType(flask_restful.Resource):
    def get(self, instance_name):
        instance = models.Instance.query_existing().filter_by(name=instance_name).first_or_404()
        poi_types = '{}'
        if instance.poi_type_json and instance.poi_type_json.poi_types_json:
            poi_types = instance.poi_type_json.poi_types_json
        return json.loads(poi_types)

    def post(self, instance_name):
        instance = models.Instance.query_existing().filter_by(name=instance_name).first_or_404()
        try:
            poi_types_json = request.get_json(silent=False)
        except:
            abort(400, status="error", message='Incorrect json provided')

        try:
            _validate_poi_types_json(poi_types_json)

            poi_types = models.PoiTypeJson(
                six.text_type(json.dumps(poi_types_json, ensure_ascii=False)), instance
            )
            db.session.add(poi_types)
            db.session.commit()
        except Exception:
            logging.exception("fail")
            raise

        return json.loads(instance.poi_type_json.poi_types_json), 200

    def delete(self, instance_name):
        instance = models.Instance.query_existing().filter_by(name=instance_name).first_or_404()
        poi_types = instance.poi_type_json
        try:
            if poi_types:
                db.session.delete(poi_types)
                db.session.commit()
        except Exception:
            logging.exception("fail")
            raise

        return json.loads('{}'), 204


class AutocompletePoiType(flask_restful.Resource):
    def get(self, name):
        autocomplete_param = models.AutocompleteParameter.query.filter_by(name=name).first_or_404()
        poi_types = '{}'
        if autocomplete_param.poi_types_json:
            poi_types = autocomplete_param.poi_types_json
        return json.loads(poi_types)

    def post(self, name):
        autocomplete_param = models.AutocompleteParameter.query.filter_by(name=name).first_or_404()
        try:
            poi_types_json = request.get_json(silent=False)
        except:
            abort(400, status="error", message='Incorrect json provided')

        try:
            _validate_poi_types_json(poi_types_json)
            autocomplete_param.poi_types_json = six.text_type(json.dumps(poi_types_json, ensure_ascii=False))
            db.session.commit()
        except Exception:
            logging.exception("fail")
            raise

        return json.loads(autocomplete_param.poi_types_json), 200

    def delete(self, name):
        autocomplete_param = models.AutocompleteParameter.query.filter_by(name=name).first_or_404()
        try:
            autocomplete_param.poi_types_json = None
            db.session.commit()
        except Exception:
            logging.exception("fail")
            raise

        return json.loads('{}'), 204


class Instance(flask_restful.Resource):
    def __init__(self):
        pass

    @marshal_with(instance_fields)
    def _get(self, id, name):
        parser = reqparse.RequestParser()
        parser.add_argument(
            'is_free',
            type=inputs.boolean,
            required=False,
            case_sensitive=False,
            help='boolean for returning only free or private instances',
        )
        args = parser.parse_args()
        args.update({'id': id, 'name': name})
        if any(v is not None for v in args.values()):
            return (
                models.Instance.query_existing()
                .filter_by(**{k: v for k, v in args.items() if v is not None})
                .all()
            )
        else:
            return models.Instance.query_existing().all()

    def get(self, version=0, id=None, name=None):
        resp = self._get(id, name)
        if version == 1:
            status_code = 404 if not resp else 200
            return {'instances': resp}, status_code
        return resp

    def delete(self, version=0, id=None, name=None):
        instance = models.Instance.get_from_id_or_name(id, name)

        try:
            instance.discarded = True
            db.session.commit()
        except Exception:
            logging.exception("fail")
            raise

        return marshal(instance, instance_fields)

    def post(self, name, version=0):
        '''
        Create a default instance with a specific name.
        '''
        query = models.Instance.query_all().filter(models.Instance.name == name)
        if query.count() > 0:
            return {'instance': name, 'created': 'failed', 'error': 'already created'}, 400

        try:
            new_instance = models.Instance(name=name)
            db.session.add(new_instance)
            db.session.commit()
            return {'instance': name, 'created': 'ok'}, 201
        except Exception as e:
            return {'instance': name, 'created': 'failed', 'error': e.message()}, 500

    def put(self, version=0, id=None, name=None):
        instance = models.Instance.get_from_id_or_name(id, name)

        parser = reqparse.RequestParser()
        parser.add_argument(
            'scenario',
            type=str,
            case_sensitive=False,
            help='the name of the scenario used by jormungandr',
            choices=['new_default', 'distributed'],
            location=('json', 'values'),
            default=instance.scenario,
        )
        parser.add_argument(
            'journey_order',
            type=str,
            case_sensitive=False,
            help='the sort order of the journeys in jormungandr',
            choices=['arrival_time', 'departure_time'],
            location=('json', 'values'),
            default=instance.journey_order,
        )
        parser.add_argument(
            'max_walking_duration_to_pt',
            type=int,
            help='the maximum duration of walking in fallback section',
            location=('json', 'values'),
            default=instance.max_walking_duration_to_pt,
        )
        parser.add_argument(
            'max_bike_duration_to_pt',
            type=int,
            help='the maximum duration of bike in fallback section',
            location=('json', 'values'),
            default=instance.max_bike_duration_to_pt,
        )
        parser.add_argument(
            'max_bss_duration_to_pt',
            type=int,
            help='the maximum duration of bss in fallback section',
            location=('json', 'values'),
            default=instance.max_bss_duration_to_pt,
        )
        parser.add_argument(
            'max_car_duration_to_pt',
            type=int,
            help='the maximum duration of car in fallback section',
            location=('json', 'values'),
            default=instance.max_car_duration_to_pt,
        )
        parser.add_argument(
            'max_car_no_park_duration_to_pt',
            type=int,
            help='the maximum duration of car in fallback section when parking aren\'t used',
            location=('json', 'values'),
            default=instance.max_car_no_park_duration_to_pt,
        )
        parser.add_argument(
            'max_nb_transfers',
            type=int,
            help='the maximum number of transfers in a journey',
            location=('json', 'values'),
            default=instance.max_nb_transfers,
        )
        parser.add_argument(
            'walking_speed',
            type=float,
            help='the walking speed',
            location=('json', 'values'),
            default=instance.walking_speed,
        )
        parser.add_argument(
            'bike_speed',
            type=float,
            help='the biking speed',
            location=('json', 'values'),
            default=instance.bike_speed,
        )
        parser.add_argument(
            'bss_speed',
            type=float,
            help='the speed of bss',
            location=('json', 'values'),
            default=instance.bss_speed,
        )
        parser.add_argument(
            'car_speed',
            type=float,
            help='the speed of car',
            location=('json', 'values'),
            default=instance.car_speed,
        )
        parser.add_argument(
            'car_no_park_speed',
            type=float,
            help='the speed of car when parking aren\'t used',
            location=('json', 'values'),
            default=instance.car_no_park_speed,
        )
        parser.add_argument(
            'taxi_speed',
            type=float,
            help='the speed of taxi',
            location=('json', 'values'),
            default=instance.taxi_speed,
        )
        parser.add_argument(
            'min_bike',
            type=int,
            help='minimum duration of bike fallback',
            location=('json', 'values'),
            default=instance.min_bike,
        )
        parser.add_argument(
            'min_bss',
            type=int,
            help='minimum duration of bss fallback',
            location=('json', 'values'),
            default=instance.min_bss,
        )
        parser.add_argument(
            'min_car',
            type=int,
            help='minimum duration of car fallback',
            location=('json', 'values'),
            default=instance.min_car,
        )
        parser.add_argument(
            'min_taxi',
            type=int,
            help='minimum duration of taxi fallback',
            location=('json', 'values'),
            default=instance.min_taxi,
        )
        parser.add_argument(
            'successive_physical_mode_to_limit_id',
            type=str,
            help='the id of physical_mode to limit succession, as sent by kraken to jormungandr,'
            ' used by _max_successive_physical_mode rule',
            location=('json', 'values'),
            default=instance.successive_physical_mode_to_limit_id,
        )

        parser.add_argument(
            'max_duration',
            type=int,
            help='latest time point of research, in second',
            location=('json', 'values'),
            default=instance.max_duration,
        )

        parser.add_argument(
            'walking_transfer_penalty',
            type=int,
            help='transfer penalty, in second',
            location=('json', 'values'),
            default=instance.walking_transfer_penalty,
        )

        parser.add_argument(
            'night_bus_filter_max_factor',
            type=float,
            help='night bus filter param',
            location=('json', 'values'),
            default=instance.night_bus_filter_max_factor,
        )

        parser.add_argument(
            'night_bus_filter_base_factor',
            type=int,
            help='night bus filter param',
            location=('json', 'values'),
            default=instance.night_bus_filter_base_factor,
        )
        parser.add_argument(
            'priority',
            type=int,
            help='instance priority, highest value will be chosen first in instances comparison',
            location=('json', 'values'),
            default=instance.priority,
        )
        parser.add_argument(
            'bss_provider',
            type=inputs.boolean,
            help='bss provider activation',
            location=('json', 'values'),
            default=instance.bss_provider,
        )
        parser.add_argument(
            'full_sn_geometries',
            type=inputs.boolean,
            help='activation of full geometries',
            location=('json', 'values'),
            default=instance.full_sn_geometries,
        )
        parser.add_argument(
            'is_free',
            type=inputs.boolean,
            help='instance doesn\'t require authorization to be used',
            location=('json', 'values'),
            default=instance.is_free,
        )
        parser.add_argument(
            'is_open_data',
            type=inputs.boolean,
            help='instance only use open data',
            location=('json', 'values'),
            default=instance.is_open_data,
        )
        parser.add_argument(
            'import_stops_in_mimir',
            type=inputs.boolean,
            help='import stops in global autocomplete',
            location=('json', 'values'),
            default=instance.import_stops_in_mimir,
        )
        parser.add_argument(
            'import_ntfs_in_mimir',
            type=inputs.boolean,
            help='import ntfs data in global autocomplete',
            location=('json', 'values'),
            default=instance.import_ntfs_in_mimir,
        )
        parser.add_argument(
            'admins_from_cities_db',
            type=inputs.boolean,
            help='use cities db while importing data from osm',
            location=('json', 'values'),
            default=instance.admins_from_cities_db,
        )
        parser.add_argument(
            'min_nb_journeys',
            type=int,
            help='minimum number of different suggested journeys',
            location=('json', 'values'),
            default=instance.min_nb_journeys,
        )
        parser.add_argument(
            'max_nb_journeys',
            type=int,
            required=False,
            help='maximum number of different suggested journeys',
            location=('json', 'values'),
        )
        parser.add_argument(
            'min_journeys_calls',
            type=int,
            help='minimum number of calls to kraken',
            location=('json', 'values'),
            default=instance.min_journeys_calls,
        )
        parser.add_argument(
            'max_successive_physical_mode',
            type=int,
            required=False,
            help='maximum number of successive physical modes in an itinerary',
            location=('json', 'values'),
        )
        parser.add_argument(
            'final_line_filter',
            type=inputs.boolean,
            help='filter on vj using same lines and same stops',
            location=('json', 'values'),
            default=instance.final_line_filter,
        )
        parser.add_argument(
            'max_extra_second_pass',
            type=int,
            help='maximum number of second pass to get more itineraries',
            location=('json', 'values'),
            default=instance.max_extra_second_pass,
        )

        parser.add_argument(
            'max_nb_crowfly_by_mode',
            type=dict,
            help='maximum nb of crowfly, used before computing the fallback matrix,' ' in distributed scenario',
            location=('json', 'values'),
            default=instance.max_nb_crowfly_by_mode,
        )

        parser.add_argument(
            'autocomplete_backend',
            type=str,
            case_sensitive=False,
            help='the name of the backend used by jormungandr for the autocompletion',
            choices=['kraken', 'bragi'],
            location=('json', 'values'),
            default=instance.autocomplete_backend,
        )

        parser.add_argument(
            'additional_time_after_first_section_taxi',
            type=int,
            help='additionnal time after the taxi section when used as first section mode',
            location=('json', 'values'),
            default=instance.additional_time_after_first_section_taxi,
        )

        parser.add_argument(
            'additional_time_before_last_section_taxi',
            type=int,
            help='additionnal time before the taxi section when used as last section mode',
            location=('json', 'values'),
            default=instance.additional_time_before_last_section_taxi,
        )

        parser.add_argument(
            'max_additional_connections',
            type=int,
            help='maximum number of connections allowed in journeys',
            location=('json', 'values'),
            default=instance.max_additional_connections,
        )

        parser.add_argument(
            'car_park_provider',
            type=inputs.boolean,
            help='boolean to activate / deactivate call to car parking provider',
            location=('json', 'values'),
            default=instance.car_park_provider,
        )

        parser.add_argument(
            'equipment_details_providers',
            type=str,
            action="append",
            help='list of ids of equipment providers available for the instance',
            location=('json', 'values'),
            default=[],
        )

        parser.add_argument(
            'ridesharing_services',
            type=str,
            action="append",
            help='list of ids of ridesharing services available for the instance',
            location=('json', 'values'),
            default=[],
        )

        list_modes = ["car", "car_no_park", "walking", "bike", "bss", "ridesharing", "taxi"]
        for mode in list_modes:
            parser.add_argument(
                'street_network_{}'.format(mode),
                type=str,
                help='the backend to use for the mode {}'.format(mode),
                location=('json', 'values'),
                default=getattr(instance, "street_network_{}".format(mode)),
            )
            parser.add_argument(
                'max_{}_direct_path_duration'.format(mode),
                type=int,
                help='maximum duration of direct path for the mode {}'.format(mode),
                location=('json', 'values'),
                default=getattr(instance, "max_{}_direct_path_duration".format(mode)),
            )

        parser.add_argument(
            'poi_dataset',
            type=str,
            help='poi dataset utilisé pour autocomplete',
            location=('json', 'values'),
            default=instance.poi_dataset,
        )
        parser.add_argument(
            'max_taxi_duration_to_pt',
            type=int,
            help='the maximum duration of taxi in fallback section',
            location=('json', 'values'),
            default=instance.max_taxi_duration_to_pt,
        )
        parser.add_argument(
            'ridesharing_speed',
            type=float,
            help='the ridesharing speed',
            location=('json', 'values'),
            default=instance.ridesharing_speed,
        )
        parser.add_argument(
            'max_ridesharing_duration_to_pt',
            type=int,
            help='the maximum duration of ridesharing in fallback section',
            location=('json', 'values'),
            default=instance.max_ridesharing_duration_to_pt,
        )
        parser.add_argument(
            'asynchronous_ridesharing',
            type=inputs.boolean,
            help='Active the asynchronous mode for ridesharing',
            location=('json', 'values'),
            default=instance.asynchronous_ridesharing,
        )

        args = parser.parse_args()

        try:

            def map_args_to_instance(attr_name):
                setattr(instance, attr_name, args[attr_name])

            deque(
                map(
                    map_args_to_instance,
                    [
                        'scenario',
                        'journey_order',
                        'max_walking_duration_to_pt',
                        'max_bike_duration_to_pt',
                        'max_bss_duration_to_pt',
                        'max_car_duration_to_pt',
                        'max_car_no_park_duration_to_pt',
                        'max_nb_transfers',
                        'walking_speed',
                        'bike_speed',
                        'bss_speed',
                        'car_speed',
                        'car_no_park_speed',
                        'taxi_speed',
                        'min_bike',
                        'min_bss',
                        'min_car',
                        'min_taxi',
                        'max_duration',
                        'walking_transfer_penalty',
                        'night_bus_filter_max_factor',
                        'night_bus_filter_base_factor',
                        'successive_physical_mode_to_limit_id',
                        'priority',
                        'bss_provider',
                        'full_sn_geometries',
                        'is_free',
                        'is_open_data',
                        'import_stops_in_mimir',
                        'import_ntfs_in_mimir',
                        'admins_from_cities_db',
                        'min_nb_journeys',
                        'max_nb_journeys',
                        'min_journeys_calls',
                        'max_successive_physical_mode',
                        'final_line_filter',
                        'max_extra_second_pass',
                        'autocomplete_backend',
                        'additional_time_after_first_section_taxi',
                        'additional_time_before_last_section_taxi',
                        'max_additional_connections',
                        'car_park_provider',
                        'street_network_car',
                        'street_network_car_no_park',
                        'street_network_walking',
                        'street_network_bike',
                        'street_network_bss',
                        'street_network_ridesharing',
                        'street_network_taxi',
                        'max_walking_direct_path_duration',
                        'max_bike_direct_path_duration',
                        'max_bss_direct_path_duration',
                        'max_car_direct_path_duration',
                        'max_taxi_direct_path_duration',
                        'max_ridesharing_direct_path_duration',
                        'poi_dataset',
                        'max_taxi_duration_to_pt',
                        'max_car_no_park_direct_path_duration',
                        'ridesharing_speed',
                        'max_ridesharing_duration_to_pt',
                        'asynchronous_ridesharing',
                    ],
                ),
                maxlen=0,
            )
            max_nb_crowfly_by_mode = args.get('max_nb_crowfly_by_mode')
            import copy

            new = copy.deepcopy(instance.max_nb_crowfly_by_mode)
            new.update(max_nb_crowfly_by_mode)
            instance.max_nb_crowfly_by_mode = new

            instance.equipment_details_providers = []
            for provider_id in args.equipment_details_providers:
                equipment_provider = models.EquipmentsProvider.query.get(provider_id)
                if equipment_provider:
                    instance.equipment_details_providers.append(equipment_provider)
                else:
                    msg = "Couldn't set equipment providers - Provider '{}' isn't present in db".format(
                        provider_id
                    )
                    return {"message": msg}, 400

            instance.ridesharing_services = []
            for ridesharing_service_id in args.ridesharing_services:
                ridesharing_service = models.RidesharingService.query.get(ridesharing_service_id)
                if ridesharing_service:
                    instance.ridesharing_services.append(ridesharing_service)
                else:
                    msg = "Couldn't set ridesharing services - '{}' isn't present in db".format(
                        ridesharing_service_id
                    )
                    return {"message": msg}, 400

            db.session.commit()
        except Exception:
            logging.exception("fail")
            raise
        return marshal(instance, instance_fields)


class User(flask_restful.Resource):
    def _get_args(self, user_id):
        parser = reqparse.RequestParser()
        parser.add_argument(
            'disable_geojson', type=inputs.boolean, default=True, help='remove geojson from the response'
        )
        if user_id:
            return parser

        parser.add_argument('login', type=six.text_type, required=False, case_sensitive=False, help='login')
        parser.add_argument('email', type=six.text_type, required=False, case_sensitive=False, help='email')
        parser.add_argument('key', type=six.text_type, required=False, case_sensitive=False, help='key')
        parser.add_argument('end_point_id', type=int)
        parser.add_argument('block_until', type=datetime_format, required=False, case_sensitive=False)
        return parser

    def _get_user_by_id(self, user_id):
        return models.User.query.get_or_404(user_id)

    def _get_user_by_keys(self, keys):
        logging.debug(keys)
        return models.User.get_from_token(keys, datetime.now())

    def _get_all_users(self, args):
        del args['disable_geojson']
        filter_params = {k: v for k, v in args.items() if v}
        if filter_params:
            users = models.User.query.filter_by(**filter_params).all()
            return marshal(users, user_fields)
        else:
            users = models.User.query.all()
            return marshal(users, user_fields)

    def get(self, user_id=None):
        args = self._get_args(user_id).parse_args()
        g.disable_geojson = args['disable_geojson']
        if user_id:
            return marshal(self._get_user_by_id(user_id), user_fields_full)
        else:
            if args['key']:
                return marshal(self._get_user_by_keys(args['key']), user_fields)
            else:
                return self._get_all_users(args)

    def post(self):
        user = None
        parser = reqparse.RequestParser()
        parser.add_argument(
            'login',
            type=six.text_type,
            required=True,
            case_sensitive=False,
            help='login is required',
            location=('json', 'values'),
        )
        parser.add_argument(
            'email',
            type=six.text_type,
            required=True,
            case_sensitive=False,
            help='email is required',
            location=('json', 'values'),
        )
        parser.add_argument(
            'block_until',
            type=datetime_format,
            required=False,
            help='end block date access',
            location=('json', 'values'),
        )
        parser.add_argument(
            'end_point_id', type=int, required=False, help='id of the end_point', location=('json', 'values')
        )
        parser.add_argument(
            'billing_plan_id',
            type=int,
            required=False,
            help='id of the billing_plan',
            location=('json', 'values'),
        )
        parser.add_argument(
            'type',
            type=str,
            required=False,
            default='with_free_instances',
            help='type of user: [with_free_instances, without_free_instances, super_user]',
            location=('json', 'values'),
            choices=['with_free_instances', 'without_free_instances', 'super_user'],
        )
        parser.add_argument('shape', type=geojson_argument, required=False, location=('json', 'values'))
        parser.add_argument('default_coord', type=CoordFormat(), required=False, location=('json', 'values'))
        args = parser.parse_args()

        if not validate_email(
            args['email'],
            check_mx=current_app.config['EMAIL_CHECK_MX'],
            verify=current_app.config['EMAIL_CHECK_SMTP'],
        ):
            return {'error': 'email invalid'}, 400

        end_point = None
        if args['end_point_id']:
            end_point = models.EndPoint.query.get(args['end_point_id'])
        else:
            end_point = models.EndPoint.get_default()

        if not end_point:
            return {'error': 'end_point doesn\'t exist'}, 400

        if args['billing_plan_id']:
            billing_plan = models.BillingPlan.query.get(args['billing_plan_id'])
        else:
            billing_plan = models.BillingPlan.get_default(end_point)

        if not billing_plan:
            return {'error': 'billing plan doesn\'t exist'}, 400

        try:
            user = models.User(login=args['login'], email=args['email'], block_until=args['block_until'])
            user.type = args['type']
            user.end_point = end_point
            user.billing_plan = billing_plan
            user.shape = ujson.dumps(args['shape'])
            user.default_coord = args['default_coord']
            db.session.add(user)
            db.session.commit()

            tyr_user_event = TyrUserEvent()
            tyr_user_event.request(user, "create_user")

            return marshal(user, user_fields_full), 200
        except (sqlalchemy.exc.IntegrityError, sqlalchemy.orm.exc.FlushError):
            return {'error': 'duplicate user'}, 409
        except Exception:
            logging.exception("fail")
            raise

    def put(self, user_id):
        user = models.User.query.get_or_404(user_id)
        parser = reqparse.RequestParser()
        parser.add_argument(
            'login',
            type=six.text_type,
            required=False,
            default=user.login,
            case_sensitive=False,
            help='user identifier',
            location=('json', 'values'),
        )
        parser.add_argument(
            'email',
            type=six.text_type,
            required=False,
            default=user.email,
            case_sensitive=False,
            help='email is required',
            location=('json', 'values'),
        )
        parser.add_argument(
            'type',
            type=str,
            required=False,
            default=user.type,
            location=('json', 'values'),
            help='type of user: [with_free_instances, without_free_instances, super_user]',
            choices=['with_free_instances', 'without_free_instances', 'super_user'],
        )
        parser.add_argument(
            'end_point_id',
            type=int,
            default=user.end_point_id,
            help='id of the end_point',
            location=('json', 'values'),
        )
        parser.add_argument(
            'block_until',
            type=datetime_format,
            required=False,
            help='block until argument is not correct',
            location=('json', 'values'),
        )
        parser.add_argument(
            'billing_plan_id',
            type=int,
            default=user.billing_plan_id,
            help='billing id of the end_point',
            location=('json', 'values'),
        )
        parser.add_argument(
            'shape',
            type=geojson_argument,
            default=ujson.loads(user.shape),
            required=False,
            location=('json', 'values'),
        )
        parser.add_argument('default_coord', type=CoordFormat(), required=False, location=('json', 'values'))
        args = parser.parse_args()

        if not validate_email(
            args['email'],
            check_mx=current_app.config['EMAIL_CHECK_MX'],
            verify=current_app.config['EMAIL_CHECK_SMTP'],
        ):
            return {'error': 'email invalid'}, 400

        end_point = models.EndPoint.query.get(args['end_point_id'])
        if not end_point:
            return {'error': 'end_point doesn\'t exist'}, 400

        billing_plan = models.BillingPlan.query.get_or_404(args['billing_plan_id'])
        if not billing_plan:
            return {'error': 'billing_plan doesn\'t exist'}, 400

        # If the user gives the empty object, we don't change the
        # shape. This is because the empty object can be outputed by
        # GET to express "there is a shape, but I don't show it to you
        # as you don't care". We want that giving the result of GET to
        # PUT doesn't change anything. That explain this strangeness.
        if args['shape'] == {}:
            args['shape'] = ujson.loads(user.shape)

        try:
            last_login = user.login
            user.email = args['email']
            user.login = args['login']
            user.type = args['type']
            user.block_until = args['block_until']
            user.end_point = end_point
            user.billing_plan = billing_plan
            user.shape = ujson.dumps(args['shape'])
            user.default_coord = args['default_coord']
            db.session.commit()

            tyr_user_event = TyrUserEvent()
            tyr_user_event.request(user, "update_user", last_login)

            return marshal(user, user_fields_full), 200
        except (sqlalchemy.exc.IntegrityError, sqlalchemy.orm.exc.FlushError):
            return {'error': 'duplicate user'}, 409
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
        return {}, 204


class UserV1(User):
    def _get_args(self, user_id):
        parser = super(UserV1, self)._get_args(user_id)
        parser.add_argument('page', type=int, required=False, default=1)
        return parser

    def _get_all_users(self, args):
        del args['disable_geojson']
        filter_params = {k: v for k, v in args.items() if v and k != 'page'}

        pagination = models.User.query.filter_by(**filter_params).paginate(
            args['page'], current_app.config.get('MAX_ITEMS_PER_PAGE', 5)
        )
        pagination_json = {
            'current_page': pagination.page,
            'items_per_page': pagination.per_page,
            'total_items': pagination.total,
        }
        if pagination.has_next:
            pagination_json['next'] = url_for(request.endpoint, page=pagination.next_num)
        return marshal(pagination.items, user_fields), pagination_json

    def get(self, user_id=None):
        resp = super(UserV1, self).get(user_id)
        # In case of a response with pagination, the response type is a tuple and it needs to be serialized
        if type(resp) == tuple:
            return {'users': resp[0], 'pagination': resp[1]}
        return {'users': resp}

    def put(self, user_id):
        resp = super(UserV1, self).put(user_id)
        return {'user': resp[0]}, resp[1] if resp[1] == 200 else resp

    def post(self):
        resp = super(UserV1, self).post()
        return {'user': resp[0]}, resp[1] if resp[1] == 200 else resp


class Key(flask_restful.Resource):
    def __init__(self):
        pass

    def get(self, user_id, key_id=None, version=0):
        try:
            keys = models.User.query.get_or_404(user_id).keys.all()
        except Exception:
            logging.exception("fail")
            raise
        resp = marshal(keys, key_fields)
        if version == 1:
            return {'keys': resp}
        return resp

    def post(self, user_id, version=0):
        parser = reqparse.RequestParser()
        parser.add_argument(
            'valid_until',
            type=inputs.date,
            required=False,
            help='end validity date of the key',
            location=('json', 'values'),
        )
        parser.add_argument(
            'app_name',
            type=str,
            required=True,
            help='app name associated to this key',
            location=('json', 'values'),
        )
        args = parser.parse_args()
        user = models.User.query.get_or_404(user_id)
        try:
            user.add_key(args['app_name'], valid_until=args['valid_until'])
            db.session.commit()
        except Exception:
            logging.exception("fail")
            raise
        resp = marshal(user, user_fields_full)
        if version == 1:
            return {'user': resp}
        return resp

    def delete(self, user_id, key_id, version=0):
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
        if version == 1:
            return {'user': marshal(user, user_fields_full)}
        return marshal(user, user_fields_full)

    def put(self, user_id, key_id, version=0):
        parser = reqparse.RequestParser()
        parser.add_argument(
            'valid_until',
            type=inputs.date,
            required=False,
            help='end validity date of the key',
            location=('json', 'values'),
        )
        parser.add_argument(
            'app_name',
            type=str,
            required=True,
            help='app name associated to this key',
            location=('json', 'values'),
        )
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
        resp = marshal(user, user_fields_full)
        if version == 1:
            return {'user': resp}
        return resp


class Authorization(flask_restful.Resource):
    def __init__(self):
        pass

    def delete(self, user_id, version=0):
        parser = reqparse.RequestParser()
        parser.add_argument(
            'api_id', type=int, required=True, help='api_id is required', location=('json', 'values')
        )
        parser.add_argument(
            'instance_id', type=int, required=True, help='instance_id is required', location=('json', 'values')
        )
        args = parser.parse_args()

        try:
            user = models.User.query.get_or_404(user_id)
            authorizations = [
                a
                for a in user.authorizations
                if a.api_id == args['api_id'] and a.instance_id == args['instance_id']
            ]
            if not authorizations:
                abort(404)
            for authorization in authorizations:
                db.session.delete(authorization)
            db.session.commit()
        except Exception:
            logging.exception("fail")
            raise
        resp = marshal(user, user_fields_full)
        if version == 1:
            return {'user': resp}
        return resp

    def post(self, user_id, version=0):
        parser = reqparse.RequestParser()
        parser.add_argument(
            'api_id', type=int, required=True, help='api_id is required', location=('json', 'values')
        )
        parser.add_argument(
            'instance_id', type=int, required=True, help='instance_id is required', location=('json', 'values')
        )
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
            return {'error': 'duplicate entry'}, 409
        except Exception:
            logging.exception("fail")
            raise
        resp = marshal(user, user_fields_full)
        if version == 1:
            return {'user': resp}
        return resp


class EndPoint(flask_restful.Resource):
    def get(self, version=0):
        resp = marshal(models.EndPoint.query.all(), end_point_fields)
        if version == 1:
            return {'end_points': resp}
        return resp

    def post(self, version=0):
        parser = reqparse.RequestParser()
        parser.add_argument(
            'name', type=six.text_type, required=True, help='name of the endpoint', location='json'
        )
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

        except (sqlalchemy.exc.IntegrityError, sqlalchemy.orm.exc.FlushError) as e:
            return {'error': str(e)}, 409
        except Exception:
            logging.exception("fail")
            raise
        resp = marshal(end_point, end_point_fields)
        if version == 1:
            return {'end_point': [resp]}, 201
        return resp

    def put(self, version=0, id=None):
        if not id:
            abort(400, status="error", message='id is required')

        end_point = models.EndPoint.query.get_or_404(id)
        parser = reqparse.RequestParser()
        parser.add_argument(
            'name', type=six.text_type, default=end_point.name, help='name of the endpoint', location=('json')
        )
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

        except (sqlalchemy.exc.IntegrityError, sqlalchemy.orm.exc.FlushError) as e:
            return {'error': str(e)}, 409
        except Exception:
            logging.exception("fail")
            raise
        resp = marshal(end_point, end_point_fields)
        if version == 1:
            return {'end_point': [resp]}
        return resp

    def delete(self, version=0, id=None):
        if not id:
            abort(400, status="error", message='id is required')

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
        return {}, 204


class TravelerProfile(flask_restful.Resource):
    """
    Traveler profile api for creating updating and removing
    """

    def __init__(self):
        # fallback modes
        fb_modes = ['walking', 'car', 'bss', 'bike', 'ridesharing']

        parser = reqparse.RequestParser()
        parser.add_argument('walking_speed', type=PositiveFloat(), required=False, location=('json', 'values'))
        parser.add_argument('bike_speed', type=PositiveFloat(), required=False, location=('json', 'values'))
        parser.add_argument('bss_speed', type=PositiveFloat(), required=False, location=('json', 'values'))
        parser.add_argument('car_speed', type=PositiveFloat(), required=False, location=('json', 'values'))
        parser.add_argument('wheelchair', type=BooleanType(), required=False, location=('json', 'values'))
        parser.add_argument(
            'max_walking_duration_to_pt',
            type=PositiveFloat(),
            required=False,
            help='in second',
            location=('json', 'values'),
        )
        parser.add_argument(
            'max_bike_duration_to_pt',
            type=PositiveFloat(),
            required=False,
            help='in second',
            location=('json', 'values'),
        )
        parser.add_argument(
            'max_bss_duration_to_pt',
            type=PositiveFloat(),
            required=False,
            help='in second',
            location=('json', 'values'),
        )
        parser.add_argument(
            'max_car_duration_to_pt',
            type=PositiveFloat(),
            required=False,
            help='in second',
            location=('json', 'values'),
        )
        parser.add_argument(
            'first_section_mode[]',
            type=OptionValue(fb_modes),
            case_sensitive=False,
            required=False,
            action='append',
            dest='first_section_mode',
            location='values',
        )
        parser.add_argument(
            'last_section_mode[]',
            type=OptionValue(fb_modes),
            case_sensitive=False,
            required=False,
            action='append',
            dest='last_section_mode',
            location='values',
        )

        # flask parser returns a list for first_section_mode and last_section_mode
        parser.add_argument(
            'first_section_mode', type=OptionValue(fb_modes), action='append', required=False, location='json'
        )
        parser.add_argument(
            'last_section_mode', type=OptionValue(fb_modes), action='append', required=False, location='json'
        )

        self.args = parser.parse_args()

    def check_resources(f):
        @wraps(f)
        def wrapper(*args, **kwds):
            tp = kwds.get('traveler_type')
            if tp in acceptable_traveler_types or tp is None:
                return f(*args, **kwds)
            return (
                {'error': 'traveler profile: {0} is not one of in {1}'.format(tp, acceptable_traveler_types)},
                400,
            )

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
                profile = models.TravelerProfile.get_by_coverage_and_type(
                    coverage=name, traveler_type=traveler_type
                )
                if profile:
                    traveler_profiles.append(profile)

            if traveler_profiles:
                return traveler_profiles
            return {'error': 'No matching traveler profiles are found in db'}, 404
        except (sqlalchemy.exc.IntegrityError, sqlalchemy.orm.exc.FlushError) as e:
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
            for (attr, default_value) in default_traveler_profile_params[traveler_type].items():
                # override hardcoded values by args if args are not None
                value = default_value if self.args.get(attr) is None else self.args.get(attr)
                setattr(profile, attr, value)
            db.session.add(profile)
            db.session.commit()
            return profile
        except (sqlalchemy.exc.IntegrityError, sqlalchemy.orm.exc.FlushError) as e:
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
            for (attr, args_value) in self.args.items():
                # override hardcoded values by args if args are not None
                if args_value is not None:
                    setattr(profile, attr, args_value)
            db.session.commit()
            return profile
        except (sqlalchemy.exc.IntegrityError, sqlalchemy.orm.exc.FlushError) as e:
            return {'error': str(e)}, 409
        except Exception:
            logging.exception("fail")
            raise

    @check_resources
    def delete(self, name=None, traveler_type=None):
        profile = models.TravelerProfile.get_by_coverage_and_type(name, traveler_type)
        if profile is None:
            return (
                {'error': 'Instance: {0} has no such profile: {1} in db to delete'.format(name, traveler_type)},
                400,
            )
        try:
            db.session.delete(profile)
            db.session.commit()
            return '', 204
        except sqlalchemy.orm.exc.FlushError as e:
            return {'error': str(e)}, 409
        except sqlalchemy.orm.exc.UnmappedInstanceError:
            return {'error': 'no such profile in db to delete'}, 400
        except Exception:
            logging.exception("fail")
            raise


class BillingPlan(flask_restful.Resource):
    def get(self, version=0, billing_plan_id=None):
        if billing_plan_id:
            billing_plans = models.BillingPlan.query.get_or_404(billing_plan_id)
        else:
            billing_plans = models.BillingPlan.query.all()
        resp = marshal(billing_plans, billing_plan_fields_full)
        if version == 1:
            return {'billing_plans': resp}
        return resp

    def post(self, version=0):
        parser = reqparse.RequestParser()
        parser.add_argument(
            'name',
            type=six.text_type,
            required=True,
            case_sensitive=False,
            help='name is required',
            location=('json', 'values'),
        )
        parser.add_argument(
            'max_request_count',
            type=int,
            required=False,
            help='max request count for this billing plan',
            location=('json', 'values'),
        )
        parser.add_argument(
            'max_object_count',
            type=int,
            required=False,
            help='max object count for this billing plan',
            location=('json', 'values'),
        )
        parser.add_argument(
            'default',
            type=bool,
            required=False,
            default=True,
            help='if this plan is the default one',
            location=('json', 'values'),
        )
        parser.add_argument(
            'end_point_id', type=int, required=False, help='id of the end_point', location=('json', 'values')
        )
        args = parser.parse_args()

        if args['end_point_id']:
            end_point = models.EndPoint.query.get(args['end_point_id'])
        else:
            end_point = models.EndPoint.get_default()

        if not end_point:
            return {'error': 'end_point doesn\'t exist'}, 400

        try:
            billing_plan = models.BillingPlan(
                name=args['name'],
                max_request_count=args['max_request_count'],
                max_object_count=args['max_object_count'],
                default=args['default'],
            )
            billing_plan.end_point = end_point
            db.session.add(billing_plan)
            db.session.commit()
            resp = marshal(billing_plan, billing_plan_fields_full)
            if version == 1:
                return {'billing_plan': [resp]}, 201
            return resp
        except Exception:
            logging.exception("fail")
            raise

    def put(self, version=0, billing_plan_id=None):
        if not id:
            abort(400, status="error", message='billing_plan_id is required')

        billing_plan = models.BillingPlan.query.get_or_404(billing_plan_id)
        parser = reqparse.RequestParser()
        parser.add_argument(
            'name',
            type=six.text_type,
            required=False,
            default=billing_plan.name,
            case_sensitive=False,
            location=('json', 'values'),
        )
        parser.add_argument(
            'max_request_count',
            type=int,
            required=False,
            default=billing_plan.max_request_count,
            help='max request count for this billing plan',
            location=('json', 'values'),
        )
        parser.add_argument(
            'max_object_count',
            type=int,
            required=False,
            default=billing_plan.max_object_count,
            help='max object count for this billing plan',
            location=('json', 'values'),
        )
        parser.add_argument(
            'default',
            type=bool,
            required=False,
            default=billing_plan.default,
            help='if this plan is the default one',
            location=('json', 'values'),
        )
        parser.add_argument(
            'end_point_id',
            type=int,
            default=billing_plan.end_point_id,
            help='id of the end_point',
            location=('json', 'values'),
        )
        args = parser.parse_args()

        end_point = models.EndPoint.query.get(args['end_point_id'])
        if not end_point:
            return {'error': 'end_point doesn\'t exist'}, 400

        try:
            billing_plan.name = args['name']
            billing_plan.max_request_count = args['max_request_count']
            billing_plan.max_object_count = args['max_object_count']
            billing_plan.default = args['default']
            billing_plan.end_point = end_point
            db.session.commit()
            resp = marshal(billing_plan, billing_plan_fields_full)
            if version == 1:
                return {'billing_plan': [resp]}
            return resp
        except Exception:
            logging.exception("fail")
            raise

    def delete(self, version=0, billing_plan_id=None):
        billing_plan = models.BillingPlan.query.get_or_404(billing_plan_id)
        try:
            db.session.delete(billing_plan)
            db.session.commit()
        except (sqlalchemy.exc.IntegrityError, sqlalchemy.orm.exc.FlushError):
            return {'error': 'billing_plan used'}, 409  # Conflict
        except Exception:
            logging.exception("fail")
            raise
        return {}, 204


class AutocompleteParameter(flask_restful.Resource):
    def get(self, version=0, name=None):
        if name:
            autocomplete_params = models.AutocompleteParameter.query.filter_by(name=name).first_or_404()
        else:
            autocomplete_params = models.AutocompleteParameter.query.all()
        resp = marshal(autocomplete_params, autocomplete_parameter_fields)
        if version == 1:
            return {'autocomplete_parameters': resp}
        return resp

    def post(self, version=0):
        parser = reqparse.RequestParser()
        parser.add_argument(
            'name',
            type=six.text_type,
            required=True,
            case_sensitive=False,
            help='name is required',
            location=('json', 'values'),
        )
        parser.add_argument(
            'street',
            type=str,
            required=False,
            default='OSM',
            help='source for street: [OSM]',
            location=('json', 'values'),
            choices=utils.street_source_types,
        )
        parser.add_argument(
            'address',
            type=str,
            required=False,
            default='BANO',
            help='source for address: {}'.format(utils.address_source_types),
            location=('json', 'values'),
            choices=utils.address_source_types,
        )
        parser.add_argument(
            'poi',
            type=str,
            required=False,
            default='OSM',
            help='source for poi: [FUSIO, OSM]',
            location=('json', 'values'),
            choices=utils.poi_source_types,
        )
        parser.add_argument(
            'admin',
            type=str,
            required=False,
            default='OSM',
            help='source for admin: {}'.format(utils.admin_source_types),
            location=('json', 'values'),
            choices=utils.admin_source_types,
        )
        parser.add_argument('admin_level', type=int, action='append', required=False)

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
            return {'error': 'duplicate name'}, 409
        except Exception:
            logging.exception("fail")
            raise

        resp = marshal(autocomplete_parameter, autocomplete_parameter_fields)
        if version == 1:
            return {'autocomplete_parameters': [resp]}, 201
        return resp

    def put(self, version=0, name=None):
        autocomplete_param = models.AutocompleteParameter.query.filter_by(name=name).first_or_404()
        parser = reqparse.RequestParser()
        parser.add_argument(
            'street',
            type=str,
            required=False,
            default=autocomplete_param.street,
            help='source for street: {}'.format(utils.street_source_types),
            location=('json', 'values'),
            choices=utils.street_source_types,
        )
        parser.add_argument(
            'address',
            type=str,
            required=False,
            default=autocomplete_param.address,
            help='source for address: {}'.format(utils.address_source_types),
            location=('json', 'values'),
            choices=utils.address_source_types,
        )
        parser.add_argument(
            'poi',
            type=str,
            required=False,
            default=autocomplete_param.poi,
            help='source for poi: {}'.format(utils.poi_source_types),
            location=('json', 'values'),
            choices=utils.poi_source_types,
        )
        parser.add_argument(
            'admin',
            type=str,
            required=False,
            default=autocomplete_param.admin,
            help='source for admin: {}'.format(utils.admin_source_types),
            location=('json', 'values'),
            choices=utils.admin_source_types,
        )
        parser.add_argument(
            'admin_level', type=int, action='append', required=False, default=autocomplete_param.admin_level
        )

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

        resp = marshal(autocomplete_param, autocomplete_parameter_fields)
        if version == 1:
            return {'autocomplete_parameters': [resp]}
        return resp

    def delete(self, version=0, name=None):
        autocomplete_param = models.AutocompleteParameter.query.filter_by(name=name).first_or_404()
        try:
            remove_autocomplete_depot.delay(name)
            db.session.delete(autocomplete_param)
            db.session.commit()
        except Exception:
            logging.exception("fail")
            raise
        return None, 204


class InstanceDataset(flask_restful.Resource):
    def get(self, instance_name):
        parser = reqparse.RequestParser()
        parser.add_argument(
            'count',
            type=int,
            required=False,
            help='number of last dataset to dump per type',
            location=('json', 'values'),
            default=1,
        )
        args = parser.parse_args()
        instance = models.Instance.get_by_name(instance_name)
        datasets = instance.last_datasets(args['count'])

        return marshal(datasets, dataset_field)


class AutocompleteDataset(flask_restful.Resource):
    def get(self, ac_instance_name):
        parser = reqparse.RequestParser()
        parser.add_argument(
            'count',
            type=int,
            required=False,
            help='number of last dataset to dump per type',
            location=('json', 'values'),
            default=1,
        )
        args = parser.parse_args()
        instance = models.AutocompleteParameter.query.filter_by(name=ac_instance_name).first_or_404()
        datasets = instance.last_datasets(args['count'])

        return marshal(datasets, dataset_field)


class AutocompleteUpdateData(flask_restful.Resource):
    def post(self, ac_instance_name):
        instance = models.AutocompleteParameter.query.filter_by(name=ac_instance_name).first_or_404()

        if not request.files:
            return marshal({'error': {'message': 'the Data file is missing'}}, error_fields), 400

        content = request.files['file']
        logger = get_instance_logger(instance)
        logger.info('content received: %s', content)
        filename = save_in_tmp(content)
        _, job = import_autocomplete([filename], instance)
        job = models.db.session.merge(job)  # reatache the object
        return marshal({'job': job}, one_job_fields), 200


class DeleteDataset(flask_restful.Resource):
    def delete(self, instance_name, type):

        instance = models.Instance.get_by_name(instance_name)
        if instance:
            res = instance.delete_dataset(_type=type)
            if res:
                return_msg = 'All {} datasets deleted for instance {}'.format(type, instance_name)
            else:
                return_msg = 'No {} dataset to be deleted for instance {}'.format(type, instance_name)
            return_status = 200
        else:
            return_msg = "No instance found for : {}".format(instance_name)
            return_status = 404

        return {'action': return_msg}, return_status


class MigrateFromPoiToOsm(flask_restful.Resource):
    def put(self, instance_name):
        instance = models.Instance.get_by_name(instance_name)
        if instance:
            instance_conf = load_instance_config(instance_name)
            connection_string = "postgres://{u}:{pw}@{h}:{port}/{db}".format(
                u=instance_conf.pg_username,
                pw=instance_conf.pg_password,
                h=instance_conf.pg_host,
                db=instance_conf.pg_dbname,
                port=instance_conf.pg_port,
            )
            engine = sqlalchemy.create_engine(connection_string)
            try:
                engine.execute("""UPDATE navitia.parameters SET parse_pois_from_osm = TRUE""").close()
            finally:
                engine.dispose()

            return_msg = 'Parameter parse_pois_from_osm activated'
            return_status = 200
        else:
            return_msg = "No instance found for : {}".format(instance_name)
            return_status = 404

        return {'action': return_msg}, return_status


def check_cities_db():
    """
    Check that the cities db is reachable
    :return: Alembic version if db is reachable else None
    """
    cities_db = sqlalchemy.create_engine(current_app.config['CITIES_DATABASE_URI'])
    try:
        cities_db.connect()
        result = cities_db.execute("SELECT version_num FROM alembic_version")
        for row in result:
            return row['version_num']
    except Exception as e:
        logging.exception("cities db not created : {}".format(e.message))
        return None
    finally:
        cities_db.dispose()


@marshal_with(job_fields)
def check_cities_job():
    """
    Check status of cities job in Tyr db
    :return: the latest cities job
    """
    return (
        models.Job.query.join(models.DataSet)
        .filter(models.DataSet.type == 'cities')
        .order_by(models.Job.created_at.desc())
        .first()
    )


class CitiesStatus(flask_restful.Resource):
    def get(self):
        response = {}
        if not current_app.config['CITIES_DATABASE_URI']:
            return {'message': 'cities db not configured'}, 404
        cities_version = check_cities_db()
        if cities_version:
            response['cities db version'] = '{}'.format(cities_version)
        else:
            return {'message': 'cities db not reachable'}, 404

        cities_job = check_cities_job()
        if cities_job and 'instance' in cities_job:
            # No instance associated to 'cities' job, remove the item
            cities_job.pop('instance', None)
            response['latest_job'] = cities_job

        return response, 200


class Cities(flask_restful.Resource):
    def post(self):
        if not check_cities_db():
            return {'message': 'cities db not reachable'}, 404

        parser = reqparse.RequestParser()
        parser.add_argument('file', type=werkzeug.FileStorage, location='files')
        args = parser.parse_args()

        if not args['file']:
            logging.error("No file provided")
            return {'message': 'No file provided'}, 400

        f = args['file']
        file_name = f.filename
        file_path = str(os.path.join(os.path.abspath(current_app.config['CITIES_OSM_FILE_PATH']), file_name))
        f.save(file_path)
        logging.info("file received: {}".format(f))

        # Create Job and Dataset in db to track progress
        job = models.Job()
        job.state = 'running'
        models.db.session.add(job)
        dataset = models.DataSet()
        dataset.name = file_path
        dataset.family_type = 'cities'
        models.db.session.add(dataset)
        job.data_sets.append(dataset)

        if COSMOGONY_REGEXP.match(file_name):
            # it's a cosmogony file, we import it with cosmogony2cities
            dataset.type = exe = 'cosmogony2cities'
        else:
            # we import it the 'old' way, with cities
            dataset.type = exe = 'cities'

        models.db.session.commit()
        try:
            cities.delay(file_path, job.id, exe)
        except:
            job.state = 'failed'
            models.db.session.commit()

        return {'message': 'OK'}, 200


class BssProvider(flask_restful.Resource):
    @marshal_with(bss_provider_list_fields)
    def get(self, id=None):
        if id:
            try:
                return {'bss_providers': [models.BssProvider.find_by_id(id)]}
            except sqlalchemy.orm.exc.NoResultFound:
                return {'bss_providers': []}, 404
        else:
            return {'bss_providers': models.BssProvider.all()}

    def post(self, id=None):
        if not id:
            abort(400, status="error", message='id is required')

        try:
            input_json = request.get_json(force=True, silent=False)
            # TODO validate input
        except BadRequest:
            abort(400, status="error", message='Incorrect json provided')

        provider = models.BssProvider(id, input_json)
        try:
            models.db.session.add(provider)
            models.db.session.commit()
        except sqlalchemy.exc.IntegrityError as ex:
            abort(400, status="error", message=str(ex))
        return marshal(provider, bss_provider_fields), 201

    def put(self, id=None):
        if not id:
            abort(400, status="error", message='id is required')

        try:
            input_json = request.get_json(force=True, silent=False)
            # TODO validate input
        except BadRequest:
            abort(400, status="error", message='Incorrect json provided')

        try:
            provider = models.BssProvider.find_by_id(id)
            status = 200
        except sqlalchemy.orm.exc.NoResultFound:
            provider = models.BssProvider(id)
            models.db.session.add(provider)
            status = 201

        provider.from_json(input_json)
        try:
            models.db.session.commit()
        except sqlalchemy.exc.IntegrityError as ex:
            abort(400, status="error", message=str(ex))
        return marshal(provider, bss_provider_fields), status

    def delete(self, id=None):
        if not id:
            abort(400, status="error", message='id is required')
        try:
            provider = models.BssProvider.find_by_id(id)
            provider.discarded = True
            models.db.session.commit()
            return None, 204
        except sqlalchemy.orm.exc.NoResultFound:
            abort(404, status="error", message='object not found')


class EquipmentsProvider(flask_restful.Resource):
    @marshal_with(equipment_provider_list_fields)
    def get(self, id=None):
        if id:
            try:
                return {'equipments_providers': [models.EquipmentsProvider.find_by_id(id)]}
            except sqlalchemy.orm.exc.NoResultFound:
                return {'equipments_providers': []}, 404
        else:
            return {'equipments_providers': models.EquipmentsProvider.all()}

    @InputJsonValidator(equipments_provider_format)
    def put(self, id=None):
        """
        Create or update an equipment provider in db
        """
        input_json = request.get_json(force=True, silent=False)
        try:
            provider = models.EquipmentsProvider.find_by_id(id)
            status = 200
        except sqlalchemy.orm.exc.NoResultFound:
            provider = models.EquipmentsProvider(id)
            models.db.session.add(provider)
            status = 201

        provider.from_json(input_json)
        try:
            models.db.session.commit()
        except sqlalchemy.exc.IntegrityError as ex:
            abort(400, status="error", message=str(ex))
        return {'equipments_provider': [marshal(provider, equipment_provider_fields)]}, status

    def delete(self, id=None):
        """
        Delete an equipment provider in db, i.e. set parameter DISCARDED to TRUE
        """
        if not id:
            abort(400, status="error", message='id is required')
        try:
            provider = models.EquipmentsProvider.find_by_id(id)
            provider.discarded = True
            models.db.session.commit()
            return None, 204
        except sqlalchemy.orm.exc.NoResultFound:
            abort(404, status="error", message='object not found')


class StreetNetworkBackend(flask_restful.Resource):
    def _get_backend_by_id(self, backend_id):
        return models.StreetNetworkBackend.query.get_or_404(backend_id)

    def _get_all_backends(self):
        parser = reqparse.RequestParser()
        parser.add_argument('page', type=int, required=False, default=1)
        args = parser.parse_args()

        return models.StreetNetworkBackend.query.filter_by(discarded=False).paginate(
            args['page'], current_app.config.get('MAX_ITEMS_PER_PAGE', 5)
        )

    def get(self, backend_id=None):
        resp = None
        if backend_id:
            resp = marshal(self._get_backend_by_id(backend_id), streetnetwork_backend_fields)
        else:
            pagination = self._get_all_backends()
            pagination_json = {
                'current_page': pagination.page,
                'items_per_page': pagination.per_page,
                'total_items': pagination.total,
            }

            if pagination.has_next:
                pagination_json['next'] = url_for(request.endpoint, page=pagination.next_num)
            return {
                'streetnetwork_backends': marshal(pagination.items, streetnetwork_backend_fields),
                'pagination': pagination_json,
            }

        return {'streetnetwork_backends': resp}

    def _validate_input(self, json_data):
        """
        Check that the received data contains all required info
        :param json_data: data received in request
        """
        try:
            validate(json_data, streetnetwork_backend_format)
        except ValidationError as e:
            abort(400, status="invalid data", message='{}'.format(parse_error(e)))

    def post(self, backend_id=None):
        """
        Create a streetnetwork backend in db
        """
        if not backend_id:
            abort(400, status="error", message='backend_id is required')

        try:
            input_json = request.get_json(force=True, silent=False)
        except BadRequest:
            abort(400, status="error", message='Incorrect json provided')

        self._validate_input(input_json)
        sn_backend = models.StreetNetworkBackend(backend_id, input_json)
        try:
            models.db.session.add(sn_backend)
            models.db.session.commit()
        except sqlalchemy.exc.IntegrityError as ex:
            abort(400, status="error", message=str(ex))

        resp = marshal(sn_backend, streetnetwork_backend_fields)
        return {'streetnetwork_backend': resp}, 201

    def put(self, backend_id=None):
        """
        Create or update a streetnetwork backend in db
        """
        if not backend_id:
            abort(400, status="error", message='backend_id is required')

        try:
            input_json = request.get_json(force=True, silent=False)
        except BadRequest:
            abort(400, status="error", message='Incorrect json provided')

        self._validate_input(input_json)
        try:
            sn_backend = models.StreetNetworkBackend.find_by_id(backend_id)
            status = 200
        except sqlalchemy.orm.exc.NoResultFound:
            sn_backend = models.StreetNetworkBackend(backend_id)
            models.db.session.add(sn_backend)
            status = 201

        sn_backend.from_json(input_json)
        try:
            models.db.session.commit()
        except sqlalchemy.exc.IntegrityError as ex:
            abort(400, status="error", message=str(ex))

        resp = marshal(sn_backend, streetnetwork_backend_fields)
        return {'streetnetwork_backend': resp}, status

    def delete(self, backend_id=None):
        """
        Delete an streetnetwork backend in db, i.e. set parameter DISCARDED to TRUE
        """
        if not backend_id:
            abort(400, status="error", message='backend_id is required')
        try:
            sn_backend = models.StreetNetworkBackend.find_by_id(backend_id)
            sn_backend.discarded = True
            models.db.session.commit()
            return None, 204
        except sqlalchemy.orm.exc.NoResultFound:
            abort(404, status="error", message='object not found')
