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
from .tests_mechanism import AbstractTestFixture, dataset
from jormungandr.street_network.asgard import Asgard, DIRECT_PATH_ALTERNATIVES_PROFILES
import logging
from navitiacommon import response_pb2, type_pb2
import pytest


MOCKED_ASGARD_CONF = [
    {
        "modes": ['walking', 'car', 'bss', 'bike', 'car_no_park'],
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


def journey_response(journey, mode):
    map_mode_dist = {"walking": 200, "car": 50, "bike": 100, "car_no_park": 50}
    map_mode_time = {"walking": 2000, "car": 500, "bike": 1000, "car_no_park": 500}

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
    section.street_network.length = distance
    section.length = distance
    section.id = "section"
    section.begin_date_time = 1548669936
    section.end_date_time = 1548670472


def route_response(mode):
    response = response_pb2.Response()
    response.response_type = response_pb2.ITINERARY_FOUND
    journey = response.journeys.add()
    journey_response(journey, mode)
    return response


def bss_route_response(request):
    response = response_pb2.Response()
    response.response_type = response_pb2.ITINERARY_FOUND
    journey = response.journeys.add()
    journey.nb_transfers = 0
    journey.nb_sections = 5

    bss_rent_duration = int(request.direct_path.profiles_params[0].bss_rent_duration)
    bss_return_duration = int(request.direct_path.profiles_params[0].bss_return_duration)

    # 200 + bss_rent_duration + 1600 + bss_return_duration + 200
    duration = int(2000 + bss_rent_duration + bss_return_duration)

    journey.requested_date_time = journey.departure_date_time = int(1548669936)
    journey.arrival_date_time = int(journey.departure_date_time + duration)

    journey.duration = duration
    journey.durations.total = duration

    journey.durations.walking = int(400)
    journey.distances.walking = int(400)
    journey.durations.bike = int(1600)
    journey.distances.bike = int(1600)

    # Walking
    section = journey.sections.add()
    section.street_network.mode = response_pb2.Walking
    section.type = response_pb2.STREET_NETWORK
    section.duration = int(200)
    section.length = int(200)
    section.id = "section_0"
    section.begin_date_time = int(1548669936)
    section.end_date_time = int(section.begin_date_time + 200)

    # Rent
    section = journey.sections.add()
    section.type = response_pb2.BSS_RENT
    section.duration = bss_rent_duration
    section.id = "section_1"
    section.begin_date_time = journey.sections[-2].duration
    section.end_date_time = section.begin_date_time + bss_rent_duration

    # Bike
    section = journey.sections.add()
    section.street_network.mode = response_pb2.Bike
    section.type = response_pb2.STREET_NETWORK
    section.duration = int(1600)
    section.length = int(1600)
    section.id = "section_2"
    section.begin_date_time = journey.sections[-2].duration
    section.end_date_time = section.begin_date_time + 1600
    add_cycle_path_type_in_section(section)

    # Put back
    section = journey.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section.duration = bss_return_duration
    section.id = "section_3"
    section.begin_date_time = journey.sections[-2].duration
    section.end_date_time = section.begin_date_time + bss_return_duration

    # Walking
    section = journey.sections.add()
    section.street_network.mode = response_pb2.Walking
    section.type = response_pb2.STREET_NETWORK
    section.duration = int(200)
    section.length = int(200)
    section.id = "section_4"
    section.begin_date_time = journey.sections[-2].duration
    section.end_date_time = section.begin_date_time + 200

    return response


def alternative_route_response(profiles_params):
    response = response_pb2.Response()
    response.response_type = response_pb2.ITINERARY_FOUND
    for params in profiles_params:
        journey = response.journeys.add()
        journey.tags.append(params.profile_tag)
        journey_response(journey, params.origin_mode)
    return response


def valid_response(request):
    if request.requested_api == type_pb2.direct_path:
        if request.direct_path.profiles_params[0].origin_mode == "bss":
            return bss_route_response(request)
        if len(request.direct_path.profiles_params) == 1:
            return route_response(request.direct_path.profiles_params[0].origin_mode)
        if len(request.direct_path.profiles_params) == len(DIRECT_PATH_ALTERNATIVES_PROFILES):
            return alternative_route_response(request.direct_path.profiles_params)
    else:
        return response_pb2.Response()


def valid_request(request):
    if request.requested_api == type_pb2.direct_path:
        # direct path with alternatives
        if len(request.direct_path.profiles_params) == 1:
            assert len(request.direct_path.profiles_params) == 1
            params = request.direct_path.profiles_params[0]
            assert params.origin_mode
            assert params.destination_mode
            assert params.walking_speed == 1.12
            assert params.max_walking_duration_to_pt == 1800
            assert params.bike_speed == 4.1
            assert params.max_bike_duration_to_pt == 1800
            assert params.bss_speed == 4.1
            assert params.max_bss_duration_to_pt == 1800
            assert params.car_speed == 11.11
            assert params.max_car_duration_to_pt == 1800
            assert params.language == "en-US"
            assert params.car_no_park_speed == (11.11 if params.origin_mode in ("car", "car_no_park") else 0)
            assert params.max_car_no_park_duration_to_pt == (
                1800 if params.origin_mode in ("car", "car_no_park") else 0
            )
            assert params.bss_rent_duration == (42 if params.origin_mode == "bss" else 120)
            assert params.bss_rent_penalty == 0
            assert params.bss_return_duration == (24 if params.origin_mode == "bss" else 60)
            assert params.bss_return_penalty == 0
            assert params.enable_instructions
        # direct path with alternatives
        elif len(request.direct_path.profiles_params) == len(DIRECT_PATH_ALTERNATIVES_PROFILES):
            from jormungandr.street_network.asgard import COMFORT, SHORTEST, BALANCED

            for params in request.direct_path.profiles_params:
                if params.profile_tag == "comfort":
                    profile = COMFORT
                elif params.profile_tag == "shortest":
                    profile = SHORTEST
                elif params.profile_tag == "balanced":
                    profile = BALANCED
                else:
                    assert False, "Bad alternatives tag"

                assert params.origin_mode
                assert params.destination_mode
                assert params.walking_speed == 1.12
                assert params.max_walking_duration_to_pt == 1800
                assert params.bike_speed == 4.1
                assert params.max_bike_duration_to_pt == 1800
                assert params.bss_speed == 4.1
                assert params.max_bss_duration_to_pt == 1800
                assert params.car_speed == 11.11
                assert params.max_car_duration_to_pt == 1800
                assert params.language == "en-US"
                assert params.car_no_park_speed == (11.11 if params.origin_mode in ("car", "car_no_park") else 0)
                assert params.max_car_no_park_duration_to_pt == (
                    1800 if params.origin_mode in ("car", "car_no_park") else 0
                )
                assert params.bss_rent_duration == (42 if params.origin_mode == "bss" else 120)
                assert params.bss_rent_penalty == 0
                assert params.bss_return_duration == (24 if params.origin_mode == "bss" else 60)
                assert params.bss_return_penalty == 0
                assert params.enable_instructions

                assert params.bike_use_roads == pytest.approx(profile.bike_use_roads, 0.00001)
                assert params.bike_use_hills == profile.bike_use_hills
                assert params.bike_use_ferry == profile.bike_use_ferry
                assert params.bike_avoid_bad_surfaces == profile.bike_avoid_bad_surfaces
                assert params.bike_shortest == profile.bike_shortest
                assert params.bicycle_type == type_pb2.BicycleType.Value(profile.bicycle_type)
                assert params.bike_use_living_streets == pytest.approx(profile.bike_use_living_streets, 0.00001)
                assert params.bike_maneuver_penalty == profile.bike_maneuver_penalty
                assert params.bike_service_penalty == profile.bike_service_penalty
                assert params.bike_service_factor == profile.bike_service_factor
                assert params.bike_country_crossing_cost == profile.bike_country_crossing_cost
                assert params.bike_country_crossing_penalty == profile.bike_country_crossing_penalty
        else:
            assert False, "wrong number of profiles_params"

    if request.requested_api == type_pb2.street_network_routing_matrix:
        assert request.sn_routing_matrix.max_duration == 1800
        assert request.sn_routing_matrix.streetnetwork_params.origin_mode
        assert request.sn_routing_matrix.streetnetwork_params.walking_speed == 1.12
        assert request.sn_routing_matrix.streetnetwork_params.bike_speed == 4.1
        assert request.sn_routing_matrix.streetnetwork_params.bss_speed == 4.1
        assert request.sn_routing_matrix.streetnetwork_params.car_speed == 11.11
        assert request.sn_routing_matrix.streetnetwork_params.car_no_park_speed == 11.11
        assert request.direct_path.streetnetwork_params.bss_rent_duration == 120
        assert request.direct_path.streetnetwork_params.bss_rent_penalty == 0
        assert request.direct_path.streetnetwork_params.bss_return_duration == 120
        assert request.direct_path.streetnetwork_params.bss_return_penalty == 0


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
        valid_request(request)
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
            + "&first_section_mode[]=car_no_park"
            + "&forbidden_uris[]=stop_point:stopA"
            + "&forbidden_uris[]=stop_point:stopB"
        )
        response = self.query_region(query)
        check_journeys(response)

        assert len(response['journeys']) == 3

        # car direct path from asgard
        assert 'car_no_park' in response['journeys'][0]['tags']
        assert len(response['journeys'][0]['sections']) == 1
        assert response['journeys'][0]['duration'] == 500
        assert response['journeys'][0]['durations']['car'] == 500
        assert response['journeys'][0]['durations']['total'] == 500
        assert response['journeys'][0]['distances']['car'] == 50
        assert response['journeys'][0]['sections'][0]['co2_emission'] == {'value': 9.2, 'unit': 'gEC'}
        assert response['journeys'][0]['co2_emission'] == {'value': 9.2, 'unit': 'gEC'}
        assert not response['journeys'][0]['sections'][0].get('cycle_lane_length')

        # bike direct path from asgard
        assert 'bike' in response['journeys'][1]['tags']
        assert len(response['journeys'][1]['sections']) == 1
        assert response['journeys'][1]['duration'] == 1000
        assert response['journeys'][1]['durations']['bike'] == 1000
        assert response['journeys'][1]['durations']['total'] == 1000
        assert response['journeys'][1]['distances']['bike'] == 100
        assert response['journeys'][1]['duration'] == 1000
        assert response['journeys'][1]['sections'][0]['co2_emission'] == {'value': 0, 'unit': 'gEC'}
        assert response['journeys'][1]['co2_emission'] == {'value': 0, 'unit': 'gEC'}
        assert response['journeys'][1]['sections'][0]['cycle_lane_length'] == 30

        # walking direct path from asgard
        assert 'walking' in response['journeys'][2]['tags']
        assert len(response['journeys'][2]['sections']) == 1
        assert response['journeys'][2]['duration'] == 2000
        assert response['journeys'][2]['durations']['walking'] == 2000
        assert response['journeys'][2]['durations']['total'] == 2000
        assert response['journeys'][2]['distances']['walking'] == 200
        assert response['journeys'][2]['duration'] == 2000
        assert response['journeys'][2]['sections'][0]['co2_emission'] == {'value': 0, 'unit': 'gEC'}
        assert response['journeys'][2]['co2_emission'] == {'value': 0, 'unit': 'gEC'}
        assert not response['journeys'][2]['sections'][0].get('cycle_lane_length')

        assert not response.get('feed_publishers')

    def test_journey_with_bike_alternatives(self):
        """
        we want a bike direct path with alternatives
        """
        query = (
            journey_basic_query
            + "&first_section_mode[]=bike"
            + "&direct_path=only_with_alternatives"
            + "&forbidden_uris[]=stop_point:stopA"
            + "&forbidden_uris[]=stop_point:stopB"
        )
        response = self.query_region(query)
        check_journeys(response)

        assert len(response['journeys']) == len(DIRECT_PATH_ALTERNATIVES_PROFILES)

    def test_journey_bss_with_direct_path(self):
        """
        we only want direct path
        """
        bss_rent_duration = 42
        bss_return_duration = 24
        query = (
            journey_basic_query
            + "&first_section_mode[]=bss"
            + "&last_section_mode[]=bss"
            + "&bss_rent_duration={}".format(bss_rent_duration)
            + "&bss_return_duration={}".format(bss_return_duration)
            + "&forbidden_uris[]=stop_point:stopA"
            + "&forbidden_uris[]=stop_point:stopB"
        )
        response = self.query_region(query)
        check_journeys(response)

        assert len(response['journeys']) == 1

        # bss direct path from asgard
        assert 'bss' in response['journeys'][0]['tags']
        assert len(response['journeys'][0]['sections']) == 5
        assert response['journeys'][0]['duration'] == 2000 + bss_rent_duration + bss_return_duration
        assert response['journeys'][0]['durations']['walking'] == 400
        assert response['journeys'][0]['durations']['bike'] == 1600
        assert response['journeys'][0]['durations']['total'] == 2000 + bss_rent_duration + bss_return_duration
        assert response['journeys'][0]['distances']['walking'] == 400
        assert response['journeys'][0]['distances']['bike'] == 1600

        sections = response['journeys'][0]['sections']

        assert sections[1]['duration'] == bss_rent_duration
        assert sections[3]['duration'] == bss_return_duration

        # bike section
        assert sections[2].get('cycle_lane_length')
        # all other sections
        assert all((not sections[i].get('cycle_lane_length') for i in [0, 1, 3, 4]))

    def test_journey_with_max_direct_path_distance(self):
        """
        we only want direct path
        """
        query = journey_basic_query + "&direct_path=only"
        response = self.query_region(query)

        assert len(response['journeys']) == 1

        query = journey_basic_query + "&max_walking_direct_path_distance={}".format(2) + "&direct_path=only"
        response = self.query_region(query, check=False)

        assert response[0]['error']['message'] == "no solution found for this journey"
        assert response[1] == 200


@dataset(
    {
        'main_routing_test': {
            'scenario': 'distributed',
            'instance_config': {'street_network': MOCKED_ASGARD_CONF_WITH_BAD_RESPONSE},
        }
    }
)
class TestAsgardBadDirectPath(AbstractTestFixture):
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
        assert (
            response['journeys'][0]['sections'][0]['type'] == 'crow_fly'
        ), "A crow fly should replace the street network"
        assert response['journeys'][0]['sections'][1]['type'] == 'public_transport'
        assert (
            response['journeys'][0]['sections'][2]['type'] == 'crow_fly'
        ), "A crow fly should replace the street network"
