# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Canal TP (www.canaltp.fr).
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
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from nose.tools import eq_

from jormungandr.scenarios import default
import navitiacommon.response_pb2 as response_pb2
from jormungandr.utils import str_to_time_stamp

def delete_non_optimal_journey_no_action_test():
    response = response_pb2.Response()
    journey = response.journeys.add()

    journey.type = "best"
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 120
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 300
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 120
    journey.duration = sum([section.duration for section in journey.sections])

    journey2 = response.journeys.add()
    journey2.type = "rapid"
    section = journey2.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 120
    section = journey2.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 300
    section = journey2.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 120
    journey2.duration = sum([section.duration for section in journey2.sections])

    scenario = default.Scenario()
    scenario._delete_non_optimal_journey(response.journeys)
    eq_(len(response.journeys), 2)

def delete_non_optimal_journey_simple_test():
    response = response_pb2.Response()
    journey1 = response.journeys.add()

    journey1.type = "best"
    section = journey1.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 120
    section = journey1.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 300
    section = journey1.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 120
    journey1.duration = sum([section.duration for section in journey1.sections])

    journey2 = response.journeys.add()
    journey2.type = "rapid"
    section = journey2.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 180
    section = journey2.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 300
    section = journey2.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 220
    journey2.duration = sum([section.duration for section in journey2.sections])

    journey3 = response.journeys.add()
    journey3.type = "non_pt_walk"
    section = journey3.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 250
    journey3.duration = sum([section.duration for section in journey3.sections])

    scenario = default.Scenario()
    scenario._delete_non_optimal_journey(response.journeys)
    eq_(len(response.journeys), 2)
    eq_(response.journeys[0], journey1)
    eq_(response.journeys[1], journey3)

def delete_non_optimal_journey_bike_test():
    response = response_pb2.Response()
    journey1 = response.journeys.add()

    journey1.type = "best"
    section = journey1.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 240
    section = journey1.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 300
    section = journey1.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 120
    journey1.duration = sum([section.duration for section in journey1.sections])

    journey2 = response.journeys.add()
    journey2.type = "rapid"
    section = journey2.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 120
    section = journey2.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 300
    section = journey2.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 220
    journey2.duration = sum([section.duration for section in journey2.sections])

    journey3 = response.journeys.add()
    journey3.type = "non_pt_walk"
    section = journey3.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 250
    journey3.duration = sum([section.duration for section in journey3.sections])

    journey4 = response.journeys.add()
    journey4.type = "non_pt_bike"
    section = journey4.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 140
    journey4.duration = sum([section.duration for section in journey4.sections])

    journey5 = response.journeys.add()
    journey5.type = "rapid"
    section = journey5.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 220
    section = journey5.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 600
    section = journey5.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 320
    journey5.duration = sum([section.duration for section in journey5.sections])

    scenario = default.Scenario()
    scenario._delete_non_optimal_journey(response.journeys)
    eq_(len(response.journeys), 3)
    eq_(response.journeys[0], journey2)
    eq_(response.journeys[1], journey3)
    eq_(response.journeys[2], journey4)
