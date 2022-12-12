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
from __future__ import absolute_import, print_function, unicode_literals, division
from collections import namedtuple
from navitiacommon import response_pb2, type_pb2
import itertools
import logging
from jormungandr.street_network.street_network import StreetNetworkPathType
from jormungandr.utils import PeriodExtremity
from jormungandr.fallback_modes import FallbackModes
from jormungandr.scenarios.helper_classes.helper_utils import (
    prepend_first_and_append_last_coord,
    append_path_item_with_access_point,
    prepend_path_item_with_access_point,
)

NO_ACCESS_POINTS_PHYSICAL_MODES = (
    'physical_mode:Bus',
    'physical_mode:Tramway',
    'physical_mode:BusRapidTransit',
    'physical_mode:Coach',
    'physical_mode:Shuttle',
)

ACCESS_POINTS_PHYSICAL_MODES = ("physical_mode:RapidTransit", "physical_mode:Metro")

# if `(physical_mode:A, physical_mode:B) in NO_ACCESS_POINTS_TRANSFER` then it means that a transfer
# where we get out of a vehicle of  `physical_mode:A` and then get in a vehicle of `physical_mode:B`
# **will not** go through an access point
NO_ACCESS_POINTS_TRANSFER = set(
    itertools.product(NO_ACCESS_POINTS_PHYSICAL_MODES, NO_ACCESS_POINTS_PHYSICAL_MODES)
)

# if `(physical_mode:A, physical_mode:B) in ACCESS_POINTS_TRANSFER` then it means that a transfer
# where we get out of a vehicle of  `physical_mode:A` and then get in a vehicle of `physical_mode:B`
# **may use** an access point
# Note that we don't handle (ACCESS_POINTS_PHYSICAL_MODES, ACCESS_POINTS_PHYSICAL_MODES) yet
ACCESS_POINTS_TRANSFER = set(
    itertools.product(ACCESS_POINTS_PHYSICAL_MODES, NO_ACCESS_POINTS_PHYSICAL_MODES)
) | set(itertools.product(NO_ACCESS_POINTS_PHYSICAL_MODES, ACCESS_POINTS_PHYSICAL_MODES))


TransferResult = namedtuple('TransferResult', ['direct_path', 'origin', 'destination'])


