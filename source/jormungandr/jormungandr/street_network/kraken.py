#  Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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

from __future__ import absolute_import, print_function, unicode_literals, division
import logging
import copy

import jormungandr.street_network.utils
from jormungandr.exceptions import TechnicalError
from navitiacommon import request_pb2, type_pb2, response_pb2
from jormungandr.street_network.street_network import (
    AbstractStreetNetworkService,
    StreetNetworkPathType,
    StreetNetworkPathKey,
)
from jormungandr import utils
import six
from functools import cmp_to_key
from jormungandr.street_network.utils import (
    crowfly_distance_between,
    create_kraken_direct_path_request,
    create_kraken_matrix_request,
)
from jormungandr.fallback_modes import FallbackModes


class Kraken(AbstractStreetNetworkService):
    def __init__(self, instance, service_url, modes=None, id=None, timeout=10, api_key=None, **kwargs):
        self.instance = instance
        self.modes = modes or []
        self.sn_system_id = id or 'kraken'

    def status(self):
        return {'id': six.text_type(self.sn_system_id), 'class': self.__class__.__name__, 'modes': self.modes}

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
            j.sections.sort(key=cmp_to_key(utils.SectionSorter()))
        return response

    def _direct_path(
        self,
        instance,
        mode,
        pt_object_origin,
        pt_object_destination,
        fallback_extremity,
        request,
        direct_path_type,
        request_id,
    ):
        """
        :param direct_path_type: we need to "invert" a direct path when it's a ending fallback by car if and only if
                                 it's returned by kraken. In other case, it's ignored
        """
        should_invert_journey = mode == 'car' and direct_path_type == StreetNetworkPathType.ENDING_FALLBACK
        if should_invert_journey:
            pt_object_origin, pt_object_destination = pt_object_destination, pt_object_origin

        direct_path_request = {}
        for attr in [
            'walking_speed',
            'max_walking_duration_to_pt',
            'bike_speed',
            'max_bike_duration_to_pt',
            'bss_speed',
            'max_bss_duration_to_pt',
            'car_speed',
            'max_car_duration_to_pt',
            'car_no_park_speed',
            'max_car_no_park_duration_to_pt',
            'taxi_speed',
            'max_taxi_duration_to_pt',
            'ridesharing_speed',
            'max_ridesharing_duration_to_pt',
            'bss_rent_duration',
            'bss_rent_penalty',
            'bss_return_duration',
            'bss_return_penalty',
            '_enable_instructions',
        ]:
            direct_path_request[attr] = request[attr]

        if direct_path_type == StreetNetworkPathType.DIRECT:
            # in distributed scenario, we allow the street network calculator to compute a very long direct path
            # in case of Kraken, the stop condition of direct path in Kraken is defined as
            # max_{mode}_duration_to_pt * 2.
            # When it comes to a direct path request, we override this parameter with
            # max_{mode}_direct_path_duration / 2.0
            from jormungandr.fallback_modes import FallbackModes as fm

            direct_path_request['max_{mode}_duration_to_pt'.format(mode=mode)] = int(
                request['max_{mode}_direct_path_duration'.format(mode=mode)] / 2
            )
            # if the crowfly distance between origin and destination is too large, there is no need to call kraken
            crowfly_distance = crowfly_distance_between(
                utils.get_pt_object_coord(pt_object_origin), utils.get_pt_object_coord(pt_object_destination)
            )
            # if the crowfly distance between origin and destination is
            # bigger than max_{mode}_direct_path_distance don't compute direct_path
            if crowfly_distance > int(request['max_{mode}_direct_path_distance'.format(mode=mode)]):
                return response_pb2.Response()

            if (
                crowfly_distance / float(direct_path_request['{mode}_speed'.format(mode=mode)])
                > request['max_{mode}_direct_path_duration'.format(mode=mode)]
            ):
                return response_pb2.Response()

        req = self._create_direct_path_request(
            mode, pt_object_origin, pt_object_destination, fallback_extremity, direct_path_request, direct_path_type
        )

        response = instance.send_and_receive(req, request_id=request_id)
        if should_invert_journey:
            response = self._reverse_journeys(response)

        def has_bss_rent_before_put_back_section(journey):
            # Here is a little trick with python's generator
            # the next 3 lines check not only the existences of BSS_RENT and BSS_PUT_BACK, but also check the fact that
            # BSS_RENT must be located Before BSS_PUT_BACK
            sections = (s for s in journey.sections)
            bss_rent = next((True for s in sections if s.type == response_pb2.BSS_RENT), False)
            bss_put_back = next((True for s in sections if s.type == response_pb2.BSS_PUT_BACK), False)
            return bss_rent and bss_put_back

        if response.journeys:
            # Note that: the journey returned by Kraken is a direct path. A direct path of walking/bike/car/car_no_park/
            # contains only one section. But for bss, there may be one or five sections.
            # For bss,we only need attribute the mode to the first section. The most significant mode will be chosen
            # later
            if has_bss_rent_before_put_back_section(response.journeys[0]):
                return response

            if mode == FallbackModes.bss.name and (
                not has_bss_rent_before_put_back_section(response.journeys[0])
            ):
                response.journeys[0].sections[0].street_network.mode = FallbackModes.walking.value
            else:
                response.journeys[0].sections[0].street_network.mode = FallbackModes[mode].value
        return response

    @staticmethod
    def handle_car_no_park_modes(mode):
        if mode in (FallbackModes.ridesharing.name, FallbackModes.taxi.name, FallbackModes.car.name):
            return FallbackModes.car_no_park.name
        return mode

    def _create_direct_path_request(
        self, mode, pt_object_origin, pt_object_destination, fallback_extremity, request, direct_path_type, language="en-US"
    ):
        return create_kraken_direct_path_request(
            self, mode, pt_object_origin, pt_object_destination, fallback_extremity, request, language
        )

    def get_uri_pt_object(self, pt_object):
        return utils.get_uri_pt_object(pt_object)

    def _get_street_network_routing_matrix(
        self, instance, origins, destinations, street_network_mode, max_duration, request, request_id, **kwargs
    ):
        # TODO: reverse is not handled as so far
        speed_switcher = jormungandr.street_network.utils.make_speed_switcher(request)

        # kraken can only manage 1-n request, so we reverse request if needed
        if len(origins) > 1:
            if len(destinations) > 1:
                logging.getLogger(__name__).error('routing matrix error, no unique center point')
                raise TechnicalError('routing matrix error, no unique center point')
            else:
                origins, destinations = destinations, origins

        req = self._create_sn_routing_matrix_request(
            origins, destinations, street_network_mode, max_duration, speed_switcher, request, **kwargs
        )

        res = instance.send_and_receive(req, request_id=request_id)
        self._check_for_error_and_raise(res)

        return res.sn_routing_matrix

    def make_location(self, obj):
        return type_pb2.LocationContext(place=self.get_uri_pt_object(obj), access_duration=0)

    def _create_sn_routing_matrix_request(
        self, origins, destinations, street_network_mode, max_duration, speed_switcher, request, **kwargs
    ):

        return create_kraken_matrix_request(
            self, origins, destinations, street_network_mode, max_duration, speed_switcher, request
        )

    def _check_for_error_and_raise(self, res):
        if res is None or res.HasField('error'):
            logging.getLogger(__name__).error(
                'routing matrix query error {}'.format(res.error if res else "Unknown")
            )
            raise TechnicalError('routing matrix fail')

    def make_path_key(self, mode, orig_uri, dest_uri, streetnetwork_path_type, period_extremity):
        """
        :param orig_uri, dest_uri, mode: matters obviously
        :param streetnetwork_path_type: whether it's a fallback at
        the beginning, the end of journey or a direct path without PT also matters especially for car (to know if we
        park before or after)
        :param period_extremity: is a PeriodExtremity (a datetime and its meaning on the
        fallback period)
        Nota: period_extremity is not taken into consideration so far because we assume that a
        direct path from A to B remains the same even the departure time are different (no realtime)
        """
        return StreetNetworkPathKey(mode, orig_uri, dest_uri, streetnetwork_path_type, None)
