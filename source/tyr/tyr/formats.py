# Copyright (c) 2001-2016, Canal TP and/or its affiliates. All rights reserved.
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

def parse_error(error):
    res = None
    try:
        res = error.message
    except AttributeError:
        res = str(error).replace("\n", " ")
    return res.decode('utf-8')

poi_type_format = {
    'type': 'object',
    'properties': {
        'id': {'type': 'string'},
        'name': {'type': 'string'}
    },
    'required': ['id', 'name']
}

key_value_format = {
    'type': 'object',
    'properties': {
        'key': {'type': 'string'},
        'value': {'type': 'string'}
    },
    'required': ['key', 'value']
}

rule_format = {
    'type': 'object',
    'properties': {
        'osm_tags_filters': {
            'type': 'array',
            'items': key_value_format},
        'poi_type_id': {'type': 'string'}
    },
    'required': ['osm_tags_filters', 'poi_type_id']
}

poi_type_conf_format = {
    'type': 'object',
    'properties': {
        'poi_types': {
            'type': 'array',
            'items': poi_type_format,
        },
        'rules': {
            'type': 'array',
            'items': rule_format
        }
    },
    'required': ['poi_types', 'rules']
}
