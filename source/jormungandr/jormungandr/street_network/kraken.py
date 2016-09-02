#  Copyright (c) 2001-2016, Canal TP and/or its affiliates. All rights reserved.
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

from __future__ import absolute_import, print_function, unicode_literals, division
from navitiacommon import request_pb2, response_pb2, type_pb2
import logging


class Kraken(object):

    def __init__(self, instance, url,
                 timeout=10, language='fr-FR',
                 api_key=None, **kwargs):
        self.instance = instance

    def direct_path(self, mode, origin, destination, datetime, clockwise):
        req = request_pb2.Request()
        req.requested_api = type_pb2.direct_path
        req.direct_path.origin.place = origin
        req.direct_path.origin.access_duration = 0
        req.direct_path.destination.place = destination
        req.direct_path.destination.access_duration = 0
        req.direct_path.datetime = datetime
        req.direct_path.clockwise = clockwise
        req.direct_path.streetnetwork_params.origin_mode = mode
        req.direct_path.streetnetwork_params.destination_mode = mode
        req.direct_path.streetnetwork_params.walking_speed = self.instance.walking_speed
        req.direct_path.streetnetwork_params.max_walking_duration_to_pt = self.instance.max_walking_duration_to_pt
        req.direct_path.streetnetwork_params.bike_speed = self.instance.bike_speed
        req.direct_path.streetnetwork_params.max_bike_duration_to_pt = self.instance.max_bike_duration_to_pt
        req.direct_path.streetnetwork_params.bss_speed = self.instance.bss_speed
        req.direct_path.streetnetwork_params.max_bss_duration_to_pt = self.instance.max_bss_duration_to_pt
        req.direct_path.streetnetwork_params.car_speed = self.instance.car_speed
        req.direct_path.streetnetwork_params.max_car_duration_to_pt = self.instance.max_car_duration_to_pt
        return self.instance.send_and_receive(req)
