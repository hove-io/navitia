# Copyright (c) 2001-2015, Canal TP and/or its affiliates. All rights reserved.
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

from __future__ import absolute_import, print_function, unicode_literals, division
from .tests_mechanism import config, NewDefaultScenarioAbstractTestFixture
from .journey_common_tests import *
from unittest import skip

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
        """Test to verify optimization of direct path calls
        """
        # Initialize counter value in the object MockKraken
        sn_service = i_manager.instances['main_routing_test'].street_network_services[0]
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

        # Without optimization (context.partial_response_is_empty = True in distributed._compute_all()
        # journey count = 18 / direct_path_call_count = 26 / routing_matrix_call_count = 20
        # get_directpath_count_by_mode(response, 'walking') == 5
        # get_directpath_count_by_mode(response, 'bike') == 5
        assert len(response["journeys"]) == 13
        assert sn_service.direct_path_call_count == 6
        assert sn_service.routing_matrix_call_count == 4
        assert get_directpath_count_by_mode(response, 'walking') == 1
        assert get_directpath_count_by_mode(response, 'bike') == 1

        # This will call jormun so we check our counter before
        self.is_valid_journey_response(response, query)

    def test_first_and_last_section_multi_modes_no_debug(self):
        """Test to verify optimization of direct path calls
        """
        # Initialize counter value in the object MockKraken
        sn_service = i_manager.instances['main_routing_test'].street_network_services[0]
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

        # Without optimization (context.partial_response_is_empty = True in distributed._compute_all()
        # journey count = 18 / direct_path_call_count = 26 / routing_matrix_call_count = 20
        # get_directpath_count_by_mode(response, 'walking') == 5
        # get_directpath_count_by_mode(response, 'bike') == 5
        assert len(response["journeys"]) == 4
        assert sn_service.direct_path_call_count == 6
        assert sn_service.routing_matrix_call_count == 4
        assert get_directpath_count_by_mode(response, 'walking') == 1
        assert get_directpath_count_by_mode(response, 'bike') == 0

        # This will call jormun so we check our counter before
        self.is_valid_journey_response(response, query)


@config({'scenario': 'distributed'})
class TestJourneysDistributed(
    JourneyCommon, DirectPath, JourneyMinBikeMinCar, NewDefaultScenarioAbstractTestFixture
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
        query = journey_basic_query + "&first_section_mode[]=walking&last_section_mode[]=car&debug=true"
        response = self.query_region(query)
        check_best(response)
        jrnys = response['journeys']
        assert jrnys
        assert jrnys[0]['sections'][0]['mode'] == 'walking'
        assert jrnys[0]['sections'][-1]['mode'] == 'car'
        context = response['context']
        assert 'car_direct_path' in context
        assert 'co2_emission' in context['car_direct_path']

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
        origin = instance.georef.place("stopB")
        assert origin

        destination = instance.georef.place("stopA")
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
        }
        resp = instance.get_street_network_routing_matrix(
            [origin], [destination], mode, max_duration, request, **kwargs
        )
        assert len(resp.rows[0].routing_response) == 1
        assert resp.rows[0].routing_response[0].duration == 107
        assert resp.rows[0].routing_response[0].routing_status == response_pb2.reached

        max_duration = 106
        resp = instance.get_street_network_routing_matrix(
            [origin], [destination], mode, max_duration, request, **kwargs
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


@config({"scenario": "distributed"})
class TestDistributedJourneysWithPtref(JourneysWithPtref, NewDefaultScenarioAbstractTestFixture):
    pass


@config({"scenario": "distributed"})
class TestDistributedOnBasicRouting(OnBasicRouting, NewDefaultScenarioAbstractTestFixture):
    @skip("temporarily disabled")
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


MOCKED_INSTANCE_CONF_TAXI = {
    "scenario": "distributed",
    "instance_config": {
        "street_network": [
            {
                "modes": ["taxi"],
                "class": "jormungandr.street_network.taxi.Taxi",
                "args": {
                    "street_network": {
                        "class": "jormungandr.street_network.kraken.Kraken",
                        "args": {"timeout": 10},
                    }
                },
            }
        ]
    },
}


@dataset({"main_routing_test": MOCKED_INSTANCE_CONF_TAXI})
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
        assert taxi_direct.get('arrival_date_time') == '20120614T075007'
        assert taxi_direct.get('duration') == 7
        assert taxi_direct.get('durations').get("car") == 7
        assert taxi_direct.get('durations').get("total") == 7
        assert taxi_direct.get('distances').get("car") == 87

        sections = taxi_direct.get('sections')
        assert len(sections) == 1
        assert sections[0].get('mode') == 'taxi'
        assert sections[0].get('departure_date_time') == '20120614T075000'
        assert sections[0].get('arrival_date_time') == '20120614T075007'
        assert sections[0].get('duration') == 7
        assert sections[0].get('type') == 'street_network'

        query += "&taxi_speed=0.15"

        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        journeys = get_not_null(response, 'journeys')
        assert len(journeys) == 2

        taxi_direct = journeys[0]

        assert taxi_direct.get('departure_date_time') == '20120614T075000'
        assert taxi_direct.get('arrival_date_time') == '20120614T080051'
        assert taxi_direct.get('duration') == 651
        assert taxi_direct.get('durations').get("car") == 651
        assert taxi_direct.get('durations').get("total") == 651
        assert taxi_direct.get('distances').get("car") == 97
        sections = taxi_direct.get('sections')
        assert len(sections) == 1
        assert sections[0].get('mode') == 'taxi'
        assert sections[0].get('departure_date_time') == '20120614T075000'
        assert sections[0].get('arrival_date_time') == '20120614T080051'
        assert sections[0].get('duration') == 651
        assert sections[0].get('type') == 'street_network'

        taxi_fallback = journeys[1]

        assert taxi_fallback.get('departure_date_time') == '20120614T075355'
        assert taxi_fallback.get('arrival_date_time') == '20120614T080222'

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
