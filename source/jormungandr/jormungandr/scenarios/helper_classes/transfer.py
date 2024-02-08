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

import copy
from collections import namedtuple
from navitiacommon import response_pb2, type_pb2
from jormungandr.protobuf_to_dict import protobuf_to_dict
from jormungandr import app, cache
import itertools
import logging
from jormungandr.street_network.street_network import StreetNetworkPathType
from jormungandr.utils import PeriodExtremity
from jormungandr.fallback_modes import FallbackModes
from jormungandr.scenarios.helper_classes.helper_utils import (
    prepend_first_coord,
    append_last_coord,
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

ACCESS_POINTS_PHYSICAL_MODES = (
    "physical_mode:LongDistanceTrain",
    "physical_mode:LocalTrain",
    "physical_mode:Train",
    "physical_mode:RapidTransit",
    "physical_mode:Metro",
)

MAP_STRING_PTOBJECT_TYPE = {
    "STOP_POINT": type_pb2.STOP_POINT,
    "ACCESS_POINT": type_pb2.ACCESS_POINT,
    "ADDRESS": type_pb2.ADDRESS,
}


MAP_CYCLEPATHTYPE = {
    "NoCycleLane": response_pb2.NoCycleLane,
    "SharedCycleWay": response_pb2.SharedCycleWay,
    "DedicatedCycleWay": response_pb2.DedicatedCycleWay,
    "SeparatedCycleWay": response_pb2.SeparatedCycleWay,
}
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


class TransferPathArgs:
    def __init__(self, section, prev_section_mode, next_section_mode):
        self.prev_section_mode = prev_section_mode
        self.next_section_mode = next_section_mode
        self.section = section

    def __repr__(self):
        return "{origin}:{destination}:{prev_section_mode}:{next_section_mode}".format(
            origin=self.section.origin.uri,
            destination=self.section.destination.uri,
            prev_section_mode=self.prev_section_mode,
            next_section_mode=self.next_section_mode,
        )


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

    def __repr__(self):
        return "{name}:{language}:{publication_date}".format(
            name=self._instance.name, language=self.language, publication_date=self._instance.publication_date
        )

    @property
    def language(self):
        return self._request.get('language', "en-US")

    def _make_sub_request_id(self, origin_uri, destination_uri):
        return "{}_transfer_{}_{}".format(self._request_id, origin_uri, destination_uri)

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
        sub_request_id = self._make_sub_request_id(section.origin.uri, section.destination.uri)

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

    def _get_access_points(self, pt_object, access_point_filter=lambda x: x):
        if self._request.get("_pt_planner") == "loki":
            return [
                type_pb2.PtObject(name=ap.name, uri=ap.uri, embedded_type=type_pb2.ACCESS_POINT, access_point=ap)
                for ap in pt_object.stop_point.access_points
                if access_point_filter(ap)
            ]

        sub_request_id = "{}_transfer_start_{}".format(self._request_id, pt_object.uri)
        stop_points = self._instance.georef.get_stop_points_from_uri(pt_object.uri, sub_request_id, depth=2)
        if not stop_points:
            return None

        return [
            type_pb2.PtObject(name=ap.name, uri=ap.uri, embedded_type=type_pb2.ACCESS_POINT, access_point=ap)
            for ap in stop_points[0].access_points
            if access_point_filter(ap)
        ]

    def get_underlying_access_points(self, section, prev_section_mode, next_section_mode):
        """
        find out based on with extremity of the section the access points are calculated and request the georef for
        access_points of the underlying stop_point
        return: access_points
        """
        if prev_section_mode in ACCESS_POINTS_PHYSICAL_MODES:
            return self._get_access_points(
                section.origin, access_point_filter=lambda access_point: access_point.is_exit
            )

        if next_section_mode in ACCESS_POINTS_PHYSICAL_MODES:
            return self._get_access_points(
                section.destination, access_point_filter=lambda access_point: access_point.is_entrance
            )

        return None

    @staticmethod
    def determinate_matrix_entry(section, access_points, prev_section_mode, next_section_mode):
        """
        determine the origins and the destinations for matrix computation
        return: origins, destinations
        """
        origins = access_points if prev_section_mode in ACCESS_POINTS_PHYSICAL_MODES else [section.origin]
        destinations = (
            access_points if next_section_mode in ACCESS_POINTS_PHYSICAL_MODES else [section.destination]
        )
        return origins, destinations

    @staticmethod
    def determinate_direct_path_entry(section, access_point, prev_section_mode, next_section_mode):
        """
        determinate the origin and the destination for final direct path computation
        return: origin, destination
        """
        # now we are going to compute the real path
        origin = access_point if prev_section_mode in ACCESS_POINTS_PHYSICAL_MODES else section.origin
        destination = access_point if next_section_mode in ACCESS_POINTS_PHYSICAL_MODES else section.destination

        return origin, destination

    @staticmethod
    def determinate_the_best_access_point(routing_matrix, access_points):
        """
        determinate the best access point
        return: access_point
        """
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

        return best_access_point

    def _get_transfer_result(self, section, origin, destination):

        sub_request_id = self._make_sub_request_id(origin.uri, destination.uri)
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
            return (
                protobuf_to_dict(direct_path, use_enum_labels=True),
                protobuf_to_dict(origin, use_enum_labels=True),
                protobuf_to_dict(destination, use_enum_labels=True),
            )
        return None, None, None

    def pb_object_from_json(self, json_object):
        pb_object = type_pb2.PtObject()
        str_embedded_type = json_object.get("embedded_type")
        pb_embedded_type = MAP_STRING_PTOBJECT_TYPE.get(str_embedded_type)
        str_embedded_type = str_embedded_type.lower()
        pb_object.embedded_type = pb_embedded_type
        for attribute in ["uri", "name"]:
            setattr(pb_object, attribute, json_object.get(attribute))
        pb_attr = getattr(pb_object, str_embedded_type)
        json_data = json_object.get(str_embedded_type, {})
        for attribute in ["uri", "name", "label"]:
            if attribute in json_data:
                setattr(pb_attr, attribute, json_data.get(attribute))
        pb_attr.coord.lon = json_data.get("coord", {}).get("lon")
        pb_attr.coord.lat = json_data.get("coord", {}).get("lat")
        if pb_embedded_type == type_pb2.ACCESS_POINT:
            for attribute in ["is_entrance", "is_exit", "pathway_mode", "length", "traversal_time", "stop_code"]:
                setattr(pb_attr, attribute, json_data.get(attribute))
            pb_attr.embedded_type = type_pb2.pt_access_point
        return pb_object

    def pb_transfer_path(self, json_transfer_path):
        resp = response_pb2.Response()
        resp.status_code = 200
        resp.response_type = response_pb2.ITINERARY_FOUND
        for json_journey in json_transfer_path.get("journeys", []):
            journey = resp.journeys.add()
            for json_section in json_journey.get("sections", []):
                section = journey.sections.add()
                section.type = response_pb2.STREET_NETWORK
                section.street_network.mode = response_pb2.Walking
                street_network = json_section.get("street_network", {})
                for item in street_network.get("path_items", []):
                    path_item = section.street_network.path_items.add()
                    for attribute in ["name", "length", "direction", "duration", "instruction"]:
                        if attribute in item:
                            setattr(path_item, attribute, item.get(attribute))
                    instruction_start_coordinate = item.get("instruction_start_coordinate")
                    if instruction_start_coordinate:
                        path_item.instruction_start_coordinate.lon = instruction_start_coordinate.get("lon")
                        path_item.instruction_start_coordinate.lat = instruction_start_coordinate.get("lat")
                    if "cycle_path_type" in item:
                        path_item.cycle_path_type = MAP_CYCLEPATHTYPE.get(item["cycle_path_type"])

                for coord in street_network.get("coordinates", []):
                    section.street_network.coordinates.add(lon=coord.get("lon"), lat=coord.get("lat"))
                for st_info in street_network.get("street_information", []):
                    pb_street_information = section.street_network.street_information.add()
                    pb_street_information.geojson_offset = st_info.get("geojson_offset")
                    pb_street_information.cycle_path_type = MAP_CYCLEPATHTYPE[st_info.get("cycle_path_type")]
                    pb_street_information.length = st_info.get("length")

        return resp

    @cache.memoize(app.config[str('CACHE_CONFIGURATION')].get(str('TIMEOUT_TRANSFER_PATH'), 10 * 60))
    def get_cached_transfer_path(self, transfer_path_args):
        access_points = self.get_underlying_access_points(
            transfer_path_args.section,
            transfer_path_args.prev_section_mode,
            transfer_path_args.next_section_mode,
        )
        # if no access points are found for this stop point, which is supposed to have access points
        # we do nothing about the transfer path
        if not access_points:
            return None, None, None

        origins, destinations = self.determinate_matrix_entry(
            transfer_path_args.section,
            access_points,
            transfer_path_args.prev_section_mode,
            transfer_path_args.next_section_mode,
        )

        if len(origins) > 1 and len(destinations) > 1:
            self._logger.error(
                "Error occurred when computing transfer path both origin's and destination's sizes are larger than 1"
            )
            return None, None, None

        if len(origins) == 1 and len(destinations) == 1:
            return self._get_transfer_result(transfer_path_args.section, origins[0], destinations[0])

        sub_request_id = "{}_transfer_matrix".format(self._request_id)
        routing_matrix = self._streetnetwork_service.get_street_network_routing_matrix(
            self._instance,
            origins,
            destinations,
            FallbackModes.walking.name,
            transfer_path_args.section.duration * 3,
            self._request,
            sub_request_id,
        )

        if all(
            matrix.routing_status == response_pb2.unreached for matrix in routing_matrix.rows[0].routing_response
        ):
            logging.getLogger(__name__).warning("no access points is reachable in transfer path computation")
            return None, None, None

        # now it's time to find the best combo
        # (stop_point -> access_points or access_points -> stop_point)
        best_access_point = self.determinate_the_best_access_point(routing_matrix, access_points)

        origin, destination = self.determinate_direct_path_entry(
            transfer_path_args.section,
            best_access_point,
            transfer_path_args.prev_section_mode,
            transfer_path_args.next_section_mode,
        )
        return self._get_transfer_result(transfer_path_args.section, origin, destination)

    def _do_access_point_transfer(self, section, prev_section_mode, next_section_mode):
        path, origin, destination = self.get_cached_transfer_path(
            TransferPathArgs(section, prev_section_mode, next_section_mode)
        )
        if not path:
            return None
        pb_origin = self.pb_object_from_json(origin)
        pb_destination = self.pb_object_from_json(destination)
        pb_path = self.pb_transfer_path(path)
        return TransferResult(pb_path, pb_origin, pb_destination)

    def _aysnc_access_point_transfer(self, section, prev_section_mode, next_section_mode):
        return self._future_manager.create_future(
            self._do_access_point_transfer, section, prev_section_mode, next_section_mode
        )

    @staticmethod
    def _get_section_key(section):
        return section.origin.uri, section.destination.uri

    @staticmethod
    def _is_valid(transfer_result):
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

        transfer_direct_path = copy.deepcopy(transfer_result.direct_path)

        if transfer_result.origin and transfer_result.destination:
            prepend_first_coord(transfer_direct_path, transfer_result.origin)
            append_last_coord(transfer_direct_path, transfer_result.destination)

        self._add_via_if_access_point(section, transfer_result.origin)
        self._add_via_if_access_point(section, transfer_result.destination)

        # we assume here the transfer street network has only one section, which is in walking mode
        transfer_street_network = transfer_direct_path.journeys[0].sections[0].street_network

        if self._is_access_point(transfer_result.origin):
            prepend_path_item_with_access_point(
                transfer_street_network.path_items,
                section.origin.stop_point,
                transfer_result.origin.access_point,
                self.language,
            )

        if self._is_access_point(transfer_result.destination):
            append_path_item_with_access_point(
                transfer_street_network.path_items,
                section.destination.stop_point,
                transfer_result.destination.access_point,
                self.language,
            )

        section.street_network.CopyFrom(transfer_street_network)