class TransferPool(object):
    def __init__(
        self,
        future_manager,
        instance,
        request,
        request_id,
    ):
        self._future_manager = future_manager
        self._instance = instance
        self._request = request
        self._request_id = request_id
        self._streetnetwork_service = self._instance.get_street_network(FallbackModes.walking.name, request)
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
            if section_key in self._transfers_future:
                continue

            if (prev_section_mode, next_section_mode) in NO_ACCESS_POINTS_TRANSFER:
                self._transfers_future[section_key] = self._aysnc_no_access_point_transfer(section)
            elif (prev_section_mode, next_section_mode) in ACCESS_POINTS_TRANSFER:
                self._transfers_future[section_key] = self._aysnc_access_point_transfer(
                    section, prev_section_mode, next_section_mode
                )
            else:
                continue

    def _do_no_access_point_transfer(self, section):
        sub_request_id = "{}_transfer_{}_{}".format(
            self._request_id, section.origin.uri, section.destination.uri
        )
        direct_path_type = StreetNetworkPathType.DIRECT
        extremity = PeriodExtremity(section.end_date_time, False)
        direct_path = self._streetnetwork_service.direct_path_with_fp(
            self._instance,
            FallbackModes.walking.name,
            section.origin,
            section.destination,
            extremity,
            self._request,
            direct_path_type,
            sub_request_id,
        )

        if direct_path and direct_path.journeys:
            return TransferResult(direct_path, section.origin, section.destination)

        return None

    def _aysnc_no_access_point_transfer(self, section):
        return self._future_manager.create_future(self._do_no_access_point_transfer, section)

    def _get_access_points(self, stop_point_uri, predicator=lambda x: x):
        sub_request_id = "{}_transfer_start_{}".format(self._request_id, stop_point_uri)
        stop_points = self._instance.georef.get_stop_points_from_uri(stop_point_uri, sub_request_id, depth=3)
        if not stop_points:
            return None

        return [
            type_pb2.PtObject(name=ap.name, uri=ap.uri, embedded_type=type_pb2.ACCESS_POINT, access_point=ap)
            for ap in stop_points[0].access_points
            if predicator(ap)
        ]

    def _do_access_point_transfer(self, section, prev_section_mode, next_section_mode):
        access_points = None
        if prev_section_mode in ACCESS_POINTS_PHYSICAL_MODES:
            access_points = self._get_access_points(section.origin.uri, predicator=lambda ap: ap.is_exit)
            # if no access points are found for this stop point, which is supposed to have access points
            # we do nothing about the transfer path
            if not access_points:
                return None

        elif next_section_mode in ACCESS_POINTS_PHYSICAL_MODES:
            access_points = self._get_access_points(
                section.destination.uri, predicator=lambda ap: ap.is_entrance
            )
            # if no access points are found for this stop point, which is supposed to have access points
            # we do nothing about the transfer path
            if not access_points:
                return None

        # determine the origins and the destinations for matrix computation
        origins = access_points if prev_section_mode in ACCESS_POINTS_PHYSICAL_MODES else [section.origin]
        destinations = (
            access_points if next_section_mode in ACCESS_POINTS_PHYSICAL_MODES else [section.destination]
        )

        if len(origins) > 1 and len(destinations) > 1:
            self._logger.error(
                "Error occurred when computing transfer path both origins and destinations's sizes are larger than 1"
            )
            return None

        if len(origins) == 1 and len(destinations) == 1:
            sub_request_id = "{}_transfer_{}_{}".format(self._request_id, origins[0].uri, destinations[0].uri)
            direct_path_type = StreetNetworkPathType.DIRECT
            extremity = PeriodExtremity(section.end_date_time, False)
            direct_path = self._streetnetwork_service.direct_path_with_fp(
                self._instance,
                FallbackModes.walking.name,
                origins[0],
                destinations[0],
                extremity,
                self._request,
                direct_path_type,
                sub_request_id,
            )
            if direct_path and direct_path.journeys:
                return TransferResult(direct_path, origins[0], destinations[0])
            return None

        sub_request_id = "{}_transfer_matrix".format(self._request_id)
        routing_matrix = self._streetnetwork_service.get_street_network_routing_matrix(
            self._instance,
            origins,
            destinations,
            FallbackModes.walking.name,
            section.duration * 2,
            self._request,
            sub_request_id,
        )

        # now it's time to find the best combo
        # (stop_point -> access_points or access_points -> stop_point)

        # since the matrix is not really a matrix(NxM) but a single row(1xN)...
        best_access_point = None
        best_duration = float('inf')
        for i, ap in enumerate(access_points):
            if routing_matrix.rows[0].routing_response[i].routing_status != response_pb2.reached:
                continue
            total_duration = routing_matrix.rows[0].routing_response[i].duration + ap.access_point.traversal_time
            if total_duration < best_duration:
                best_duration = total_duration
                best_access_point = ap

        # now we are going to compute the real path
        origin = best_access_point if prev_section_mode in ACCESS_POINTS_PHYSICAL_MODES else section.origin
        destination = (
            best_access_point if next_section_mode in ACCESS_POINTS_PHYSICAL_MODES else section.destination
        )

        sub_request_id = "{}_transfer_{}_{}".format(self._request_id, origins[0].uri, destinations[0].uri)
        direct_path_type = StreetNetworkPathType.DIRECT
        extremity = PeriodExtremity(section.end_date_time, False)

        direct_path = self._streetnetwork_service.direct_path_with_fp(
            self._instance,
            FallbackModes.walking.name,
            origin,
            destination,
            extremity,
            self._request,
            direct_path_type,
            sub_request_id,
        )
        if direct_path and direct_path.journeys:
            return TransferResult(direct_path, origin, destination)

        return None

    def _aysnc_access_point_transfer(self, section, prev_section_mode, next_section_mode):
        return self._future_manager.create_future(
            self._do_access_point_transfer, section, prev_section_mode, next_section_mode
        )

    @staticmethod
    def _get_section_key(section):
        return section.origin.uri, section.destination.uri

    def _is_valid(self, transfer_result):
        return (
            transfer_result
            and transfer_result.direct_path
            and transfer_result.direct_path.journeys
            and transfer_result.direct_path.journeys[0].sections
        )

    @staticmethod
    def _is_access_point(pt_object):
        return isinstance(pt_object, type_pb2.PtObject) and pt_object.embedded_type == type_pb2.ACCESS_POINT

    @staticmethod
    def _add_via_if_access_point(section, pt_object):
        if TransferPool._is_access_point(pt_object):
            section.vias.add().CopyFrom(pt_object.access_point)

    def wait_and_complete(self, section):
        future = self._transfers_future.get(self._get_section_key(section))
        if future is None:
            return

        transfer_result = future.wait_and_get()
        if not self._is_valid(transfer_result):
            return

        transfer_direct_path = transfer_result.direct_path

        if transfer_result.origin and transfer_result.destination:
            prepend_first_and_append_last_coord(
                transfer_direct_path, transfer_result.origin, transfer_result.destination
            )

        self._add_via_if_access_point(section, transfer_result.origin)
        self._add_via_if_access_point(section, transfer_result.destination)

        transfer_street_network = transfer_direct_path.journeys[0].sections[0].street_network

        if self._is_access_point(transfer_result.origin):
            prepend_path_item_with_access_point(
                transfer_street_network.path_items,
                section.origin.stop_point,
                transfer_result.origin.access_point,
            )

        if self._is_access_point(transfer_result.destination):
            append_path_item_with_access_point(
                transfer_street_network.path_items,
                section.destination.stop_point,
                transfer_result.destination.access_point,
            )

        section.street_network.CopyFrom(transfer_street_network)
