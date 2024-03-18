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
from __future__ import absolute_import

import collections
from navitiacommon import type_pb2
from jormungandr import utils, new_relic
from collections import namedtuple
import logging
from .helper_utils import timed_logger

FreeAccessObject = namedtuple('FreeAccessObject', ['uri', 'lon', 'lat'])
PlaceFreeAccessResult = namedtuple('PlaceFreeAccessResult', ['crowfly', 'odt', 'free_radius'])


class PlacesFreeAccess:
    """
    stop_points that are accessible freely from a given place: odt, stop_points of a stop_area, etc.
    """

    def __init__(self, future_manager, instance, requested_place_obj, pt_planner_name, request_id):
        """

        :param instance: instance of the coverage, all outside services callings pass through it(street network,
                         auto completion)
        :param requested_place_obj:
        """
        self._future_manager = future_manager
        self._instance = instance
        self._requested_place_obj = requested_place_obj
        self._value = None
        self._request_id = request_id
        self._async_request()
        self._logger = logging.getLogger(__name__)
        self._pt_planner = self._instance.get_pt_planner(pt_planner_name)

    @new_relic.distributedEvent("get_stop_points_for_stop_area", "places")
    def _get_stop_points_for_stop_area(self, uri):
        with timed_logger(self._logger, 'stop_points_for_stop_area_calling_external_service', self._request_id):
            return self._instance.georef.get_stop_points_for_stop_area(uri, self._request_id)

    @new_relic.distributedEvent("get_odt_stop_points", "places")
    def _get_odt_stop_points(self, coord):
        with timed_logger(self._logger, 'odt_stop_points_calling_external_service', self._request_id):
            return self._pt_planner.get_odt_stop_points(coord, self._request_id)

    def _do_request(self):
        self._logger.debug("requesting places with free access from %s", self._requested_place_obj.uri)
        crowfly = set()
        place = self._requested_place_obj

        if place.embedded_type == type_pb2.STOP_AREA:
            crowfly = {
                FreeAccessObject(sp[0], sp[1], sp[2])
                for sp in self._get_stop_points_for_stop_area(self._instance.georef, place.uri)
            }
        elif place.embedded_type == type_pb2.ADMINISTRATIVE_REGION:
            crowfly = {
                FreeAccessObject(sp.uri, sp.coord.lon, sp.coord.lat)
                for sa in place.administrative_region.main_stop_areas
                for sp in sa.stop_points
            }
        elif place.embedded_type == type_pb2.STOP_POINT:
            crowfly = {
                FreeAccessObject(place.stop_point.uri, place.stop_point.coord.lon, place.stop_point.coord.lat)
            }

        coord = utils.get_pt_object_coord(place)
        odt = set()

        if coord:
            odt_sps = self._get_odt_stop_points(self._pt_planner, coord)
            collections.deque(
                (odt.add(FreeAccessObject(sp.uri, sp.coord.lon, sp.coord.lat)) for sp in odt_sps),
                maxlen=1,
            )

        self._logger.debug("finish places with free access from %s", self._requested_place_obj.uri)

        # free_radius is empty until the proximities by crowfly are available
        free_radius = set()

        return PlaceFreeAccessResult(crowfly, odt, free_radius)

    def _async_request(self):
        self._value = self._future_manager.create_future(self._do_request)

    def wait_and_get(self):
        return self._value.wait_and_get()

    def __str__(self):
        return (
            "PlacesFreeAccess: _future_manager:{} _instance:{} _requested_place_obj:{} _value:{} _request_id:{} "
            "_async_request:{} _logger:{} _pt_planner:{}"
        ).format(
            self._future_manager,
            self._instance,
            self._requested_place_obj,
            self._value,
            self._request_id,
            self._async_request(),
            self._logger,
            self._pt_planner,
        )
