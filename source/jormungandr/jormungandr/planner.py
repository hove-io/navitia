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
from jormungandr import utils
from navitiacommon import request_pb2, type_pb2, default_values


class JourneyParameters(object):
    def __init__(
        self,
        max_duration=default_values.max_duration,
        max_transfers=10,
        wheelchair=False,
        forbidden_uris=None,
        allowed_id=None,
        realtime_level='base_schedule',
        max_extra_second_pass=default_values.max_extra_second_pass,
        walking_transfer_penalty=default_values.walking_transfer_penalty,
        direct_path_duration=None,
        night_bus_filter_max_factor=default_values.night_bus_filter_max_factor,
        night_bus_filter_base_factor=default_values.night_bus_filter_base_factor,
        min_nb_journeys=default_values.min_nb_journeys,
        timeframe=None,
        depth=1,
        isochrone_center=None,
        sn_params=None,
    ):

        self.max_duration = max_duration
        self.max_transfers = max_transfers
        self.wheelchair = wheelchair
        self.forbidden_uris = set(forbidden_uris) if forbidden_uris else set()
        self.allowed_id = set(allowed_id) if allowed_id else set()
        self.realtime_level = realtime_level
        self.max_extra_second_pass = max_extra_second_pass
        self.direct_path_duration = direct_path_duration
        self.night_bus_filter_max_factor = night_bus_filter_max_factor
        self.night_bus_filter_base_factor = night_bus_filter_base_factor
        self.min_nb_journeys = min_nb_journeys
        self.timeframe = timeframe
        self.depth = depth
        self.isochrone_center = isochrone_center
        self.sn_params = sn_params


# Needed for GraphicalIsochrones
class StreetNetworkParameters(object):
    def __init__(
        self,
        origin_mode=None,
        destination_mode=None,
        walking_speed=None,
        bike_speed=None,
        car_speed=None,
        bss_speed=None,
        car_no_park_speed=None,
    ):
        self.origin_mode = origin_mode
        self.destination_mode = destination_mode
        self.walking_speed = walking_speed
        self.bike_speed = bike_speed
        self.car_speed = car_speed
        self.bss_speed = bss_speed
        self.car_no_park_speed = car_no_park_speed


class GraphicalIsochronesParameters(object):
    def __init__(self, journeys_parameters=JourneyParameters(), min_duration=0, boundary_duration=None):
        self.journeys_parameters = journeys_parameters
        self.min_duration = min_duration
        self.boundary_duration = boundary_duration


class Kraken(object):
    def __init__(self, instance):
        self.instance = instance

    def _create_journeys_request(
        self, origins, destinations, datetime, clockwise, journey_parameters, bike_in_pt
    ):
        req = request_pb2.Request()
        req.requested_api = type_pb2.pt_planner
        import six

        for stop_point_id, access_duration in six.iteritems(origins):
            location = req.journeys.origin.add()
            location.place = stop_point_id
            location.access_duration = access_duration.duration

        for stop_point_id, access_duration in six.iteritems(destinations):
            location = req.journeys.destination.add()
            location.place = stop_point_id
            location.access_duration = access_duration.duration

        req.journeys.night_bus_filter_max_factor = journey_parameters.night_bus_filter_max_factor
        req.journeys.night_bus_filter_base_factor = journey_parameters.night_bus_filter_base_factor

        req.journeys.datetimes.append(datetime)
        req.journeys.clockwise = clockwise
        req.journeys.realtime_level = utils.realtime_level_to_pbf(journey_parameters.realtime_level)
        req.journeys.max_duration = journey_parameters.max_duration
        req.journeys.max_transfers = journey_parameters.max_transfers
        req.journeys.wheelchair = journey_parameters.wheelchair
        if journey_parameters.max_extra_second_pass:
            req.journeys.max_extra_second_pass = journey_parameters.max_extra_second_pass

        for uri in journey_parameters.forbidden_uris:
            req.journeys.forbidden_uris.append(uri)

        for id in journey_parameters.allowed_id:
            req.journeys.allowed_id.append(id)

        if journey_parameters.direct_path_duration is not None:
            req.journeys.direct_path_duration = journey_parameters.direct_path_duration

        req.journeys.bike_in_pt = bike_in_pt

        if journey_parameters.min_nb_journeys:
            req.journeys.min_nb_journeys = journey_parameters.min_nb_journeys

        if journey_parameters.timeframe:
            req.journeys.timeframe_duration = int(journey_parameters.timeframe)

        if journey_parameters.depth:
            req.journeys.depth = journey_parameters.depth

        if journey_parameters.isochrone_center:
            req.journeys.isochrone_center.place = journey_parameters.isochrone_center
            req.journeys.isochrone_center.access_duration = 0
            req.requested_api = type_pb2.ISOCHRONE

        if journey_parameters.sn_params:
            sn_params_request = req.journeys.streetnetwork_params
            sn_params_request.origin_mode = journey_parameters.sn_params.origin_mode
            sn_params_request.destination_mode = journey_parameters.sn_params.destination_mode
            sn_params_request.walking_speed = journey_parameters.sn_params.walking_speed
            sn_params_request.bike_speed = journey_parameters.sn_params.bike_speed
            sn_params_request.car_speed = journey_parameters.sn_params.car_speed
            sn_params_request.bss_speed = journey_parameters.sn_params.bss_speed
            sn_params_request.car_no_park_speed = journey_parameters.sn_params.car_no_park_speed

        return req

    def _create_graphical_isochrones_request(
        self, origins, destinations, datetime, clockwise, graphical_isochrones_parameters, bike_in_pt
    ):
        req = self._create_journeys_request(
            origins,
            destinations,
            datetime,
            clockwise,
            graphical_isochrones_parameters.journeys_parameters,
            bike_in_pt,
        )
        req.requested_api = type_pb2.graphical_isochrone
        req.journeys.max_duration = graphical_isochrones_parameters.journeys_parameters.max_duration
        if graphical_isochrones_parameters.boundary_duration:
            for duration in sorted(graphical_isochrones_parameters.boundary_duration, key=int, reverse=True):
                if graphical_isochrones_parameters.min_duration < duration < req.journeys.max_duration:
                    req.isochrone.boundary_duration.append(duration)
        req.isochrone.boundary_duration.insert(0, req.journeys.max_duration)
        req.isochrone.boundary_duration.append(graphical_isochrones_parameters.min_duration)

        # We are consistent with new_default there.
        if req.journeys.origin:
            req.journeys.clockwise = True
        else:
            req.journeys.clockwise = False

        req.isochrone.journeys_request.CopyFrom(req.journeys)
        return req

    def journeys(self, origins, destinations, datetime, clockwise, journey_parameters, bike_in_pt):

        import logging

        logging.getLogger(__name__).info("Creating journey request!!------------")
        req = self._create_journeys_request(
            origins, destinations, datetime, clockwise, journey_parameters, bike_in_pt
        )
        logging.getLogger(__name__).info("finish creating journey request!!-------------")

        return self.instance.send_and_receive(req)

    def graphical_isochrones(
        self, origins, destinations, datetime, clockwise, graphical_isochrones_parameters, bike_in_pt
    ):
        req = self._create_graphical_isochrones_request(
            origins, destinations, datetime, clockwise, graphical_isochrones_parameters, bike_in_pt
        )
        return self.instance.send_and_receive(req)
