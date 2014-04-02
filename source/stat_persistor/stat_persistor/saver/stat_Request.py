# encoding: utf-8
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
    journey_table = meta.tables['stat.journeys']
    journey_section_table = meta.tables['stat.journey_sections']

    #Inserer dans la table stat.requests
    query = request_table.insert()
    conn.execute(query.values(build_stat_request_dict(stat)))


    #Inserer dans la tables stat.coverages
    query = coverage_table.insert()
    for coverage in stat.coverages:
        conn.execute(query.values(
            build_stat_coverage_dict(coverage)))

    #Inserer dans la table stat.parameters
    query = parameter_table.insert()

    for param in stat.parameters:
        conn.execute(query.values(
            build_stat_parameter_dict(param)))

    #Inserer les journeys dans la table stat.journeys
    query = journey_table.insert()
    for journey in stat.journeys:
        conn.execute(query.values(build_stat_journey_dict(journey)))

        #Inserer les sections de la journey dans la table stat.journey_sections:
        query = journey_section_table.insert()
        for section in journey.sections:
            conn.execute(query.values(build_stat_section_dict(section)))

def build_stat_request_dict(stat):
    """
    Construit à partir d'un object protobuf pbnavitia.stat.HitStat
    Utilisé pour l'insertion dans la table stat.requests
    """
    result = {}
    result['request_date'] = from_timestamp(stat.request_date).strftime('%Y%m%d %H:%M:%S')
    result['year'] = from_timestamp(stat.request_date).year
    result['month']= from_timestamp(stat.request_date).month
    result['day']= from_timestamp(stat.request_date).day
    result['hour']= from_timestamp(stat.request_date).hour
    result['minute']= from_timestamp(stat.request_date).minute
    result['user_id'] = stat.user_id
    result['user_name'] = stat.user_name
    result['app_id'] = stat.application_id
    result['app_name'] = stat.application_name
    result['request_duration'] = stat.request_duration
    result['api'] = stat.api
    result['query_string'] = stat.query_string
    result['host'] = stat.host
    result['client']= stat.client
    result['response_size'] = stat.response_size
    return result

def build_stat_parameter_dict(param):
    """
    Construit à partir d'un object protobuf pbnavitia.stat.HitStat.parameters
    Utilisé pour l'insertion dans la table stat.parameters
    """
    result = {}
    result['param_key'] = param.key
    result['param_value'] = param.value
    return result

def build_stat_coverage_dict(coverage):
    """
    Construit à partir d'un object protobuf pbnavitia.stat.HitStat.coverages
    Utilisé pour l'insertion dans la table stat.coverages
    """
    result = {}
    result['region_id'] = coverage.region_id
    return result

def build_stat_journey_dict(journey):
    """
    Construit à partir d'un object protobuf pbnavitia.stat.HitStat.journeys
    Utilisé pour l'insertion dans la table stat.journeys
    """
    result = {}
    result['requested_date_time'] = from_timestamp(journey.requested_date_time).strftime('%Y%m%d %H:%M:%S')
    result['departure_date_time'] = from_timestamp(journey.departure_date_time).strftime('%Y%m%d %H:%M:%S')
    result['arrival_date_time'] = from_timestamp(journey.arrival_date_time).strftime('%Y%m%d %H:%M:%S')
    result['duration'] = journey.duration
    result['nb_transfers'] = journey.nb_transfers
    result['type'] = journey.type
    return result

def build_stat_section_dict(section):
    """
    Construit à partir d'un object protobuf pbnavitia.stat.HitStat.journey.sections
    Utilisé pour l'insertion dans la table stat.sections
    """
    result = {}
    result['departure_date_time'] = from_timestamp(section.departure_date_time).strftime('%Y%m%d %H:%M:%S')
    result['arrival_date_time'] = from_timestamp(section.arrival_date_time).strftime('%Y%m%d %H:%M:%S')
    result['duration'] = section.duration
    result['type'] = section.type
    result['from_embedded_type'] = section.from_embedded_type
    result['from_id'] = section.from_id
    from_point = "POINT(%.20f %.20f)" %(section.from_coord.lon, section.from_coord.lat)
    result['from_coord'] = func.ST_GeomFromtext(from_point, 4326)
    result['to_embedded_type'] = section.to_embedded_type
    result['to_id'] = section.to_id
    to_point = "POINT(%.20f %.20f)" %(section.to_coord.lon, section.to_coord.lat)
    result['to_coord'] = func.ST_GeomFromtext(to_point, 4326)
    result['vehicle_journey_id'] = section.vehicle_journey_id
    result['line_id'] = section.line_id
    result['route_id'] = section.route_id
    result['network_id'] = section.network_id
    result['commercial_mode_id'] = section.commercial_mode_id
    result['physical_mode_id'] = section.physical_mode_id
    return result




