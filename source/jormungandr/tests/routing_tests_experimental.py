# encoding: utf-8
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

from __future__ import absolute_import, print_function, unicode_literals, division
from jormungandr import app
from jormungandr.street_network.streetnetwork_backend_manager import StreetNetworkBackendManager
from navitiacommon.models.streetnetwork_backend import StreetNetworkBackend
from .tests_mechanism import config, NewDefaultScenarioAbstractTestFixture
from mock import MagicMock
from .journey_common_tests import *
import operator
import datetime
import math

"""
This unit runs all the common tests in journey_common_tests.py along with locals tests added in this
unit for scenario experimental
"""


@config(
    {
        "scenario": "distributed",
        "instance_config": {
            "street_network": [
                {
                    "modes": ["bike", "bss", "car", "walking"],
                    "class": "jormungandr.street_network.tests.MockKraken",
                }
            ]
        },
    }
)
class TestJourneysDistributedWithMock(JourneyMinBikeMinCar, NewDefaultScenarioAbstractTestFixture):
    def test_first_and_last_section_multi_modes(self):
        """Test to verify optimization of direct path calls"""
        # Initialize counter value in the object MockKraken
        sn_service = i_manager.instances['main_routing_test'].get_all_street_networks()[0]
        sn_service.direct_path_call_count = 0
        sn_service.routing_matrix_call_count = 0
        query = (
            "journeys?from={from_coord}&to={to_coord}&datetime={datetime}&"
            "first_section_mode[]=bike&first_section_mode[]=walking&"
            "last_section_mode[]=walking&min_nb_journeys=10&last_section_mode[]=bike&debug=true".format(
                from_coord=s_coord, to_coord=r_coord, datetime="20120614T075500"
            )
        )
        assert sn_service.direct_path_call_count == 0
        assert sn_service.routing_matrix_call_count == 0
        response = self.query_region(query)
        check_best(response)

        # Without optimization (context.partial_response_is_empty = True in distributed._compute_journeys()
        # journey count = 18 / direct_path_call_count = 26 / routing_matrix_call_count = 20
        # get_directpath_count_by_mode(response, 'walking') == 5
        # get_directpath_count_by_mode(response, 'bike') == 5
        assert len(response["journeys"]) == 8
        assert sn_service.direct_path_call_count == 4
        assert sn_service.routing_matrix_call_count == 4
        assert get_directpath_count_by_mode(response, 'walking') == 1
        assert get_directpath_count_by_mode(response, 'bike') == 1

        # This will call jormun so we check our counter before
        self.is_valid_journey_response(response, query)

    def test_first_and_last_section_multi_modes_no_debug(self):
        """Test to verify optimization of direct path calls"""
        # Initialize counter value in the object MockKraken
        sn_service = i_manager.instances['main_routing_test'].get_all_street_networks()[0]
        sn_service.direct_path_call_count = 0
        sn_service.routing_matrix_call_count = 0
        query = (
            "journeys?from={from_coord}&to={to_coord}&datetime={datetime}&"
            "first_section_mode[]=bike&first_section_mode[]=walking&"
            "last_section_mode[]=walking&min_nb_journeys=10&last_section_mode[]=bike".format(
                from_coord=s_coord, to_coord=r_coord, datetime="20120614T075500"
            )
        )
        assert sn_service.direct_path_call_count == 0
        assert sn_service.routing_matrix_call_count == 0
        response = self.query_region(query)
        check_best(response)

        # Without optimization (context.partial_response_is_empty = True in distributed._compute_journeys()
        # journey count = 18 / direct_path_call_count = 26 / routing_matrix_call_count = 20
        # get_directpath_count_by_mode(response, 'walking') == 5
        # get_directpath_count_by_mode(response, 'bike') == 5
        assert len(response["journeys"]) == 6
        assert sn_service.direct_path_call_count == 4
        assert sn_service.routing_matrix_call_count == 4
        assert get_directpath_count_by_mode(response, 'walking') == 1
        assert get_directpath_count_by_mode(response, 'bike') == 1

        # This will call jormun so we check our counter before
        self.is_valid_journey_response(response, query)


