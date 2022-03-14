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
        arrival_transfer_penalty=default_values.arrival_transfer_penalty,
        walking_transfer_penalty=default_values.walking_transfer_penalty,
        direct_path_duration=None,
        night_bus_filter_max_factor=default_values.night_bus_filter_max_factor,
        night_bus_filter_base_factor=default_values.night_bus_filter_base_factor,
        min_nb_journeys=default_values.min_nb_journeys,
        timeframe=None,
        depth=1,
        isochrone_center=None,
        sn_params=None,
        current_datetime=None,
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
        self.current_datetime = current_datetime
        self.arrival_transfer_penalty = arrival_transfer_penalty
        self.walking_transfer_penalty = walking_transfer_penalty


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

    def journeys(
        self,
        origins,
        destinations,
        datetime,
        clockwise,
        journey_parameters,
        bike_in_pt,
        request_id,
        use_pt_socket,
    ):
        req = utils.create_journeys_request(
            origins, destinations, datetime, clockwise, journey_parameters, bike_in_pt
        )
        return self.instance.send_and_receive(req, request_id=request_id, use_pt_socket=use_pt_socket)

    def graphical_isochrones(
        self, origins, destinations, datetime, clockwise, graphical_isochrones_parameters, bike_in_pt
    ):
        req = utils.create_graphical_isochrones_request(
            origins, destinations, datetime, clockwise, graphical_isochrones_parameters, bike_in_pt
        )
        return self.instance.send_and_receive(req)
