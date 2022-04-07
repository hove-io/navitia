# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.hove.com).
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

from __future__ import absolute_import
import navitiacommon.type_pb2 as type_pb2

odt_levels = {"scheduled", "with_stops", "zonal", "all"}

pb_odt_level = {
    'scheduled': type_pb2.scheduled,
    'with_stops': type_pb2.with_stops,
    'zonal': type_pb2.zonal,
    'all': type_pb2.all,
}

# When an emtpy string or 'none' is passed, it deactivates all
add_poi_infos_types = ('bss_stands', 'car_park', '', 'none')


def handle_poi_infos(add_poi_info_param, bss_stands_param):
    """
    Check if info about bss stands and/or car parks is needed
    :param add_poi_info_param: parameter 'add_poi_infos[]' of the query
    :param bss_stands_param: parameter 'bss_stands' of the query (deprecated but still handled for retrocompatibility)
    :return: True if info is needed, False otherwise
    """
    if bss_stands_param and "bss_stands" not in add_poi_info_param:
        add_poi_info_param.append("bss_stands")

    return any(value in add_poi_info_param for value in ['bss_stands', 'car_park'])


def split_uri(uri):
    """
    Split uri into an output list
    :param uri: concatenated uri params
    :return: the splitted uri list
    """
    if not uri:
        return []
    return uri.strip('/').split('/')
