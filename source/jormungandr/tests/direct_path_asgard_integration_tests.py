# Copyright (c) 2001-2016, Canal TP and/or its affiliates. All rights reserved.
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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import
from .tests_mechanism import AbstractTestFixture, dataset
from jormungandr.street_network.asgard import Asgard
import logging
from navitiacommon import response_pb2, type_pb2

MOCKED_ASGARD_CONF = [
    {
        "modes": ['walking', 'car', 'bss', 'bike'],
        "class": "tests.direct_path_asgard_integration_tests.MockAsgard",
        "args": {
            "costing_options": {"bicycle": {"bicycle_type": "Hybrid"}},
            "api_key": "",
            "asgard_socket": "bob_socket",
            "service_url": "http://bob.com",
            "timeout": 10,
        },
    }
]
MOCKED_ASGARD_CONF_WITH_BAD_RESPONSE = [
    {
        "modes": ['walking', 'car', 'bss', 'bike'],
        "class": "tests.direct_path_asgard_integration_tests.MockAsgardWithBadResponse",
        "args": {
            "costing_options": {"bicycle": {"bicycle_type": "Hybrid"}},
            "api_key": "",
            "asgard_socket": "bob_socket",
            "service_url": "http://bob.com",
            "timeout": 10,
        },
    }
]
s_coord = "0.0000898312;0.0000898312"  # coordinate of S in the dataset
r_coord = "0.00188646;0.00071865"  # coordinate of R in the dataset

journey_basic_query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}".format(
    from_coord=s_coord, to_coord=r_coord, datetime="20120614T080000"
)

# Create 4 path items of 10 meters each.
# The first is not a cycle lane, the others are.
def add_cycle_path_type_in_section(section):
    path_item = section.street_network.path_items.add()
    path_item.length = 10
    path_item.cycle_path_type = response_pb2.NoCycleLane

    path_item = section.street_network.path_items.add()
    path_item.length = 10
    path_item.cycle_path_type = response_pb2.SharedCycleWay

    path_item = section.street_network.path_items.add()
    path_item.length = 10
    path_item.cycle_path_type = response_pb2.DedicatedCycleWay

    path_item = section.street_network.path_items.add()
    path_item.length = 10
    path_item.cycle_path_type = response_pb2.SeparatedCycleWay


def route_response(mode):
    map_mode_dist = {"walking": 200, "car": 50, "bike": 100}
    map_mode_time = {"walking": 2000, "car": 500, "bike": 1000}
    response = response_pb2.Response()
    response.response_type = response_pb2.ITINERARY_FOUND
    journey = response.journeys.add()
    journey.nb_transfers = 0
    journey.nb_sections = 1
    journey.departure_date_time = 1548669936
    journey.arrival_date_time = 1548670472
    journey.requested_date_time = 1548669936

    duration = map_mode_time.get(mode)
    distance = map_mode_dist.get(mode)
    journey.duration = duration
    journey.durations.total = duration
    section = journey.sections.add()
    add_cycle_path_type_in_section(section)
    if mode == "walking":
        journey.durations.walking = duration
        journey.distances.walking = distance
        section.street_network.mode = response_pb2.Walking
    elif mode == "car":
        journey.durations.car = duration
        journey.distances.car = distance
        section.street_network.mode = response_pb2.Car
    else:
        journey.durations.bike = duration
        journey.distances.bike = distance
        section.street_network.mode = response_pb2.Bike

    section.type = response_pb2.STREET_NETWORK
    section.duration = duration
    section.length = distance
    section.id = "section"
    section.begin_date_time = 1548669936
    section.end_date_time = 1548670472

    return response


def valid_response(request):
    if request.requested_api == type_pb2.direct_path:
        return route_response(request.direct_path.streetnetwork_params.origin_mode)
    else:
        return response_pb2.Response()


def bad_response(request):
    return response_pb2.Response()


def check_journeys(resp):
    assert not resp.get('journeys') or sum([1 for j in resp['journeys'] if j['type'] == "best"]) == 1