@config({'scenario': 'distributed'})
class TestJourneysDistributed(
    JourneyCommon,
    DirectPath,
    JourneyMinBikeMinCar,
    NewDefaultScenarioAbstractTestFixture,
    JourneysDirectPathMode,
):
    """
    Test the experiental scenario
    All the tests are defined in "TestJourneys" class, we only change the scenario


    NOTE: for the moment we cannot import all routing tests, so we only get 2, but we need to add some more
    """

    def test_journey_with_different_fallback_modes(self):
        """
        Test when departure/arrival fallback modes are different
        """
        query = (
            journey_basic_query
            + "&first_section_mode[]=walking&last_section_mode[]=car&debug=true&_car_park_duration=5"
        )
        response = self.query_region(query)
        check_best(response)
        jrnys = response['journeys']
        assert jrnys
        assert jrnys[0]['sections'][0]['mode'] == 'walking'
        assert jrnys[0]['sections'][-1]['mode'] == 'car'
        context = response['context']
        assert 'car_direct_path' in context
        assert 'co2_emission' in context['car_direct_path']
        assert len(response["terminus"]) == 1

    def test_journey_with_limited_nb_crowfly(self):
        """
        Test when max_nb_crowfly_by_car=0, we cannot fallback with car..
        """
        query = (
            journey_basic_query
            + "&first_section_mode[]=walking&last_section_mode[]=car&_max_nb_crowfly_by_car=0"
        )
        response = self.query_region(query)
        check_best(response)
        jrnys = response['journeys']
        assert len(jrnys) == 1
        assert jrnys[0]['sections'][0]['mode'] == 'walking'
        assert jrnys[0]['sections'][-1]['mode'] == 'walking'

        query = journey_basic_query + "&first_section_mode[]=walking&_max_nb_crowfly_by_walking=0"
        response = self.query_region(query)
        check_best(response)
        jrnys = response['journeys']
        assert len(jrnys) == 1
        assert 'walking' in jrnys[0]['tags']
        assert 'non_pt' in jrnys[0]['tags']

        """
        Test when max_nb_crowfly_by_walking=1
        """
        query = journey_basic_query + "&first_section_mode[]=walking&_max_nb_crowfly_by_walking=1"
        response = self.query_region(query)
        check_best(response)
        jrnys = response['journeys']
        assert len(jrnys) == 2
        # we should find at least one pt journey in the response
        assert any('non_pt' not in j['tags'] for j in jrnys)

    def test_best_filtering(self):
        """
        This feature is no longer supported"""
        pass

    def test_journeys_wheelchair_profile(self):
        """
        This feature is no longer supported
        """
        pass

    def test_not_existent_filtering(self):
        """
        This feature is no longer supported
        """
        pass

    def test_other_filtering(self):
        """
        This feature is no longer supported
        """
        pass

    def test_street_network_routing_matrix(self):

        from jormungandr import i_manager
        from navitiacommon import response_pb2

        instance = i_manager.instances['main_routing_test']
        origin = instance.georef.place("stopB", None)
        assert origin

        destination = instance.georef.place("stopA", None)
        assert destination

        max_duration = 18000
        mode = 'walking'
        kwargs = {
            "walking": instance.walking_speed,
            "bike": instance.bike_speed,
            "car": instance.car_speed,
            "bss": instance.bss_speed,
            "ridesharing": instance.car_no_park_speed,
            "taxi": instance.taxi_speed,
        }
        request = {
            "walking_speed": instance.walking_speed,
            "bike_speed": instance.bike_speed,
            "car_speed": instance.car_speed,
            "bss_speed": instance.bss_speed,
            "car_no_park_speed": instance.car_no_park_speed,
            "taxi_speed": instance.taxi_speed,
            "bss_rent_duration": instance.bss_rent_duration,
            "bss_rent_penalty": instance.bss_rent_penalty,
            "bss_return_duration": instance.bss_return_duration,
            "bss_return_penalty": instance.bss_return_penalty,
            "_asgard_max_walking_duration_coeff": 1,
            "_asgard_max_bike_duration_coeff": 1,
            "_asgard_max_bss_duration_coeff": 1,
            "_asgard_max_car_duration_coeff": 1,
        }
        service = instance.get_street_network(mode, request)
        request_id = None
        resp = service.get_street_network_routing_matrix(
            instance, [origin], [destination], mode, max_duration, request, request_id, **kwargs
        )
        assert len(resp.rows[0].routing_response) == 1
        assert resp.rows[0].routing_response[0].duration == 107
        assert resp.rows[0].routing_response[0].routing_status == response_pb2.reached

        max_duration = 106
        resp = service.get_street_network_routing_matrix(
            instance, [origin], [destination], mode, max_duration, request, request_id, **kwargs
        )
        assert len(resp.rows[0].routing_response) == 1
        assert resp.rows[0].routing_response[0].duration == 0
        assert resp.rows[0].routing_response[0].routing_status == response_pb2.unreached

    def test_intersection_objects(self):
        # The coordinates of arrival and the stop point are separated by 20m
        r = self.query(
            '/v1/coverage/main_routing_test/journeys?from=stopA&to=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&datetime=20120614080000&'
        )
        assert len(r['journeys'][0]['sections']) == 3

        # destination of crow_fly section and origin of next pt section should be the same object.
        assert r['journeys'][0]['sections'][0]['type'] == 'crow_fly'
        assert r['journeys'][0]['sections'][1]['type'] == 'public_transport'
        assert r['journeys'][0]['sections'][0]['to'] == r['journeys'][0]['sections'][1]['from']

        # destination of pt section and origin of next street_network section should be the same object.
        assert r['journeys'][0]['sections'][-1]['type'] == 'street_network'
        assert r['journeys'][0]['sections'][1]['to'] == r['journeys'][0]['sections'][-1]['from']

        r = self.query(
            '/v1/coverage/main_routing_test/journeys?from=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&to=stopA&datetime=20120614080000'
        )
        assert len(r['journeys'][0]['sections']) == 3

        # destination of crow_fly section and origin of next pt section should be the same object.
        assert r['journeys'][0]['sections'][0]['type'] == 'street_network'
        assert r['journeys'][0]['sections'][1]['type'] == 'public_transport'
        assert r['journeys'][0]['sections'][0]['to'] == r['journeys'][0]['sections'][1]['from']

        # destination of pt section and origin of next street_network section should be the same object.
        assert r['journeys'][0]['sections'][-1]['type'] == 'crow_fly'
        assert r['journeys'][0]['sections'][1]['to'] == r['journeys'][0]['sections'][-1]['from']

    def test_walking_bss_duplicate_journey(self):
        query = (
            '/v1/coverage/main_routing_test/journeys?'
            'from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&'
            'datetime=20120614T080000&'
            'first_section_mode[]=walking&first_section_mode[]=bss&'
            'last_section_mode[]=walking&last_section_mode[]=bss&'
            'bss_speed=1&walking_speed=0.2735'
        )

        r = self.query(query)
        # only one pt in the response
        assert sum(int('non_pt' not in j['tags']) for j in r['journeys']) == 1
        # The pt_journey is computed by the BSS mode, since there is no bss section, the travel mode should be walking
        pt_journey = next((j for j in r['journeys'] if 'non_pt' not in j['tags']), None)
        assert pt_journey['sections'][0]['mode'] == 'walking'
        assert pt_journey['sections'][-1]['mode'] == 'walking'
        assert len(r["terminus"]) == 1
        assert r["terminus"][0]["id"] == "stopA"

        r = self.query(query + "&debug=true")
        # find all pt_journeys
        pt_journeys = [j for j in r['journeys'] if 'non_pt' not in j['tags']]

        # should be two pt in the response
        assert len(pt_journeys) == 2

        pt_1 = pt_journeys[0]
        pt_2 = pt_journeys[1]

        for s1, s2 in zip(pt_1['sections'], pt_2['sections']):
            assert s1['type'] == s2['type']
            if s1['type'] == 'public_transport':
                s1_vj = next(l['id'] for l in s1['links'] if l['type'] == 'vehicle_journey')
                s2_vj = next(l['id'] for l in s2['links'] if l['type'] == 'vehicle_journey')
                assert s1_vj == s2_vj

        # there should be one journey deleted because of duplicate journey
        assert sum(int('deleted_because_duplicate_journey' in j['tags']) for j in r['journeys']) == 1

    def test_walking_bss_override_mode_journey(self):
        query = (
            '/v1/coverage/main_routing_test/journeys?'
            'from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&'
            'datetime=20120614T080000&'
            'first_section_mode[]=walking&first_section_mode[]=bss&'
            'last_section_mode[]=walking&last_section_mode[]=bss&'
            'bss_speed=1&walking_speed=0.2735&debug=true'
        )
        # for the first request, the walking duration to the stop_point is equal to the bss duration
        r = self.query(query)
        assert sum(int('non_pt' not in j['tags']) for j in r['journeys']) == 2

        debug = "true"

        query = (
            '/v1/coverage/main_routing_test/journeys?'
            'from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&'
            'datetime=20120614T080000&'
            'first_section_mode[]=walking&first_section_mode[]=bss&'
            'last_section_mode[]=walking&last_section_mode[]=bss&'
            'bss_speed=1&walking_speed=1&debug={debug}'.format(debug=debug)
        )
        r = self.query(query)
        assert sum(int('non_pt' not in j['tags']) for j in r['journeys']) == 2

        debug = "false"

        query = (
            '/v1/coverage/main_routing_test/journeys?'
            'from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&'
            'datetime=20120614T080000&'
            'first_section_mode[]=walking&first_section_mode[]=bss&'
            'last_section_mode[]=walking&last_section_mode[]=bss&'
            'bss_speed=1&walking_speed=1&debug={debug}'.format(debug=debug)
        )
        r = self.query(query)
        assert sum(int('non_pt' not in j['tags']) for j in r['journeys']) == 1

    def test_duplicate_journey_crow_fly_walking_bss(self):
        query_template = (
            '/v1/coverage/main_routing_test/journeys?'
            'from=0.000898311981954709;0.0008084807837592382&to=0.00898311981954709;0.0001796623963909418&'
            'datetime=20120614T080000&'
            'first_section_mode[]=walking&first_section_mode[]=bss&'
            'last_section_mode[]=walking&last_section_mode[]=bss&'
            'bss_speed=1.5&walking_speed=1&_final_line_filter=true&debug=true'
        )
        r = self.query(query_template.format(debug='false'))
        # should be two journeys in the response
        assert len(r['journeys']) == 2

        # first journey should finish with a crow_fly bss (fallback)
        # So it should be also tagged 'to_delete' & 'deleted_because_duplicate_journey'
        assert r['journeys'][0]['sections'][-1]['type'] == 'crow_fly'
        assert r['journeys'][0]['sections'][-1]['mode'] == 'bss'
        assert 'deleted_because_duplicate_journey' in r['journeys'][0]['tags']
        assert 'to_delete' in r['journeys'][0]['tags']

        # second journey should finish with a crow_fly walking (fallback), so we keep it
        assert r['journeys'][1]['sections'][-1]['type'] == 'crow_fly'
        assert r['journeys'][1]['sections'][-1]['mode'] == 'walking'
        assert 'deleted_because_duplicate_journey' not in r['journeys'][1]['tags']
        assert 'to_delete' not in r['journeys'][1]['tags']

    def test_direct_path_bss_bike(self):
        query = (
            journey_basic_query
            + "&direct_path_mode[]=bike"
            + "&direct_path_mode[]=bss"
            + "&max_duration_to_pt=0"
        )
        response = self.query_region(query)
        assert len(response['journeys']) == 2

        # we should find two journeys: one is bike and another is bss
        assert any('bss' in j['tags'] for j in response['journeys'])
        assert any('bike' in j['tags'] for j in response['journeys'])

    def test_direct_path_bss_walking(self):
        # In case we ask for :
        # - direct_path_mode=walking
        # - first_section_mode=walking&bss
        # - last_section_mode=bss
        # We expect to have a walking direct_path journey
        query = (
            journey_basic_query
            + "&direct_path_mode[]=walking"
            + "&first_section_mode[]=walking&first_section_mode[]=bss"
            + "&last_section_mode[]=bss"
        )
        response = self.query_region(query)
        assert len(response['journeys']) == 2

        # we should find at least a walking direct_path journey
        assert any('non_pt_walking' in j['tags'] for j in response['journeys'])
        assert any('walking' in j['tags'] for j in response['journeys'])
        assert len(response['journeys'][1]['sections']) == 1

    def test_journey_with_access_points(self):
        query = journey_basic_query + "&_access_points=true&language=en-US"
        response = self.query_region(query)
        assert len(response['journeys']) == 2

        pt_journey = next((j for j in response['journeys'] if 'non_pt' not in j['tags']), None)
        assert pt_journey
        assert len(pt_journey['sections'][0]['vias']) == 1

        pathway = pt_journey['sections'][0]['vias'][0]
        access_point = pathway['access_point']
        assert access_point["id"] == "access_point:B1"
        assert pathway["is_entrance"]
        assert not pathway["is_exit"]
        assert pathway["traversal_time"] == 2
        assert pathway["length"] == 1

        path = pt_journey['sections'][0]['path'][-1]
        assert path['duration'] == 2
        assert path['length'] == 1
        assert path['via_uri'] == "access_point:B1"
        assert path['instruction'] == "Then enter stop_point:stopB (Condom) via access_point:B1."

        path_sum = sum(p['duration'] for p in pt_journey['sections'][0]['path'])
        assert pt_journey['sections'][0]['duration'] == pytest.approx(path_sum, 1.0)

        pathway = pt_journey['sections'][2]['vias'][0]
        access_point = pathway['access_point']
        assert access_point["id"] == "access_point:A2"
        assert not pathway["is_entrance"]
        assert pathway["is_exit"]
        assert pathway["traversal_time"] == 4
        assert pathway["length"] == 3

        path = pt_journey['sections'][2]['path'][0]
        assert path['duration'] == 4
        assert path['length'] == 3
        assert path['via_uri'] == "access_point:A2"
        assert path['instruction'] == "Exit stop_point:stopA (Condom) via access_point:A2."

        path_sum = sum(p['duration'] for p in pt_journey['sections'][2]['path'])
        assert pt_journey['sections'][2]['duration'] == pytest.approx(path_sum, 1.0)

    def test_path_instructions(self):
        # Verify some path instructions managed by jormungandr in English
        query = journey_basic_query + "&_access_points=true&language=en-US"
        response = self.query_region(query)
        assert len(response['journeys']) == 2

        pt_journey = next((j for j in response['journeys'] if 'non_pt' not in j['tags']), None)
        assert pt_journey
        assert len(pt_journey['sections'][0]['vias']) == 1
        path = pt_journey['sections'][0]['path'][-1]
        assert path['instruction'] == "Then enter stop_point:stopB (Condom) via access_point:B1."
        path = pt_journey['sections'][2]['path'][0]
        assert path['instruction'] == "Exit stop_point:stopA (Condom) via access_point:A2."

        # Verify some path instructions managed by jormungandr in French
        query = journey_basic_query + "&_access_points=true"
        response = self.query_region(query)
        assert len(response['journeys']) == 2

        pt_journey = next((j for j in response['journeys'] if 'non_pt' not in j['tags']), None)
        assert pt_journey
        assert len(pt_journey['sections'][0]['vias']) == 1
        path = pt_journey['sections'][0]['path'][-1]
        assert path['instruction'] == "Accédez à stop_point:stopB (Condom) via access_point:B1."
        path = pt_journey['sections'][2]['path'][0]
        assert path['instruction'] == "Sortez de stop_point:stopA (Condom) via access_point:A2."

        # Verify some path instructions managed by jormungandr in English as default language
        query = journey_basic_query + "&_access_points=true&language=ja-JP"
        response = self.query_region(query)
        assert len(response['journeys']) == 2

        pt_journey = next((j for j in response['journeys'] if 'non_pt' not in j['tags']), None)
        assert pt_journey
        assert len(pt_journey['sections'][0]['vias']) == 1
        path = pt_journey['sections'][0]['path'][-1]
        assert path['instruction'] == "Then enter stop_point:stopB (Condom) via access_point:B1."
        path = pt_journey['sections'][2]['path'][0]
        assert path['instruction'] == "Exit stop_point:stopA (Condom) via access_point:A2."

    def test_last_and_first_coord_in_geojson(self):
        """
        Test when departure/arrival fallback modes are different
        """
        query = journey_basic_query + "&first_section_mode[]=walking"
        response = self.query_region(query)
        check_best(response)
        jrnys = response['journeys']
        pt_journey = next((j for j in jrnys if 'non_pt' not in j['tags']), None)
        assert pt_journey
        from_coord = pt_journey['sections'][0]['from']['address']['coord']
        to_coord = pt_journey['sections'][0]['to']['stop_point']['coord']

        coords = pt_journey['sections'][0]['geojson']['coordinates']

        assert coords[0][0] == pytest.approx(float(from_coord['lon']), 0.1e-5)
        assert coords[0][1] == pytest.approx(float(from_coord['lat']), 0.1e-5)

        assert coords[-1][0] == pytest.approx(float(to_coord['lon']), 0.1e-5)
        assert coords[-1][1] == pytest.approx(float(to_coord['lat']), 0.1e-5)


