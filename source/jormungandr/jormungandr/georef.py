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

from __future__ import absolute_import, print_function
from navitiacommon import request_pb2, type_pb2

class Kraken(object):

    def __init__(self, instance):
        self.instance = instance

    def get_stop_points(self, place, mode, max_duration, reverse=False):
        req = request_pb2.Request()
        req.requested_api = type_pb2.nearest_stop_points
        req.nearest_stop_points.place = place
        req.nearest_stop_points.mode = mode
        req.nearest_stop_points.reverse = reverse
        req.nearest_stop_points.max_duration = max_duration

        req.nearest_stop_points.walking_speed = self.instance.walking_speed
        req.nearest_stop_points.bike_speed = self.instance.bike_speed
        req.nearest_stop_points.bss_speed = self.instance.bss_speed
        req.nearest_stop_points.car_speed = self.instance.car_speed

        result = self.instance.send_and_receive(req)
        nsp = {}
        for item in result.nearest_stop_points:
            nsp[item.stop_point.uri] = item.access_duration
        return nsp

    def place(self, place):
        req = request_pb2.Request()
        req.requested_api = type_pb2.place_uri
        req.place_uri.uri = place
        response = self.instance.send_and_receive(req)
        return response.places[0]


