# Copyright (c) 2001-2017, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
# the software to build cool stuff with public transport.
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
from __future__ import absolute_import, print_function, unicode_literals, division
from navitiacommon import type_pb2


def make_pt_object(embedded_type, lon, lat, uri=None):
    pt_object = type_pb2.PtObject()
    pt_object.embedded_type = embedded_type
    if uri:
        pt_object.uri = uri
    if embedded_type == type_pb2.STOP_POINT:
        pt_object.stop_point.coord.lat = lat
        pt_object.stop_point.coord.lon = lon
    elif embedded_type == type_pb2.STOP_AREA:
        pt_object.stop_area.coord.lat = lat
        pt_object.stop_area.coord.lon = lon
    elif embedded_type == type_pb2.ADDRESS:
        pt_object.address.coord.lat = lat
        pt_object.address.coord.lon = lon
    elif embedded_type == type_pb2.ADMINISTRATIVE_REGION:
        pt_object.administrative_region.coord.lat = lat
        pt_object.administrative_region.coord.lon = lon
    elif embedded_type == type_pb2.POI:
        pt_object.poi.coord.lat = lat
        pt_object.poi.coord.lon = lon
    return pt_object