@config({"scenario": "distributed"})
class TestDistributedJourneysWithPtref(JourneysWithPtref, NewDefaultScenarioAbstractTestFixture):
    pass


@config({"scenario": "distributed"})
class TestDistributedOnBasicRouting(OnBasicRouting, NewDefaultScenarioAbstractTestFixture):
    def test_isochrone(self):
        super(TestDistributedOnBasicRouting, self).test_isochrone()


@config({"scenario": "distributed"})
class TestDistributedMinNbJourneys(JourneysMinNbJourneys, NewDefaultScenarioAbstractTestFixture):
    pass


@config({"scenario": "distributed"})
class TestDistributedWithNightBusFilter(JourneysWithNightBusFilter, NewDefaultScenarioAbstractTestFixture):
    pass


@config({"scenario": "distributed"})
class TestDistributedTimeFrameDuration(JourneysTimeFrameDuration, NewDefaultScenarioAbstractTestFixture):
    pass


@config({"scenario": "distributed"})
class TestDistributedJourneyTickets(JourneysTickets, NewDefaultScenarioAbstractTestFixture):
    def test_journey_tickets_pt_journey_fare(self):
        """
        test tickets with pt_journey_fare
        in the first request, we will obtain a journey with default tickets,
        in the second request, we activate _pt_journey_fare and we will obtain a journey with the same tickets that should
        appear twice
        """
        query = 'journeys?from=2.39592;48.84838&to=2.36381;48.86750&datetime=20180309T080000'
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)

        # Tickets
        default_tickets = response['tickets']
        assert len(default_tickets) == 2
        assert default_tickets[0]['name'] == 'A-Ticket name'
        assert default_tickets[0]['source_id'] == 'A-Ticket'
        assert default_tickets[0]['comment'] == 'A-Ticket comment'
        assert default_tickets[1]['name'] == 'B-Ticket name'
        assert default_tickets[1]['source_id'] == 'B-Ticket'
        assert default_tickets[1]['comment'] == 'B-Ticket comment'
        # Links between journeys.fare and journeys.tickets
        assert default_tickets[0]['id'] == response['journeys'][1]['fare']['links'][0]['id']
        assert default_tickets[1]['id'] == response['journeys'][0]['fare']['links'][0]['id']

        query_pt_journey_fare = query + "&_pt_journey_fare=kraken&_compute_pt_journey_fare=true"
        response = self.query_region(query_pt_journey_fare)

        new_tickets = response['tickets']
        # for each ticket in default_tickets, the ticket should be found twice in new_tickets
        for ticket in default_tickets:
            count = sum(int(ticket == new_ticket) for new_ticket in new_tickets)
            assert count == 2


