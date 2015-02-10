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

import logging
from stat_persistor.saver.utils import from_timestamp, from_time, FunctionalError
import datetime
from stat_persistor.config import Config
from sqlalchemy.sql import func


def persist_stat_request(meta, conn, stat):
    """
    Inserer les statistiques pour chaque requete:
    stat.request, stat.parameters, stat.journeys, stat.journey_sections
    """
    request_table = meta.tables['stat.requests']
    coverage_table = meta.tables['stat.coverages']
    parameter_table = meta.tables['stat.parameters']
    error_table = meta.tables['stat.errors']
    journey_table = meta.tables['stat.journeys']
    journey_section_table = meta.tables['stat.journey_sections']
    interpreted_parameters_table = meta.tables['stat.interpreted_parameters']
    journey_request_table = meta.tables['stat.journey_request']
    filter_table = meta.tables['stat.filter']

    #stat.requests
    query = request_table.insert()
    request_id = conn.execute(query.values(build_stat_request_dict(stat)))

    #stat.coverages
    query = coverage_table.insert()
    for coverage in stat.coverages:
        conn.execute(query.values(
            build_stat_coverage_dict(coverage, request_id.inserted_primary_key[0])))

    #stat.parameters
    query = parameter_table.insert()
    for param in stat.parameters:
        conn.execute(query.values(
            build_stat_parameter_dict(param, request_id.inserted_primary_key[0])))

    #stat.interpreted_parameters
    query = interpreted_parameters_table.insert()
    query_filter = filter_table.insert()
    for param in stat.interpreted_parameters:
        res = conn.execute(query.values(
            build_stat_parameter_dict(param, request_id.inserted_primary_key[0])))
        for filter in param.filters:
            conn.execute(query_filter.values(
                build_filter_dict(filter, res.inserted_primary_key[0])))


    #stat.errors
    query = error_table.insert()
    if stat.error.id:
        conn.execute(query.values(
            build_stat_error_dict(stat.error, request_id.inserted_primary_key[0])))

    #stat.journey_request
    query = journey_request_table.insert()
    if stat.journey_request.IsInitialized():
        conn.execute(query.values(
            build_journey_request_dict(stat.journey_request, request_id.inserted_primary_key[0])))

    #stat.journeys
    for journey in stat.journeys:
        query = journey_table.insert()
        journey_id = conn.execute(
            query.values(build_stat_journey_dict(journey,
                                                 request_id.inserted_primary_key[0])))

        #stat.journey_sections:
        query = journey_section_table.insert()
        for section in journey.sections:
            conn.execute(query.values(build_stat_section_dict(section,
                                                              request_id.inserted_primary_key[0],
                                                              journey_id.inserted_primary_key[0])))

def build_stat_request_dict(stat):
    """
    Construit à partir d'un object protobuf pbnavitia.stat.HitStat
    Utilisé pour l'insertion dans la table stat.requests
    """
    return{
        'request_date': get_datetime_from_timestamp(stat.request_date),
        'user_id': stat.user_id,
        'user_name': stat.user_name,
        'app_id': stat.application_id,
        'app_name': stat.application_name,
        'request_duration': stat.request_duration,
        'api': stat.api,
        'host': stat.host,
        'client': stat.client,
        'response_size': stat.response_size
    }

def build_journey_request_dict(journey_request, request_id):
    """
    Construit à partir d'un object protobuf pbnavitia.stat.HitStat
    Utilisé pour l'insertion dans la table stat.requests
    """
    return{
        'requested_date_time': get_datetime_from_timestamp(journey_request.requested_date_time),
        'clockwise': journey_request.clockwise,
        'departure_insee': journey_request.departure_insee,
        'departure_admin': journey_request.departure_admin,
        'arrival_insee': journey_request.arrival_insee,
        'arrival_admin': journey_request.arrival_admin,
        'request_id': request_id
    }

def build_filter_dict(filter, interpreted_param_id):
    return{
        'object': filter.object,
        'attribute': filter.attribute,
        'operator': filter.operator,
        'value': filter.value,
        'interpreted_parameter_id': interpreted_param_id,
    }


def build_stat_parameter_dict(param, request_id):
    """
    Construit à partir d'un object protobuf pbnavitia.stat.HitStat.parameters
    Utilisé pour l'insertion dans la table stat.parameters
    """
    return {
        'request_id': request_id,
        'param_key': param.key,
        'param_value': param.value
    }


def build_stat_error_dict(error, request_id):
    """
    Construit à partir d'un object protobuf pbnavitia.stat.HitStat.error
    Utilisé pour l'insertion dans la table stat.errors
    """
    return {
        'request_id': request_id,
        'id': error.id,
        'message': error.message
    }


def build_stat_coverage_dict(coverage, request_id):
    """
    Construit à partir d'un object protobuf pbnavitia.stat.HitStat.coverages
    Utilisé pour l'insertion dans la table stat.coverages
    """
    return {
        'region_id': coverage.region_id,
        'request_id': request_id
    }

def get_datetime_from_timestamp(date_time):
    try:
        request_date_time = from_timestamp(date_time)
    except ValueError as e:
        logging.getLogger("Unable to parse date: %s", str(e))
        request_date_time = 0

    return request_date_time.strftime('%Y%m%d %H:%M:%S')

def build_stat_journey_dict(journey, request_id):
    """
    Construit à partir d'un object protobuf pbnavitia.stat.HitStat.journeys
    Utilisé pour l'insertion dans la table stat.journeys
    """
    return{
        'request_id': request_id,
        'requested_date_time': get_datetime_from_timestamp(journey.requested_date_time),
        'departure_date_time': get_datetime_from_timestamp(journey.departure_date_time),
        'arrival_date_time': get_datetime_from_timestamp(journey.arrival_date_time),
        'duration': journey.duration,
        'nb_transfers': journey.nb_transfers,
        'type': journey.type
    }

def build_stat_section_dict(section, request_id, journey_id):
    """
    Construit à partir d'un object protobuf pbnavitia.stat.HitStat.journey.sections
    Utilisé pour l'insertion dans la table stat.sections
    """
    from_point = "POINT({0:.20f} {1:.20f})".format(section.from_coord.lon, section.from_coord.lat)
    to_point = "POINT({0:.20f} {1:.20f})".format(section.to_coord.lon, section.to_coord.lat)

    return{
        'request_id': request_id,
        'journey_id': journey_id,
        'departure_date_time': get_datetime_from_timestamp(section.departure_date_time),
        'arrival_date_time': get_datetime_from_timestamp(section.arrival_date_time),
        'duration': section.duration,
        'mode': section.mode,
        'type': section.type,
        'from_embedded_type': section.from_embedded_type,
        'from_id': section.from_id,
        'from_name': section.from_name,
        'from_coord': func.ST_GeomFromtext(from_point, 4326),
        'from_admin_id': section.from_admin_id,
        'from_admin_name': section.from_admin_name,
        'to_embedded_type': section.to_embedded_type,
        'to_id': section.to_id,
        'to_name': section.to_name,
        'to_coord': func.ST_GeomFromtext(to_point, 4326),
        'to_admin_id': section.to_admin_id,
        'to_admin_name': section.to_admin_name,
        'vehicle_journey_id': section.vehicle_journey_id,
        'line_id': section.line_id,
        'line_code': section.line_code,
        'route_id': section.route_id,
        'network_id': section.network_id,
        'network_name': section.network_name,
        'commercial_mode_id': section.commercial_mode_id,
        'commercial_mode_name': section.commercial_mode_name,
        'physical_mode_id': section.physical_mode_id,
        'physical_mode_name': section.physical_mode_name
    }
