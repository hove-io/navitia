# encoding: utf-8
from stat_persistor.saver.utils import from_timestamp, from_time, FunctionalError
import datetime
from stat_persistor.config import Config


def build_stat_request(stat):
    out_str = ''
    out_str += build_stat_request_dict(stat)
    out_str += build_stat_parameter_dict(stat)
    out_str += build_stat_journeys_dict(stat)

    return out_str

def build_stat_request_dict(stat):
    """
    construit à partir d'un object protobuf pbnavitia.stat.HitStat
    le dictionnaire utilisé pour l'insertion dans un fichier
    """
    out_str = ''
    out_str += from_timestamp(stat.request_date).strftime('%Y%m%d %H:%M:%S')
    out_str += ';' + str(stat.user_id)
    out_str += ';' + stat.user_name
    out_str += ';' + str(stat.application_id)
    out_str += ';' + stat.application_name
    out_str += ';' + str(stat.request_duration)
    out_str += ';' + stat.api
    out_str += ';' + stat.query_string
    out_str += ';' + stat.host
    out_str += ';' + stat.client
    out_str += ';' + str(stat.response_size)
    out_str += ';' + stat.region_id + '\n'

    return out_str

def build_stat_parameter_dict(stat):
    """
    Construit les paramètres
    """
    out_str = ''
    for param in stat.parameters:
        out_str += param.key + ';' + param.value + '\n'

    return out_str

def build_stat_journeys_dict(stat):
    out_str = ''

    for journey in stat.journeys:
        out_str += build_stat_journey_dict(journey)
        out_str += build_stat_sections_dict(journey.sections)

    return out_str

def build_stat_journey_dict(journey):
    out_str = ''
    out_str += from_timestamp(journey.requested_date_time).strftime('%Y%m%d %H:%M:%S')
    out_str += ';' + from_timestamp(journey.departure_date_time).strftime('%Y%m%d %H:%M:%S')
    out_str += ';' + from_timestamp(journey.arrival_date_time).strftime('%Y%m%d %H:%M:%S')
    out_str += ';' + str(journey.duration)
    out_str += ';' + str(journey.nb_transfers)
    out_str += ';' + journey.type + '\n'

    return out_str

def build_stat_sections_dict(sections):
    out_str = ''
    for section in sections:
        out_str += build_stat_section_dict(section)

    return out_str

def build_stat_section_dict(section):
    out_str = ''
    out_str += from_timestamp(section.departure_date_time).strftime('%Y%m%d %H:%M:%S')
    out_str += ';' + from_timestamp(section.arrival_date_time).strftime('%Y%m%d %H:%M:%S')
    out_str += ';' + str(section.duration)
    out_str += ';' + section.type
    out_str += ';' + section.from_embedded_type
    out_str += ';' + section.from_id
    #out_str += ';' + section.from_coord
    out_str += ';' + section.to_embedded_type
    out_str += ';' + section.to_id
    #out_str += ';' + section.to_coord
    out_str += ';' + section.vehicle_journey_id
    out_str += ';' + section.line_id
    out_str += ';' + section.route_id
    out_str += ';' + section.network_id
    out_str += ';' + section.physical_mode_id
    out_str += ';' + section.commercial_mode_id +'\n'

    return out_str

def persist_stat_request(stat,config):
    """
    enregistre dans un fichier la ligne de statistique
    """
    save_stat_request(stat, config)


def save_stat_request(stat, config):

    stat_result = datetime.datetime.now().strftime('%Y%m%d %H:%M:%S') + ';' + build_stat_request(stat)
    with open(config.stat_file,'a') as stat_file:
        stat_file.write(stat_result)