@config({"scenario": "distributed"})
class TestDistributedJourneyTicketsWithDebug(JourneysTicketsWithDebug, NewDefaultScenarioAbstractTestFixture):
    pass


def _make_function_distance_over_upper_limit(from_coord, to_coord, mode, op):
    def test_ko_crow_fly_longer_than_max_mode_direct_path_distance(self):
        query = (
            'journeys?'
            'from={from_coord}'
            '&to={to_coord}'
            '&datetime={datetime}'
            '&first_section_mode[]={mode}'
            '&last_section_mode[]={mode}'
            '&max_duration=0'
            '&_min_bike=0'
            '&_min_car=0'
            '&_min_taxi=0'
        ).format(from_coord=from_coord, to_coord=to_coord, datetime="20120614T080000", mode=mode)

        response = self.query_region(query)

        assert len(response['journeys']) == 1
        assert mode in response['journeys'][0]['tags']
        assert 'non_pt' in response['journeys'][0]['tags']

        direct_path_distance = response['journeys'][0]['distances'][mode]

        # crow_fly is unknown so we divide distance in response by 10
        # so we make sure we are under crow_fly distance
        query = (query + '&max_{mode}_direct_path_distance={max_dp_distance}').format(
            mode=mode, max_dp_distance=int(direct_path_distance / 10)
        )
        response = self.query_region(query)
        # New Default -> 'journeys' in response
        # Distributed -> 'journeys' not in response
        assert op('journeys' not in response)

    return test_ko_crow_fly_longer_than_max_mode_direct_path_distance


@dataset({"main_routing_test": {"scenario": "distributed"}})
class TestDistributedMaxDistanceForDirectPathUpperLimit(NewDefaultScenarioAbstractTestFixture):
    """
    Test max_{mode}_direct_path_distance's upper limit
    Direct path should be filtered if its crow_fly distance is greater than max_{mode}_direct_path_distance
    """

    s = '8.98311981954709e-05;8.98311981954709e-05'
    r = '0.0018864551621048887;0.0007186495855637672'
    test_max_walking_direct_path_distance = _make_function_distance_over_upper_limit(
        s, r, 'walking', operator.truth
    )
    test_max_car_direct_path_distance = _make_function_distance_over_upper_limit(s, r, 'car', operator.truth)
    test_max_bike_direct_path_distance = _make_function_distance_over_upper_limit(s, r, 'bike', operator.truth)

    a = '0.001077974378345651;0.0007186495855637672'
    b = '8.98311981954709e-05;0.0002694935945864127'
    test_max_taxi_direct_path_distance = _make_function_distance_over_upper_limit(a, b, 'taxi', operator.truth)


def _make_function_distance_under_lower_limit(from_coord, to_coord, mode):
    def test_ko_crow_fly_smaller_than_max_mode_direct_path_distance(self):
        query = (
            'journeys?'
            'from={from_coord}'
            '&to={to_coord}'
            '&datetime={datetime}'
            '&first_section_mode[]={mode}'
            '&last_section_mode[]={mode}'
            '&max_duration=0'
            '&max_{mode}_direct_path_distance={max_dp_distance}'
            '&_min_bike=0'
            '&_min_car=0'
            '&_min_taxi=0'
        ).format(
            from_coord=from_coord,
            to_coord=to_coord,
            datetime="20120614T080000",
            mode=mode,
            max_dp_distance=50000,
        )

        response = self.query_region(query)

        assert len(response['journeys']) == 1
        assert mode in response['journeys'][0]['tags']
        assert 'non_pt' in response['journeys'][0]['tags']

    return test_ko_crow_fly_smaller_than_max_mode_direct_path_distance


@dataset({"main_routing_test": {"scenario": "distributed"}})
class TestDistributedMaxDistanceForDirectPathLowerLimit(NewDefaultScenarioAbstractTestFixture):
    """
    Test max_{mode}_direct_path_distance's lower limit
    Direct path should be found if its crow_fly distance is lower than max_{mode}_direct_path_duration.
    """

    s = '8.98311981954709e-05;8.98311981954709e-05'
    r = '0.0018864551621048887;0.0007186495855637672'
    test_max_walking_direct_path_distance = _make_function_distance_under_lower_limit(s, r, 'walking')
    test_max_car_direct_path_distance = _make_function_distance_under_lower_limit(s, r, 'car')
    test_max_bike_direct_path_distance = _make_function_distance_under_lower_limit(s, r, 'bike')

    a = '0.001077974378345651;0.0007186495855637672'
    b = '8.98311981954709e-05;0.0002694935945864127'
    test_max_taxi_direct_path_distance = _make_function_distance_under_lower_limit(a, b, 'taxi')


@dataset({"main_routing_test": {"scenario": "new_default"}})
class TestNewDefaultMaxDistanceForDirectPath(NewDefaultScenarioAbstractTestFixture):
    """
    the max_{mode}_direct_path_distance should be deactivated in new_default
    """

    s = '8.98311981954709e-05;8.98311981954709e-05'
    r = '0.0018864551621048887;0.0007186495855637672'
    test_max_walking_direct_path_duration = _make_function_distance_over_upper_limit(
        s, r, 'walking', operator.not_
    )
    test_max_car_direct_path_duration = _make_function_distance_over_upper_limit(s, r, 'car', operator.not_)
    test_max_bike_direct_path_duration = _make_function_distance_over_upper_limit(s, r, 'bike', operator.not_)


