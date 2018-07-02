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
from jormungandr import utils
from navitiacommon import request_pb2, type_pb2


class JourneyParameters(object):
    def __init__(self,
                 max_duration=86400,
                 max_transfers=10,
                 wheelchair=False,
                 forbidden_uris=None,
                 allowed_id=None,
                 realtime_level='base_schedule',
                 max_extra_second_pass=None,
                 walking_transfer_penalty=120,
                 direct_path_duration=None,
                 night_bus_filter_max_factor=None,
                 night_bus_filter_base_factor=None,
                 min_nb_journeys=None,
                 timeframe=None
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


class Kraken(object):

    def __init__(self, instance):
        self.instance = instance

    def journeys(self, origins, destinations, datetime, clockwise, journey_parameters, bike_in_pt):
        req = request_pb2.Request()
        req.requested_api = type_pb2.pt_planner
        for stop_point_id, access_duration in origins.items():
            location = req.journeys.origin.add()
            location.place = stop_point_id
            location.access_duration = access_duration

        for stop_point_id, access_duration in destinations.items():
            location = req.journeys.destination.add()
            location.place = stop_point_id
            location.access_duration = access_duration

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
           req.journeys.timeframe_end_datetime = int(journey_parameters.timeframe.end_datetime)
           req.journeys.timeframe_max_datetime = int(journey_parameters.timeframe.max_datetime)

        return self.instance.send_and_receive(req)
