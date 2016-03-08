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
from jormungandr import utils
from navitiacommon import request_pb2, type_pb2


class JourneyParameters(object):
    def __init__(self,
                 max_duration=86400,
                 max_transfers=10,
                 wheelchair=False,
                 forbidden_uris=None,
                 realtime_level='base_schedule',
                 max_extra_second_pass=None,
                 walking_transfer_penalty=120):
        self.max_duration = max_duration
        self.max_transfers = max_transfers
        self.wheelchair = wheelchair
        self.forbidden_uris = set(forbidden_uris) if forbidden_uris else set()
        self.realtime_level = realtime_level
        self.max_extra_second_pass = max_extra_second_pass


class Kraken(object):

    def __init__(self, instance):
        self.instance = instance

    def journeys(self, origins, destinations, datetime, clockwise, journey_parameters):
        req = request_pb2.Request()
        req.requested_api = type_pb2.pt_planner
        for stop_point_id, access_duration in origins.iteritems():
            location = req.journeys.origin.add()
            location.place = stop_point_id
            location.access_duration = access_duration

        for stop_point_id, access_duration in destinations.iteritems():
            location = req.journeys.destination.add()
            location.place = stop_point_id
            location.access_duration = access_duration

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

        return self.instance.send_and_receive(req)