def _make_function_duration_over_upper_limit(from_coord, to_coord, mode, op):
    def test_ko_direct_path_longer_than_max_mode_direct_path_duration(self):
        query = (
            'journeys?'
            'from={from_coord}'
            '&to={to_coord}'
            '&datetime={datetime}'
            '&first_section_mode[]={mode}'
            '&last_section_mode[]={mode}'
            '&max_duration=0'
            '&_min_bike=0'
            '&_min_car=0'
            '&_min_taxi=0'
        ).format(from_coord=from_coord, to_coord=to_coord, datetime="20120614T080000", mode=mode)

        response = self.query_region(query)

        assert len(response['journeys']) == 1
        assert mode in response['journeys'][0]['tags']
        assert 'non_pt' in response['journeys'][0]['tags']

        direct_path_duration = response['journeys'][0]['duration']

        query = (query + '&max_{mode}_direct_path_duration={max_dp_duration}').format(
            mode=mode, max_dp_duration=direct_path_duration - 1
        )
        response = self.query_region(query)
        # New Default -> 'journeys' in response
        # Distributed -> 'journeys' not in response
        assert op('journeys' not in response)

    return test_ko_direct_path_longer_than_max_mode_direct_path_duration


@dataset({"main_routing_test": {"scenario": "distributed"}})
class TestDistributedMaxDurationForDirectPathUpperLimit(NewDefaultScenarioAbstractTestFixture):
    """
    Test max_{mode}_direct_path_duration's upper limit

    Direct path should be filtered if its duration is greater than max_{mode}_direct_path_duration
    """

    s = '8.98311981954709e-05;8.98311981954709e-05'
    r = '0.0018864551621048887;0.0007186495855637672'
    test_max_walking_direct_path_duration = _make_function_duration_over_upper_limit(
        s, r, 'walking', operator.truth
    )
    test_max_car_direct_path_duration = _make_function_duration_over_upper_limit(s, r, 'car', operator.truth)
    test_max_bss_direct_path_duration = _make_function_duration_over_upper_limit(s, r, 'bss', operator.truth)
    test_max_bike_direct_path_duration = _make_function_duration_over_upper_limit(s, r, 'bike', operator.truth)

    a = '0.001077974378345651;0.0007186495855637672'
    b = '8.98311981954709e-05;0.0002694935945864127'
    test_max_taxi_direct_path_duration = _make_function_duration_over_upper_limit(a, b, 'taxi', operator.truth)


def _make_function_duration_under_upper_limit(from_coord, to_coord, mode):
    def test_get_direct_path_smaller_than_max_mode_direct_path_duration(self):
        query = (
            'journeys?'
            'from={from_coord}'
            '&to={to_coord}'
            '&datetime={datetime}'
            '&first_section_mode[]={mode}'
            '&last_section_mode[]={mode}'
            '&max_duration=0'
            '&{mode}_speed=1'
            '&max_{mode}_direct_path_duration={max_dp_duration}'
            '&_min_bike=0'
            '&_min_car=0'
            '&_min_taxi=0'
        ).format(
            from_coord=from_coord, to_coord=to_coord, datetime="20120614T080000", mode=mode, max_dp_duration=3600
        )

        response = self.query_region(query)

        assert len(response['journeys']) == 1
        assert mode in response['journeys'][0]['tags']
        assert 'non_pt' in response['journeys'][0]['tags']

    return test_get_direct_path_smaller_than_max_mode_direct_path_duration


@dataset({"main_routing_test": {"scenario": "distributed"}})
class TestDistributedMaxDurationForDirectPathLowerLimit(NewDefaultScenarioAbstractTestFixture):
    """
    Test max_{mode}_direct_path_duration's lower limit

    Direct path should be found if its duration is lower than max_{mode}_direct_path_duration.
    Especially, when the direct path's duration is large and the street network calculator is Kraken
    """

    s = '8.98311981954709e-05;8.98311981954709e-05'
    r = '0.0018864551621048887;0.0007186495855637672'
    test_max_walking_direct_path_duration = _make_function_duration_under_upper_limit(s, r, 'walking')
    test_max_car_direct_path_duration = _make_function_duration_under_upper_limit(s, r, 'car')
    test_max_bss_direct_path_duration = _make_function_duration_under_upper_limit(s, r, 'bss')
    test_max_bike_direct_path_duration = _make_function_duration_under_upper_limit(s, r, 'bike')

    a = '0.001077974378345651;0.0007186495855637672'
    b = '8.98311981954709e-05;0.0002694935945864127'
    test_max_taxi_direct_path_duration = _make_function_duration_under_upper_limit(a, b, 'taxi')


@dataset({"main_routing_test": {"scenario": "new_default"}})
class TestNewDefaultMaxDurationForDirectPath(NewDefaultScenarioAbstractTestFixture):
    """
    the max_{mode}_direct_path_duration should be deactivated in new_default
    """

    s = '8.98311981954709e-05;8.98311981954709e-05'
    r = '0.0018864551621048887;0.0007186495855637672'
    test_max_walking_direct_path_duration = _make_function_duration_over_upper_limit(
        s, r, 'walking', operator.not_
    )
    test_max_car_direct_path_duration = _make_function_duration_over_upper_limit(s, r, 'car', operator.not_)
    test_max_bss_direct_path_duration = _make_function_duration_over_upper_limit(s, r, 'bss', operator.not_)
    test_max_bike_direct_path_duration = _make_function_duration_over_upper_limit(s, r, 'bike', operator.not_)


@config(
    {
        "scenario": "distributed",
        'instance_config': {
            "ridesharing": [
                {
                    "class": "jormungandr.scenarios.ridesharing.instant_system.InstantSystem",
                    "args": {
                        "service_url": "http://distributed_ridesharing.wtf",
                        "api_key": "key",
                        "network": "Super Covoit 3000",
                        "rating_scale_min": 0,
                        "rating_scale_max": 5,
                    },
                }
            ]
        },
    }
)
class TestJourneysRidesharingDistributed(
    JourneysRidesharing, JourneyCommon, DirectPath, JourneyMinBikeMinCar, NewDefaultScenarioAbstractTestFixture
):
    def test_best_filtering(self):
        """
        This feature is not supported
        """
        pass

    def test_journeys_wheelchair_profile(self):
        """
        This feature is not supported
        """
        pass

    def test_not_existent_filtering(self):
        """
        This feature is not supported
        """
        pass

    def test_other_filtering(self):
        """
        This feature is not supported
        """
        pass


