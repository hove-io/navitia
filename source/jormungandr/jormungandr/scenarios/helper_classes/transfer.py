# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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
from navitiacommon import response_pb2
import itertools
import logging
from jormungandr.street_network.street_network import StreetNetworkPathType
from jormungandr.utils import PeriodExtremity
from jormungandr.fallback_modes import FallbackModes

NO_ACCESS_POINTS_PHYSICAL_MODES = (
    'physical_mode:Bus',
    'physical_mode:Tramway',
    'physical_mode:BusRapidTransit',
    'physical_mode:Coach',
    'physical_mode:Shuttle',
)

ACCESS_POINTS_PHYSICAL_MODES = ("physical_mode:RapidTransit",)

NO_ACCESS_POINTS_TRANSFER = set(
    itertools.product(NO_ACCESS_POINTS_PHYSICAL_MODES, NO_ACCESS_POINTS_PHYSICAL_MODES)
)

ACCESS_POINTS_TRANSFER = set(
    itertools.product(ACCESS_POINTS_PHYSICAL_MODES, NO_ACCESS_POINTS_PHYSICAL_MODES)
) | set(itertools.product(NO_ACCESS_POINTS_PHYSICAL_MODES, ACCESS_POINTS_PHYSICAL_MODES))


class TransferPool(object):
    def __init__(
        self,
        future_manager,
        instance,
        request,
    ):
        self._future_manager = future_manager
        self._instance = instance
        self._request = request
        self._streetnetwork_service = self._instance.get_street_network("walking", request)
        self._transfers_future = dict()
        self._logger = logging.getLogger(__name__)

    @staticmethod
    def hasfield(section, field):
        try:
            return section.HasField(field)
        except ValueError:
            return False

    def get_physical_mode(self, section):
        if (
            self.hasfield(section, 'pt_display_informations')
            and self.hasfield(section.pt_display_informations, 'uris')
            and self.hasfield(section.pt_display_informations.uris, 'physical_mode')
        ):
            return section.pt_display_informations.uris.physical_mode
        else:
            return None

    def async_compute_transfer(self, journey_sections):
        for index, section in enumerate(journey_sections[1:-2], start=1):
            if section.type != response_pb2.TRANSFER:
                continue

            prev_section = journey_sections[index - 1]

            next_section = (
                journey_sections[index + 2]
                if journey_sections[index + 1].type == response_pb2.WAITING
                else journey_sections[index + 1]
            )

            prev_section_mode = self.get_physical_mode(prev_section)
            next_section_mode = self.get_physical_mode(next_section)

            section_key = self._get_section_key(section)

            if (prev_section_mode, next_section_mode) in NO_ACCESS_POINTS_TRANSFER:
                if section_key not in self._transfers_future:
                    self._transfers_future[section_key] = self._aysnc_no_access_point_transfer(section)
            elif (prev_section_mode, next_section_mode) in ACCESS_POINTS_TRANSFER:
                # TODO: there will be a whole new feature!
                continue
            else:
                continue

    def _do_no_access_point_transfer(self, section):
        sub_request_id = "transfer_{}_{}".format(section.origin.uri, section.destination.uri)
        direct_path_type = StreetNetworkPathType.DIRECT
        extremity = PeriodExtremity(section.end_date_time, False)
        return self._streetnetwork_service.direct_path_with_fp(
            self._instance,
            FallbackModes.walking.name,
            section.origin,
            section.destination,
            extremity,
            self._request,
            direct_path_type,
            sub_request_id,
        )

    def _aysnc_no_access_point_transfer(self, section):
        return self._future_manager.create_future(self._do_no_access_point_transfer, section)

    @staticmethod
    def _get_section_key(section):
        return section.origin.uri, section.destination.uri

    def wait_and_complete(self, section):
        future = self._transfers_future.get(self._get_section_key(section))
        if future is None:
            return

        transfer_journey = future.wait_and_get()
        if transfer_journey and transfer_journey.journeys:
            new_section = transfer_journey.journeys[0].sections
            section.street_network.CopyFrom(new_section[0].street_network)
