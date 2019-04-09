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
from jormungandr.street_network.street_network import AbstractStreetNetworkService, StreetNetworkPathType
from jormungandr import utils, fallback_modes as fm
from jormungandr.utils import get_pt_object_coord, SectionSorter, pb_del_if


from navitiacommon import response_pb2


class Taxi(AbstractStreetNetworkService):
    def __init__(self, instance, service_url, modes=None, id=None, timeout=10, api_key=None, **kwargs):
        self.instance = instance
        self.modes = modes or [fm.FallbackModes.taxi.name]

        assert list(self.modes) == [fm.FallbackModes.taxi.name], (
            'Class: ' + str(self.__class__) + ' can only be used for ridesharing'
        )

        self.sn_system_id = id or 'taxi'
        config = kwargs.get('street_network', {})
        if 'service_url' not in config['args']:
            config['args'].update({'service_url': None})
        if 'instance' not in config['args']:
            config['args'].update({'instance': instance})

        config['args'].update({'modes': self.modes})
        self.street_network = utils.create_object(config)

    def status(self):
        return {'id': unicode(self.sn_system_id), 'class': self.__class__.__name__, 'modes': self.modes}

    def _direct_path(
        self, mode, pt_object_origin, pt_object_destination, fallback_extremity, request, direct_path_type
    ):

        copy_request = copy.deepcopy(request)
        copy_request["car_no_park_speed"] = copy_request["taxi_speed"]
        response = self.street_network._direct_path(
            mode, pt_object_origin, pt_object_destination, fallback_extremity, copy_request, direct_path_type
        )
        return response

    def _add_additional_section_in_fallback(
        self, response, pt_object_origin, pt_object_destination, request, direct_path_type
    ):
        logger = logging.getLogger(__name__)
        logger.info("Creating additional section for direct path")

        for journey in response.journeys:
            # Depending of the fallback type(beginning/ending fallback), the additional section's
            # place can be the origin/destination of the pt_journey
            if direct_path_type == StreetNetworkPathType.BEGINNING_FALLBACK:
                self._add_additional_section_after_first_section_taxi(
                    journey, pt_object_destination, request["additional_time_after_first_section_taxi"]
                )
            else:
                self._add_additional_section_before_first_section_taxi(
                    journey, pt_object_origin, request["additional_time_before_last_section_taxi"]
                )

    def _add_additional_section_after_first_section_taxi(self, journey, pt_object, additional_time):
        logging.getLogger(__name__).info("Creating additional section_after_first_section_taxi")

        additional_section = self._create_additional_section(
            pt_object, additional_time, journey.sections[-1].end_date_time, "section_1"
        )

        # We have to complete the destination of the first section ourselves
        # Because Jormun does not do it afterwards
        journey.sections[-1].destination.CopyFrom(pt_object)

        self._update_journey(journey, additional_section)

    def _add_additional_section_before_first_section_taxi(self, journey, pt_object, additional_time):
        logging.getLogger(__name__).info("Creating additional section_before_first_section_taxi")
        additional_section = self._create_additional_section(
            pt_object, additional_time, journey.sections[0].begin_date_time, "section_0"
        )

        # We have to complete the origin of the first section ourselves
        # Because Jormun does not do it afterwards
        journey.sections[0].origin.CopyFrom(pt_object)

        # we push off the whole car section
        for s in journey.sections:
            s.begin_date_time += additional_time
            s.end_date_time += additional_time

        self._update_journey(journey, additional_section)

    def _create_additional_section(self, pt_object, additional_time, begin_date_time, section_id):
        additional_section = response_pb2.Section()

        additional_section.id = section_id
        additional_section.origin.CopyFrom(pt_object)
        additional_section.destination.CopyFrom(pt_object)
        additional_section.duration = additional_time
        additional_section.type = response_pb2.WAITING
        additional_section.begin_date_time = begin_date_time
        additional_section.end_date_time = additional_section.begin_date_time + additional_section.duration

        return additional_section

    def _update_journey(self, journey, additional_section):
        journey.duration += additional_section.duration
        journey.durations.total += additional_section.duration
        journey.arrival_date_time += additional_section.duration

        # The sections will be sorted afterwards based on begin and end_date_time
        journey.sections.extend([additional_section])
        journey.nb_sections += 1

    def get_street_network_routing_matrix(
        self, origins, destinations, street_network_mode, max_duration, request, **kwargs
    ):

        copy_request = copy.deepcopy(request)
        copy_request["car_no_park_speed"] = copy_request["taxi_speed"]
        response = self.street_network.get_street_network_routing_matrix(
            origins, destinations, street_network_mode, max_duration, copy_request, **kwargs
        )

        if response and len(response.rows):
            self._add_additional_time_in_routing_matrix(
                response.rows[0].routing_response, origins, destinations, copy_request
            )

        return response

    def _add_additional_time_in_routing_matrix(self, response, origins, destinations, request):
        addtional_matrix_time = (
            request["additional_time_after_first_section_taxi"]
            if len(origins) == 1
            else request["additional_time_before_last_section_taxi"]
        )

        for r in response:
            r.duration += addtional_matrix_time

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
        mode = 'car_no_park'
        return self.street_network.make_path_key(mode, orig_uri, dest_uri, streetnetwork_path_type, None)

    def post_processing(
        self, response, pt_object_origin, pt_object_destination, mode, request, direct_path_type
    ):
        if not response:
            return response
        copy_response = response_pb2.Response()
        copy_response.CopyFrom(response)

        if direct_path_type != StreetNetworkPathType.DIRECT:
            self._add_additional_section_in_fallback(
                copy_response, pt_object_origin, pt_object_destination, request, direct_path_type
            )
        return self.street_network.post_processing(
            copy_response, pt_object_origin, pt_object_destination, mode, request, direct_path_type
        )