@dataset({"main_routing_test": {"scenario": "distributed"}})
class TestTaxiDistributed(NewDefaultScenarioAbstractTestFixture):
    def test_first_section_mode_taxi(self):
        query = sub_query + "&datetime=20120614T075000" + "&first_section_mode[]=taxi" + "&debug=true"

        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        journeys = get_not_null(response, 'journeys')
        assert len(journeys) == 1

        taxi_direct = journeys[0]

        assert taxi_direct.get('departure_date_time') == '20120614T075000'
        assert taxi_direct.get('arrival_date_time') == '20120614T075027'
        assert taxi_direct.get('duration') == 27
        assert taxi_direct.get('durations', {}).get("car") == 0
        assert taxi_direct.get('durations', {}).get("taxi") == 27
        assert taxi_direct.get('durations', {}).get("total") == 27
        assert taxi_direct.get('distances', {}).get("car") == 0
        assert taxi_direct.get('distances', {}).get("taxi") == 304

        sections = taxi_direct.get('sections')
        assert len(sections) == 1
        assert sections[0].get('mode') == 'taxi'
        assert sections[0].get('departure_date_time') == '20120614T075000'
        assert sections[0].get('arrival_date_time') == '20120614T075027'
        assert sections[0].get('duration') == 27
        assert sections[0].get('type') == 'street_network'

        query += "&taxi_speed=0.15"

        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        journeys = get_not_null(response, 'journeys')
        assert len(journeys) == 2

        taxi_direct = next((j for j in journeys if 'non_pt' in j['tags']), None)
        assert taxi_direct

        assert taxi_direct.get('departure_date_time') == '20120614T075000'
        assert taxi_direct.get('arrival_date_time') == '20120614T082349'
        assert taxi_direct.get('duration') == 2029
        assert taxi_direct.get('durations', {}).get("car") == 0
        assert taxi_direct.get('durations', {}).get("taxi") == 2029
        assert taxi_direct.get('durations', {}).get("total") == 2029
        assert taxi_direct.get('distances', {}).get("car") == 0
        assert taxi_direct.get('distances', {}).get("taxi") == 304
        sections = taxi_direct.get('sections')
        assert len(sections) == 1
        assert sections[0].get('mode') == 'taxi'
        assert sections[0].get('departure_date_time') == '20120614T075000'
        assert sections[0].get('arrival_date_time') == '20120614T082349'
        assert sections[0].get('duration') == 2029
        assert sections[0].get('type') == 'street_network'

        taxi_fallback = next((j for j in journeys if 'non_pt' not in j['tags']), None)
        assert taxi_fallback

        assert taxi_fallback.get('departure_date_time') == '20120614T075355'
        assert taxi_fallback.get('arrival_date_time') == '20120614T080222'

        assert taxi_fallback.get('durations', {}).get('taxi') == 125
        assert taxi_fallback.get('durations', {}).get('walking') == 80

        assert taxi_fallback.get('distances', {}).get('taxi') == 18
        assert taxi_fallback.get('distances', {}).get('walking') == 89

        sections = taxi_fallback.get('sections')
        assert len(sections) == 4
        assert sections[0].get('mode') == 'taxi'
        assert sections[0].get('departure_date_time') == '20120614T075355'
        assert sections[0].get('arrival_date_time') == '20120614T075600'
        assert sections[0].get('duration') == 125
        assert sections[0].get('type') == 'street_network'

        assert sections[1].get('departure_date_time') == '20120614T075600'
        assert sections[1].get('arrival_date_time') == '20120614T080100'
        assert sections[1].get('duration') == 300
        assert sections[1].get('type') == 'waiting'

        assert sections[2].get('departure_date_time') == '20120614T080100'
        assert sections[2].get('arrival_date_time') == '20120614T080102'
        assert sections[2].get('duration') == 2
        assert sections[2].get('type') == 'public_transport'

        assert sections[3].get('mode') == 'walking'
        assert sections[3].get('departure_date_time') == '20120614T080102'
        assert sections[3].get('arrival_date_time') == '20120614T080222'
        assert sections[3].get('duration') == 80
        assert sections[3].get('type') == 'street_network'

        query += "&max_duration=0"
        response = self.query_region(query)
        # the pt journey is eliminated
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1

    def test_last_section_mode_taxi(self):
        query = journey_basic_query + "&walking_speed=0.5" + "&last_section_mode[]=taxi&_min_taxi=0"

        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        journeys = get_not_null(response, 'journeys')
        assert len(journeys) == 2

        taxi_fallback = next((j for j in journeys if 'non_pt' not in j['tags']), None)
        assert taxi_fallback

        assert taxi_fallback.get('departure_date_time') == '20120614T080021'
        assert taxi_fallback.get('arrival_date_time') == '20120614T080610'

        assert taxi_fallback.get('durations', {}).get('taxi') == 8
        assert taxi_fallback.get('durations', {}).get('walking') == 39
        assert taxi_fallback.get('durations', {}).get('total') == 349

        assert taxi_fallback.get('distances', {}).get('taxi') == 88
        assert taxi_fallback.get('distances', {}).get('walking') == 19

        sections = taxi_fallback.get('sections')
        assert len(sections) == 4
        assert sections[0].get('mode') == 'walking'
        assert sections[0].get('departure_date_time') == '20120614T080021'
        assert sections[0].get('arrival_date_time') == '20120614T080100'
        assert sections[0].get('duration') == 39
        assert sections[0].get('type') == 'street_network'

        assert sections[1].get('departure_date_time') == '20120614T080100'
        assert sections[1].get('arrival_date_time') == '20120614T080102'
        assert sections[1].get('duration') == 2
        assert sections[1].get('type') == 'public_transport'

        assert sections[2].get('departure_date_time') == '20120614T080102'
        assert sections[2].get('arrival_date_time') == '20120614T080602'
        assert sections[2].get('duration') == 300
        assert sections[2].get('type') == 'waiting'

        assert sections[3].get('mode') == 'taxi'
        assert sections[3].get('departure_date_time') == '20120614T080602'
        assert sections[3].get('arrival_date_time') == '20120614T080610'
        assert sections[3].get('duration') == 8
        assert sections[3].get('type') == 'street_network'

    def test_min_taxi(self):
        query = (
            sub_query
            + "&datetime=20120614T075000"
            + "&first_section_mode[]=taxi"
            + "&first_section_mode[]=walking"
            + "&debug=true"
        )

        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        journeys = get_not_null(response, 'journeys')
        # the taxi direct_path because it's duration in less than min_taxi default
        taxi_direct = journeys[0]
        assert 'deleted_because_too_short_heavy_mode_fallback' in taxi_direct['tags']

        query = (
            sub_query
            + "&datetime=20120614T075000"
            + "&first_section_mode[]=taxi"
            + "&first_section_mode[]=walking"
            + "&_min_taxi=652"
            + "&debug=true"
            + "&taxi_speed=0.15"
        )

        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        taxi_fallback = next((j for j in response['journeys'] if "taxi" in j['tags']), None)
        assert 'deleted_because_too_short_heavy_mode_fallback' in taxi_fallback['tags']

    def test_max_taxi_duration_to_pt(self):
        # we begin with a normal request to get the fallback duration in taxi
        query = sub_query + "&datetime=20120614T075000" + "&first_section_mode[]=taxi" + "&taxi_speed=0.05"

        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        journeys = get_not_null(response, 'journeys')
        assert len(journeys) == 2

        # find the pt journey with taxi as it fallback mode
        taxi_with_pt = next((j for j in journeys if 'non_pt_taxi' not in j['tags']), None)
        assert taxi_with_pt

        # get the fallback duration (it's the addition of wait time and taxi journey's duration)
        taxi_fallback_time = taxi_with_pt['sections'][0]['duration'] + taxi_with_pt['sections'][1]['duration']

        query = (
            sub_query
            + "&datetime=20120614T075000"
            + "&first_section_mode[]=taxi"
            + "&taxi_speed=0.05"
            # Now we set the max_taxi_duration_to_pt
            + "&max_taxi_duration_to_pt={}".format(taxi_fallback_time - 1)
        )

        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        journeys = get_not_null(response, 'journeys')

        assert len(journeys) == 1
        # the pt journey is gone....
        assert 'non_pt_taxi' in journeys[0]['tags']

    def test_additional_time(self):
        # we begin with a normal request to get the fallback duration in taxi
        first_additional_time = 42
        last_additional_time = 20
        query = (
            sub_query
            + "&datetime=20120614T075000"
            + "&first_section_mode[]=taxi"
            + "&last_section_mode[]=taxi"
            + "&taxi_speed=0.05"
            + "&additional_time_after_first_section_taxi={}".format(first_additional_time)
            + "&additional_time_before_last_section_taxi={}".format(last_additional_time)
        )

        response = self.query_region(query)
        self.is_valid_journey_response(response, query)
        pt_with_taxi = next((j for j in response['journeys'] if 'non_pt' not in j['tags']), None)
        assert len(pt_with_taxi['sections']) == 5
        assert pt_with_taxi['sections'][1]['type'] == 'waiting'
        assert pt_with_taxi['sections'][1]['duration'] == first_additional_time

        assert pt_with_taxi['sections'][3]['type'] == 'waiting'
        assert pt_with_taxi['sections'][3]['duration'] == last_additional_time


