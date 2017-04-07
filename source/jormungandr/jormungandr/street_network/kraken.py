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
import logging
import copy
from jormungandr.exceptions import TechnicalError
from navitiacommon import request_pb2, type_pb2
from jormungandr.utils import get_uri_pt_object
from jormungandr.street_network.street_network import AbstractStreetNetworkService, StreetNetworkPathType, \
    StreetNetworkPathKey
from jormungandr import utils

class Kraken(AbstractStreetNetworkService):
    sn_system_id = 'kraken'

    def __init__(self, instance, service_url, timeout=10, api_key=None, **kwargs):
        self.instance = instance

    def _reverse_journeys(self, response):
        if not getattr(response, "journeys"):
            return response
        for j in response.journeys:
            if not getattr(j, "sections"):
                continue
            previous_section_begin = j.arrival_date_time
            for s in j.sections:
                o = copy.deepcopy(s.origin)
                d = copy.deepcopy(s.destination)
                s.origin.CopyFrom(d)
                s.destination.CopyFrom(o)
                s.end_date_time = previous_section_begin
                previous_section_begin = s.begin_date_time = s.end_date_time - s.duration
            j.sections.sort(utils.SectionSorter())
        return response

    def direct_path(self, mode, pt_object_origin, pt_object_destination, fallback_extremity, request, direct_path_type):
        """
        :param direct_path_type: we need to "invert" a direct path when it's a ending fallback by car if and only if
                                 it's returned by kraken. In other case, it's ignored
        """
        should_invert_journey = (mode == 'car' and direct_path_type == StreetNetworkPathType.ENDING_FALLBACK)
        if should_invert_journey:
            pt_object_origin, pt_object_destination = pt_object_destination, pt_object_origin

        req = request_pb2.Request()
        req.requested_api = type_pb2.direct_path
        req.direct_path.origin.place = get_uri_pt_object(pt_object_origin)
        req.direct_path.origin.access_duration = 0
        req.direct_path.destination.place = get_uri_pt_object(pt_object_destination)
        req.direct_path.destination.access_duration = 0
        req.direct_path.datetime = fallback_extremity.datetime
        req.direct_path.clockwise = fallback_extremity.represents_start
        req.direct_path.streetnetwork_params.origin_mode = mode
        req.direct_path.streetnetwork_params.destination_mode = mode
        req.direct_path.streetnetwork_params.walking_speed = request['walking_speed']
        req.direct_path.streetnetwork_params.max_walking_duration_to_pt = request['max_walking_duration_to_pt']
        req.direct_path.streetnetwork_params.bike_speed = request['bike_speed']
        req.direct_path.streetnetwork_params.max_bike_duration_to_pt = request['max_bike_duration_to_pt']
        req.direct_path.streetnetwork_params.bss_speed = request['bss_speed']
        req.direct_path.streetnetwork_params.max_bss_duration_to_pt = request['max_bss_duration_to_pt']
        req.direct_path.streetnetwork_params.car_speed = request['car_speed']
        req.direct_path.streetnetwork_params.max_car_duration_to_pt = request['max_car_duration_to_pt']

        response = self.instance.send_and_receive(req)

        if should_invert_journey:
            return self._reverse_journeys(response)
        return response


    def get_street_network_routing_matrix(self, origins, destinations, street_network_mode, max_duration, request, **kwargs):
        # TODO: reverse is not handled as so far
        speed_switcher = {
            "walking": request['walking_speed'],
            "bike": request['bike_speed'],
            "car": request['car_speed'],
            "bss": request['bss_speed'],
        }
        req = request_pb2.Request()
        req.requested_api = type_pb2.street_network_routing_matrix

        #kraken can only manage 1-n request, so we reverse request if needed
        if len(origins) > 1:
            if len(destinations) > 1:
                logging.getLogger(__name__).error('routing matrix error, no unique center point')
                raise TechnicalError('routing matrix error, no unique center point')
            else:
                origins, destinations = destinations, origins

        for o in origins:
            orig = req.sn_routing_matrix.origins.add()
            orig.place = get_uri_pt_object(o)
            orig.access_duration = 0
        for d in destinations:
            dest = req.sn_routing_matrix.destinations.add()
            dest.place = get_uri_pt_object(d)
            dest.access_duration = 0

        req.sn_routing_matrix.mode = street_network_mode
        req.sn_routing_matrix.speed = speed_switcher.get(street_network_mode, kwargs.get("walking"))
        req.sn_routing_matrix.max_duration = max_duration

        res = self.instance.send_and_receive(req)
        if res.HasField('error'):
            logging.getLogger(__name__).error('routing matrix query error {}'.format(res.error))
            raise TechnicalError('routing matrix fail')
        return res.sn_routing_matrix

    def make_path_key(self, mode, orig_uri, dest_uri, streetnetwork_path_type, period_extremity):
        """
        :param orig_uri, dest_uri, mode: matters obviously
        :param streetnetwork_path_type: whether it's a fallback at
        the beginning, the end of journey or a direct path without PT also matters especially for car (to know if we
        park before or after)
        :param period_extremity: is a PeriodExtremity (a datetime and it's meaning on the
        fallback period)
        Nota: period_extremity is not taken into consideration so far because we assume that a
        direct path from A to B remains the same even the departure time are different (no realtime)
        """
        return StreetNetworkPathKey(mode, orig_uri, dest_uri, streetnetwork_path_type, None)
