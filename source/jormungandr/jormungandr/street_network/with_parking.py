#  Copyright (c) 2001-2016, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.kisio.com).
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

import logging
import itertools
from jormungandr.street_network.street_network import AbstractStreetNetworkService, StreetNetworkPathType
from jormungandr import utils
from jormungandr.utils import get_pt_object_coord
import six

from navitiacommon import response_pb2


class WithParking(AbstractStreetNetworkService):
    def __init__(self, instance, service_url, modes=None, id=None, timeout=10, api_key=None, **kwargs):
        self.instance = instance
        self.modes = modes or []
        self.sn_system_id = id or 'with_parking'
        self.parking_module = utils.create_object(kwargs.get('parking', None))
        config = kwargs.get('street_network', None)
        if 'service_url' not in config['args']:
            config['args'].update({'service_url': None})
        if 'instance' not in config['args']:
            config['args'].update({'instance': instance})

        config['args'].update({'modes': self.modes})
        self.street_network = utils.create_object(config)

    def status(self):
        return {'id': six.text_type(self.sn_system_id), 'class': self.__class__.__name__, 'modes': self.modes}

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
        response = self.street_network._direct_path(
            instance,
            mode,
            pt_object_origin,
            pt_object_destination,
            fallback_extremity,
            request,
            direct_path_type,
            request_id,
        )
        if response and len(response.journeys):
            # Depending of the fallback type(beginning/ending fallback), the parking section's
            # place can be the origin/destination of the pt_journey
            pt_object = (
                pt_object_origin
                if direct_path_type is StreetNetworkPathType.ENDING_FALLBACK
                else pt_object_destination
            )
            self.add_parking_section_in_direct_path(response, pt_object, direct_path_type)

        return response

    def add_parking_section_in_direct_path(self, response, pt_object, direct_path_type):
        logger = logging.getLogger(__name__)
        logger.info("Creating parking section for direct path")

        for journey in response.journeys:
            parking_duration = self.parking_module.get_parking_duration(get_pt_object_coord(pt_object))

            parking_section = response_pb2.Section()

            parking_section.id = 'section_1'
            parking_section.origin.CopyFrom(pt_object)
            parking_section.destination.CopyFrom(pt_object)
            parking_section.duration += parking_duration

            journey.duration += parking_duration
            journey.durations.total += parking_duration

            if direct_path_type == StreetNetworkPathType.ENDING_FALLBACK:
                # And we have to complete the destination of the first section ourselves
                # Because Jormun does not do it afterwards
                journey.sections[0].origin.CopyFrom(pt_object)
                parking_section.type = response_pb2.LEAVE_PARKING
                parking_section.begin_date_time = journey.sections[0].begin_date_time
                parking_section.end_date_time = parking_section.begin_date_time + parking_duration
                # we push off the whole car section
                for s in journey.sections:
                    s.begin_date_time += parking_duration
                    s.end_date_time += parking_duration
            else:
                # And we have to complete the destination of the first section ourselves
                # Because Jormun does not do it afterwards
                journey.sections[-1].destination.CopyFrom(pt_object)
                parking_section.type = response_pb2.PARK
                parking_section.begin_date_time = journey.sections[-1].end_date_time
                parking_section.end_date_time = parking_section.begin_date_time + parking_duration

            journey.arrival_date_time += parking_duration

            journey.sections.extend([parking_section])
            journey.nb_sections += 1

    def _get_street_network_routing_matrix(
        self, instance, origins, destinations, street_network_mode, max_duration, request, request_id, **kwargs
    ):
        response = self.street_network.get_street_network_routing_matrix(
            instance, origins, destinations, street_network_mode, max_duration, request, request_id, **kwargs
        )

        if response and len(response.rows) and len(response.rows[0].routing_response):
            self.add_parking_time_in_routing_matrix(response.rows[0].routing_response, origins, destinations)

        return response

    def add_parking_time_in_routing_matrix(self, response, origins, destinations):
        if len(origins) == 1:
            # The parking time depends on the place we want to park in
            self.add_additional_parking_time(response, destinations)
        else:
            self.add_additional_leave_parking_time(response, origins)

    def _additional_parking_time(self, responses, pt_objs):
        # The response and the destinations/origins related to this response are ordered the same way
        reached = ((r, dest) for r, dest in zip(responses, pt_objs) if r.routing_status == response_pb2.reached)

        # duplicate the generator with tee to avoid copy paste
        it1, it2 = itertools.tee(reached)

        # get coordinates of all reachable stop_point
        coords = (dest.stop_point.coord for _, dest in it1)
        durations = self.parking_module.get_parking_duration_in_batch(coords)
        if not durations:
            durations = [0] * len(pt_objs)

        res = (r for r, _ in it2)
        # add duration to reachable stop_point
        for duration, r in zip(durations, res):
            r.duration += duration

    def add_additional_parking_time(self, responses, destinations):
        self._additional_parking_time(responses, destinations)

    def add_additional_leave_parking_time(self, responses, origins):
        self._additional_parking_time(responses, origins)

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

        return self.street_network.make_path_key(mode, orig_uri, dest_uri, streetnetwork_path_type, None)