@dataset({'main_routing_test': {"scenario": "distributed"}, 'min_nb_journeys_test': {"scenario": "distributed"}})
class TestKrakenDistributedWithDatabase(NewDefaultScenarioAbstractTestFixture):
    def setUp(self):
        self.old_db_val = app.config['DISABLE_DATABASE']
        app.config['DISABLE_DATABASE'] = False

    def tearDown(self):
        app.config['DISABLE_DATABASE'] = self.old_db_val

    def _call_and_check_journeys_on_coverage(self, coverage, query_from, query_to, datetime):
        query = 'v1/coverage/{coverage}/journeys?from={query_from}&to={query_to}&datetime={datetime}&debug=true'.format(
            coverage=coverage, query_from=query_from, query_to=query_to, datetime=datetime
        )
        response = self.query(query)
        self.is_valid_journey_response(response, query)
        assert response['debug']['regions_called'][0]['name'] == coverage

    def sn_backends_getter(self):
        kraken = StreetNetworkBackend(id='kraken')
        kraken.klass = "jormungandr.street_network.tests.MockKraken"
        kraken.args = {'timeout': 10}
        kraken.created_at = datetime.datetime.utcnow()

        return [kraken]

    def test_call_with_two_krakens(self):
        """
        Checks that in distributed mode with streetnetwork_backends in database
        There is no error when multiples krakens are up and we call one of them after an other
        """
        manager = StreetNetworkBackendManager(self.sn_backends_getter)
        manager._can_connect_to_database = MagicMock(return_value=True)

        i_manager.instances["main_routing_test"]._streetnetwork_backend_manager = manager
        i_manager.instances["min_nb_journeys_test"]._streetnetwork_backend_manager = manager

        self._call_and_check_journeys_on_coverage("main_routing_test", "stopA", "stopB", "20120614T080000")
        self._call_and_check_journeys_on_coverage(
            "min_nb_journeys_test", "stop_point:sa1:s1", "stop_point:sa3:s1", "20180309T080000"
        )
        self._call_and_check_journeys_on_coverage("main_routing_test", "stopB", "stopC", "20120614T080000")


@dataset({"main_routing_test": {"scenario": "distributed"}})
class TestCarNoParkDistributed(NewDefaultScenarioAbstractTestFixture):
    def test_max_car_no_park_duration_to_pt(self):
        # we begin with a normal request to get the fallback duration in taxi
        query = (
            sub_query
            + "&datetime=20120614T075000"
            + "&first_section_mode[]=car_no_park"
            + "&car_no_park_speed=0.1"
        )

        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        journeys = get_not_null(response, 'journeys')
        assert len(journeys) == 2

        # find the pt journey with car_no_pt as it fallback mode
        car_no_park_direct_path = next((j for j in journeys if 'non_pt' in j['tags']), None)
        assert car_no_park_direct_path
        assert "car_no_park" in car_no_park_direct_path['tags']

        # find the pt journey with car_no_pt as it fallback mode
        car_no_park_with_pt = next((j for j in journeys if 'non_pt' not in j['tags']), None)
        assert car_no_park_with_pt

        # get the fallback duration (it's the addition of wait time and taxi journey's duration)
        car_no_park_with_pt_fallback_time = car_no_park_with_pt['sections'][0]['duration']

        query = (
            sub_query
            + "&datetime=20120614T075000"
            + "&first_section_mode[]=car_no_park"
            + "&car_no_park_speed=0.1"
            # Now we set the max_taxi_duration_to_pt
            + "&max_car_no_park_duration_to_pt={}".format(car_no_park_with_pt_fallback_time - 1)
        )

        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        journeys = get_not_null(response, 'journeys')

        assert len(journeys) == 1
        # the pt journey is gone....
        assert 'non_pt_car_no_park' in journeys[0]['tags']


@dataset({"main_routing_test": {"scenario": "distributed"}})
class TestCarDistributed(NewDefaultScenarioAbstractTestFixture):
    def test_stop_points_nearby_duration_park_n_ride(self):
        # we begin with a normal request to get the fallback duration in taxi
        query = (
            sub_query
            + "&datetime=20120614T080000"
            + "&_min_car=0"
            + "&first_section_mode[]=car"
            + "&car_speed=1"
        )

        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        journeys = get_not_null(response, 'journeys')
        assert len(journeys) == 2

        # find the pt journey with taxi as it fallback mode
        car_with_pt = next((j for j in journeys if 'non_pt' not in j['tags']), None)
        assert car_with_pt

        # get the fallback duration (it's the addition of wait time and taxi journey's duration)
        park_to_stop_point_duration = car_with_pt['sections'][2]['duration']
        query = (
            sub_query
            + "&datetime=20120614T080000"
            + "&first_section_mode[]=car"
            + "&car_speed=1"
            + "&_min_car=0"
            # now we set _stop_points_nearby_duration
            + "&_stop_points_nearby_duration={}".format(int(park_to_stop_point_duration / math.sqrt(2)) - 1)
        )

        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        journeys = get_not_null(response, 'journeys')

        assert len(journeys) == 1
        # the pt journey is gone....
        assert 'non_pt' in journeys[0]['tags']


@config({"scenario": "distributed"})
class TesDistributedJourneyNoCoverageParams(NoCoverageParams, NewDefaultScenarioAbstractTestFixture):
    pass


