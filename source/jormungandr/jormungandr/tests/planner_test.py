# encoding: utf-8

#  Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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
import pytest
from jormungandr.pt_planners.pt_planner import JourneyParameters, GraphicalIsochronesParameters
from jormungandr.utils import str_to_time_stamp, create_journeys_request, create_graphical_isochrones_request
import navitiacommon.type_pb2 as type_pb2


def check_basic_journeys_request(journeys_req):
    assert len(journeys_req.origin) == 1
    assert journeys_req.origin[0].place == "Kisio Digital"
    assert journeys_req.origin[0].access_duration == 42

    assert len(journeys_req.destination) == 1
    assert journeys_req.destination[0].place == "Somewhere"
    assert journeys_req.destination[0].access_duration == 666

    assert journeys_req.night_bus_filter_max_factor == 1.5
    assert journeys_req.night_bus_filter_base_factor == 900
    assert len(journeys_req.datetimes) == 1
    assert journeys_req.datetimes[0] == str_to_time_stamp("20120614T080000")
    assert journeys_req.clockwise is True
    assert journeys_req.realtime_level == type_pb2.BASE_SCHEDULE
    assert journeys_req.max_duration == 86400
    assert journeys_req.max_transfers == 10
    assert journeys_req.wheelchair is False
    assert journeys_req.max_extra_second_pass == 0
    assert journeys_req.forbidden_uris == []
    assert journeys_req.allowed_id == []
    assert journeys_req.direct_path_duration == 0
    assert journeys_req.bike_in_pt is False
    assert journeys_req.min_nb_journeys == 0
    assert journeys_req.timeframe_duration == 0
    assert journeys_req.depth == 1
    assert journeys_req.isochrone_center.place == ""
    assert journeys_req.isochrone_center.access_duration == 0


def check_graphical_isochrones_request(isochrone_request):
    check_basic_journeys_request(isochrone_request.journeys_request)

    assert len(isochrone_request.boundary_duration) == 2

    # To make sure the values are always in the same order.
    isochrone_request.boundary_duration.sort()
    assert isochrone_request.boundary_duration[0] == 0
    assert isochrone_request.boundary_duration[1] == 86400


def create_journeys_request_test():
    origin = {"Kisio Digital": 42}
    destination = {"Somewhere": 666}
    journey_parameters = JourneyParameters()
    datetime = str_to_time_stamp("20120614T080000")

    req = create_journeys_request(origin, destination, datetime, True, journey_parameters, False)

    assert req.requested_api == type_pb2.pt_planner
    check_basic_journeys_request(req.journeys)
    assert req.journeys.walking_transfer_penalty == 120
    assert req.journeys.arrival_transfer_penalty == 120


def test_journey_request_current_time():
    origin = {"Kisio Digital": 42}
    destination = {"Somewhere": 666}
    datetime = str_to_time_stamp("20120614T080000")
    journey_parameters = JourneyParameters(current_datetime=123456789)

    req = create_journeys_request(origin, destination, datetime, True, journey_parameters, False)
    assert req._current_datetime == 123456789


def create_graphical_isochrones_request_test():
    origin = {"Kisio Digital": 42}
    destination = {"Somewhere": 666}
    graphical_isochrones_parameters = GraphicalIsochronesParameters()
    datetime = str_to_time_stamp("20120614T080000")

    req = create_graphical_isochrones_request(
        origin, destination, datetime, True, graphical_isochrones_parameters, False
    )

    assert req.requested_api == type_pb2.graphical_isochrone
    check_graphical_isochrones_request(req.isochrone)


def test_journey_request_tranfer_penalties():
    origin = {"Kisio Digital": 42}
    destination = {"Somewhere": 666}
    journey_parameters = JourneyParameters(arrival_transfer_penalty=60, walking_transfer_penalty=240)
    datetime = str_to_time_stamp("20120614T080000")

    req = create_journeys_request(origin, destination, datetime, True, journey_parameters, False)

    assert req.requested_api == type_pb2.pt_planner
    check_basic_journeys_request(req.journeys)
    assert req.journeys.walking_transfer_penalty == 240
    assert req.journeys.arrival_transfer_penalty == 60