class MockAsgard(Asgard):
    def __init__(
        self, instance, service_url, asgard_socket, modes=None, id=None, timeout=10, api_key=None, **kwargs
    ):
        Asgard.__init__(
            self, instance, service_url, asgard_socket, modes or [], id or 'asgard', timeout, api_key, **kwargs
        )

    def _call_asgard(self, request):
        return valid_response(request)


class MockAsgardWithBadResponse(Asgard):
    def __init__(
        self, instance, service_url, asgard_socket, modes=None, id=None, timeout=10, api_key=None, **kwargs
    ):
        Asgard.__init__(
            self, instance, service_url, asgard_socket, modes or [], id or 'asgard', timeout, api_key, **kwargs
        )

    def _call_asgard(self, request):
        return bad_response(request)


@dataset(
    {'main_routing_test': {'scenario': 'distributed', 'instance_config': {'street_network': MOCKED_ASGARD_CONF}}}
)
class TestAsgardDirectPath(AbstractTestFixture):
    def test_journey_with_direct_path(self):
        """
        we only want direct path
        """
        query = (
            journey_basic_query
            + "&first_section_mode[]=walking"
            + "&first_section_mode[]=bike"
            + "&first_section_mode[]=car"
            + "&forbidden_uris[]=stop_point:stopA"
            + "&forbidden_uris[]=stop_point:stopB"
        )
        response = self.query_region(query)
        check_journeys(response)

        assert len(response['journeys']) == 3

        # car direct path from asgard
        assert 'car' in response['journeys'][0]['tags']
        assert len(response['journeys'][0]['sections']) == 1
        assert response['journeys'][0]['duration'] == 500
        assert response['journeys'][0]['durations']['car'] == 500
        assert response['journeys'][0]['durations']['total'] == 500
        assert response['journeys'][0]['distances']['car'] == 50
        assert not response['journeys'][0]['sections'][0].get('cycle_lane_length')

        # bike direct path from asgard
        assert 'bike' in response['journeys'][1]['tags']
        assert len(response['journeys'][1]['sections']) == 1
        assert response['journeys'][1]['duration'] == 1000
        assert response['journeys'][1]['durations']['bike'] == 1000
        assert response['journeys'][1]['durations']['total'] == 1000
        assert response['journeys'][1]['distances']['bike'] == 100
        assert response['journeys'][1]['duration'] == 1000
        assert response['journeys'][1]['sections'][0]['cycle_lane_length'] == 30

        # walking direct path from asgard
        assert 'walking' in response['journeys'][2]['tags']
        assert len(response['journeys'][2]['sections']) == 1
        assert response['journeys'][2]['duration'] == 2000
        assert response['journeys'][2]['durations']['walking'] == 2000
        assert response['journeys'][2]['durations']['total'] == 2000
        assert response['journeys'][2]['distances']['walking'] == 200
        assert response['journeys'][2]['duration'] == 2000
        assert not response['journeys'][2]['sections'][0].get('cycle_lane_length')


@dataset(
    {
        'main_routing_test': {
            'scenario': 'distributed',
            'instance_config': {'street_network': MOCKED_ASGARD_CONF_WITH_BAD_RESPONSE},
        }
    }
)
class TestAsgardDirectPath(AbstractTestFixture):
    def test_crowfly_replaces_section_if_street_network_failed(self):
        """
        Topic: This case arrives when the street network computation has failed.
               In this case, we replace the lost street network section by a crowfly, like in New Default
        """
        response, status = self.query_region(journey_basic_query, check=False)
        assert status == 200
        check_journeys(response)

        assert len(response['journeys']) == 1

        assert len(response['journeys'][0]['sections']) == 3
        assert response['journeys'][0]['sections'][0]['type'] == 'crow_fly'  # replace the street network
        assert response['journeys'][0]['sections'][1]['type'] == 'public_transport'
        assert response['journeys'][0]['sections'][2]['type'] == 'crow_fly'  # replace the street network