@dataset({"routing_with_transfer_test": {"scenario": "distributed"}})
class TestRoutingWithTransfer(NewDefaultScenarioAbstractTestFixture):
    def test_complete_transfer_path_bus_coach(self):
        """
        We first query without requesting walking _transfer_path and then with _transfer_path
        With _transfer_path enabled we expect in transfer section :
        - a path
        - detailed geojson instead of a simple line (crow_fly)
        - same duration as with _transfer_path=false
        """
        query = (
            '/v1/coverage/routing_with_transfer_test/journeys?'
            'from={}&to={}&'
            'datetime=20120614T100000&_override_scenario=distributed'
        ).format("stopF", "stopA")

        response = self.query(query)
        assert 'journeys' in response
        journeys = response['journeys']

        assert len(journeys) == 1
        assert len(journeys[0]['sections']) == 6
        assert journeys[0]['sections'][1]['display_informations']['physical_mode'] == 'Bus'
        assert journeys[0]['sections'][2]['type'] == 'transfer'
        assert journeys[0]['sections'][2]['transfer_type'] == 'walking'
        assert journeys[0]['sections'][3]['type'] == 'waiting'
        assert journeys[0]['sections'][4]['display_informations']['physical_mode'] == 'Autocar'
        assert 'path' not in journeys[0]['sections'][2]
        assert 'geojson' in journeys[0]['sections'][2]
        assert 'coordinates' in journeys[0]['sections'][2]['geojson']
        assert len(journeys[0]['sections'][2]['geojson']['coordinates']) == 2

        query = (
            '/v1/coverage/routing_with_transfer_test/journeys?'
            'from={}&to={}&_transfer_path=true&'
            'datetime=20120614T100000&_override_scenario=distributed'
        ).format("stopF", "stopA")

        response = self.query(query)

        assert 'journeys' in response
        journeys = response['journeys']

        assert len(journeys) == 1
        assert len(journeys[0]['sections']) == 6
        assert journeys[0]['sections'][1]['display_informations']['physical_mode'] == 'Bus'
        assert journeys[0]['sections'][2]['type'] == 'transfer'
        assert journeys[0]['sections'][2]['transfer_type'] == 'walking'
        assert journeys[0]['sections'][3]['type'] == 'waiting'
        assert journeys[0]['sections'][4]['display_informations']['physical_mode'] == 'Autocar'
        assert 'path' in journeys[0]['sections'][2]
        assert 'path' in journeys[0]['sections'][2]
        assert len(journeys[0]['sections'][2]['path']) == 3
        assert journeys[0]['sections'][2]['path'][0]['name'] == 'rue de'
        assert journeys[0]['sections'][2]['path'][1]['name'] == 'rue cd'
        assert journeys[0]['sections'][2]['path'][2]['name'] == 'rue bc'
        assert 'geojson' in journeys[0]['sections'][2]
        assert 'coordinates' in journeys[0]['sections'][2]['geojson']
        assert len(journeys[0]['sections'][2]['geojson']['coordinates']) == 4

    def test_complete_transfer_path_metro_coach(self):
        """
        We first query without requesting walking _transfer_path and then with _transfer_path
        In this case : transfer tramway <-> Metro, we expect to receive the same response
        """
        query = (
            '/v1/coverage/routing_with_transfer_test/journeys?'
            'from={}&to={}&forbidden_uris[]=physical_mode:Coach&'
            'datetime=20120614T100000&_override_scenario=distributed'
        ).format("stopF", "stopA")

        response = self.query(query)

        assert 'journeys' in response
        journeys = response['journeys']

        assert len(journeys) == 1
        assert len(journeys[0]['sections']) == 6
        assert journeys[0]['sections'][1]['display_informations']['physical_mode'] == 'Bus'
        assert journeys[0]['sections'][2]['type'] == 'transfer'
        assert journeys[0]['sections'][2]['transfer_type'] == 'walking'
        assert journeys[0]['sections'][3]['type'] == 'waiting'
        assert journeys[0]['sections'][4]['display_informations']['physical_mode'] == 'Metro'
        assert 'path' not in journeys[0]['sections'][2]
        assert 'geojson' in journeys[0]['sections'][2]
        assert 'coordinates' in journeys[0]['sections'][2]['geojson']
        assert len(journeys[0]['sections'][2]['geojson']['coordinates']) == 2

        query = (
            '/v1/coverage/routing_with_transfer_test/journeys?'
            'from={}&to={}&_transfer_path=true&forbidden_uris[]=physical_mode:Coach&'
            'datetime=20120614T100000&_override_scenario=distributed'
        ).format("stopF", "stopA")

        response = self.query(query)

        assert 'journeys' in response
        journeys = response['journeys']

        assert len(journeys) == 1
        assert len(journeys[0]['sections']) == 6
        assert journeys[0]['sections'][1]['display_informations']['physical_mode'] == 'Bus'
        assert journeys[0]['sections'][2]['type'] == 'transfer'
        assert journeys[0]['sections'][2]['transfer_type'] == 'walking'
        assert journeys[0]['sections'][3]['type'] == 'waiting'
        assert journeys[0]['sections'][4]['display_informations']['physical_mode'] == 'Metro'
        assert 'path' not in journeys[0]['sections'][2]
        assert 'geojson' in journeys[0]['sections'][2]
        assert 'coordinates' in journeys[0]['sections'][2]['geojson']
        assert len(journeys[0]['sections'][2]['geojson']['coordinates']) == 2

    def test_complete_transfer_path_bus_rer_with_access_points(self):
        query = (
            '/v1/coverage/routing_with_transfer_test/journeys?'
            'from={}&to={}&datetime=20120614T080000&_override_scenario=distributed&count=1&_transfer_path=true&'
            'language=en-US'
        ).format("stopA", "stopF")

        response = self.query(query)

        assert 'journeys' in response
        journeys = response['journeys']
        assert len(journeys) == 1

        assert len(journeys) == 1
        journey = journeys[0]
        assert len(journey['sections']) == 6

        sections = journey['sections']
        assert sections[1]['display_informations']['physical_mode'] == 'Bus'

        assert sections[2]['type'] == 'transfer'
        assert sections[2]['transfer_type'] == 'walking'
        assert len(sections[2]['vias']) == 1

        via = sections[2]['vias'][0]
        assert via['name'] == 'access_point:E1'
        assert via['is_entrance'] is True
        assert via['is_exit'] is False
        assert via['length'] == 100
        assert via['traversal_time'] == 100
        assert via['access_point']['coord']['lat'] == '0.01616961567518476'
        assert via['access_point']['coord']['lon'] == '0.01796623963909418'

        last_transfer_path = sections[2]['path'][-1]
        assert last_transfer_path['instruction'] == 'Then enter stop_point:stopE (Condom) via access_point:E1.'
        assert last_transfer_path['name'] == "access_point:E1"
        assert last_transfer_path['via_uri'] == "access_point:E1"

        assert sections[4]['display_informations']['physical_mode'] == 'RER'


@dataset({"main_routing_test": {"scenario": "distributed"}})
class TestLinksDistributed(NewDefaultScenarioAbstractTestFixture):
    def test_same_journey_schedules_link(self):
        query = (
            sub_query
            + "&datetime=20120614T080000"
            + "&first_section_mode[]=walking"
            + "&last_section_mode[]=walking"
        )

        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        journeys = get_not_null(response, 'journeys')
        assert len(journeys) == 2
        assert journeys[0]['links'][0]['rel'] == 'same_journey_schedules'
        assert journeys[0]['links'][0]['type'] == 'journeys'
        href_value = journeys[0]['links'][0]['href']
        assert href_value is not None
        assert href_value.count('allowed_id') == 2

        query = (
            sub_query
            + "&datetime=20120614T080000"
            + "&first_section_mode[]=walking"
            + "&last_section_mode[]=walking"
            + "&allowed_id[]=stop_point:stopA"
            + "&allowed_id[]=stop_point:stopB"
        )

        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        journeys = get_not_null(response, 'journeys')
        assert len(journeys) == 2
        assert journeys[0]['links'][0]['rel'] == 'same_journey_schedules'
        assert journeys[0]['links'][0]['type'] == 'journeys'
        href_value = journeys[0]['links'][0]['href']
        assert href_value is not None
        # Before correction: assert href_value.count('allowed_id') == 11
        assert href_value.count('allowed_id') == 2
