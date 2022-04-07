#  Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
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
from navitiacommon import request_pb2, response_pb2, type_pb2
import logging
from jormungandr import utils


class Kraken(object):
    def __init__(self, instance):
        self.instance = instance

    def place(self, place, request_id):
        req = request_pb2.Request()
        req.requested_api = type_pb2.place_uri
        req.place_uri.uri = place
        req.place_uri.depth = 2
        response = self.instance.send_and_receive(req, request_id=request_id)
        if response.places:
            return response.places[0]
        if utils.is_coord(place):
            # In some cases, the given "from" is not always a findable address by kraken
            # we just forward the given coord and return a pt object
            lon, lat = utils.get_lon_lat(place)
            p = type_pb2.PtObject()
            p.uri = place
            p.embedded_type = type_pb2.ADDRESS
            p.address.coord.lon = lon
            p.address.coord.lat = lat
            p.address.uri = place
            return p
        return None

    def get_car_co2_emission_on_crow_fly(self, origin, destination, request_id):
        logger = logging.getLogger(__name__)
        req = request_pb2.Request()
        req.requested_api = type_pb2.car_co2_emission
        req.car_co2_emission.origin.place = origin
        req.car_co2_emission.origin.access_duration = 0
        req.car_co2_emission.destination.place = destination
        req.car_co2_emission.destination.access_duration = 0

        response = self.instance.send_and_receive(req, request_id=request_id)
        if response.error and response.error.id == response_pb2.Error.error_id.Value('no_solution'):
            logger.error("Cannot compute car co2 emission from {} to {}".format(origin, destination))
            return None
        return response.car_co2_emission

    def get_crow_fly(
        self,
        origin,
        streetnetwork_mode,
        max_duration,
        max_nb_crowfly,
        object_type=type_pb2.STOP_POINT,
        filter=None,
        stop_points_nearby_duration=300,
        request_id=None,
        depth=2,
        forbidden_uris=[],
        allowed_id=[],
        **kwargs
    ):

        logger = logging.getLogger(__name__)
        # Getting stop_points or stop_areas using crow fly
        # the distance of crow fly is defined by the mode speed and max_duration
        req = request_pb2.Request()
        req.requested_api = type_pb2.places_nearby
        req.places_nearby.uri = origin
        req.places_nearby.distance = kwargs.get(streetnetwork_mode, kwargs.get("walking")) * max_duration
        req.places_nearby.depth = depth
        req.places_nearby.count = max_nb_crowfly
        req.places_nearby.start_page = 0
        req.disable_feedpublisher = True
        req.places_nearby.types.append(object_type)

        allowed_id_filter = ''
        if allowed_id is not None:
            allowed_id_count = len(allowed_id)
            if allowed_id_count > 0:
                poi_ids = ('poi.id={}'.format(uri) for uri in allowed_id)
                allowed_id_items = '  or  '.join(poi_ids)

                # Format the filter for all allowed_ids uris
                if allowed_id_count >= 1:
                    allowed_id_filter = ' and ({})'.format(allowed_id_items)

        # We implement filter only for poi with poi_type.uri=poi_type:amenity:parking
        if filter is not None:
            req.places_nearby.filter = filter + allowed_id_filter
        if streetnetwork_mode == "car":
            req.places_nearby.stop_points_nearby_radius = (
                kwargs.get("walking", 1.11) * stop_points_nearby_duration
            )
            req.places_nearby.depth = 1
        if forbidden_uris is not None:
            for uri in forbidden_uris:
                req.places_nearby.forbidden_uris.append(uri)
        res = self.instance.send_and_receive(req, request_id=request_id)
        if len(res.feed_publishers) != 0:
            logger.error("feed publisher not empty: expect performance regression!")
        return res.places_nearby

    def get_stop_points_for_stop_area(self, uri, request_id):
        req = request_pb2.Request()
        req.requested_api = type_pb2.PTREFERENTIAL
        req.ptref.requested_type = type_pb2.STOP_POINT
        req.ptref.count = 100
        req.ptref.start_page = 0
        req.ptref.depth = 0
        req.ptref.filter = 'stop_area.uri = {uri}'.format(uri=uri)

        result = self.instance.send_and_receive(req, request_id=request_id)
        if not result.stop_points:
            logging.getLogger(__name__).info(
                'PtRef, Unable to find stop_point with filter {}'.format(req.ptref.filter)
            )
        return result.stop_points

    def get_stop_points_from_uri(self, uri, request_id):
        req = request_pb2.Request()
        req.requested_api = type_pb2.PTREFERENTIAL
        req.ptref.requested_type = type_pb2.STOP_POINT
        req.ptref.count = 100
        req.ptref.start_page = 0
        req.ptref.depth = 0
        req.ptref.filter = 'stop_point.uri = {uri}'.format(uri=uri)
        result = self.instance.send_and_receive(req, request_id=request_id)
        return result.stop_points

    def get_odt_stop_points(self, coord, request_id):
        req = request_pb2.Request()
        req.requested_api = type_pb2.odt_stop_points
        req.coord.lon = coord.lon
        req.coord.lat = coord.lat
        return self.instance.send_and_receive(req, request_id=request_id).stop_points
