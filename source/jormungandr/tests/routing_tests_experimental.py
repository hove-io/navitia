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
from .routing_tests import OnBasicRouting

'''
This unit runs all the common tests in journey_common_tests.py along with locals tests added in this
unit for scenario experimental
'''

@config({'scenario': 'distributed'})
class TestJourneysDistributed(JourneyCommon, DirectPath, JourneyMinBikeMinCar, NewDefaultScenarioAbstractTestFixture):
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
        #self.is_valid_journey_response(response, query)# linestring with 1 value (0,0)
        jrnys = response['journeys']
        assert jrnys
        assert jrnys[0]['sections'][0]['mode'] == 'walking'
        assert jrnys[0]['sections'][-1]['mode'] == 'car'
        context = response['context']
        assert 'car_direct_path' in context
        assert 'co2_emission' in context['car_direct_path']

    def test_best_filtering(self):
        """
        This feature is no longer supported"""
        pass

    def test_datetime_represents_arrival(self):
        super(TestJourneysDistributed, self).test_datetime_represents_arrival()

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
        }
        request = {
            "walking_speed": instance.walking_speed,
            "bike_speed": instance.bike_speed,
            "car_speed": instance.car_speed,
            "bss_speed": instance.bss_speed,
        }
        resp = instance.get_street_network_routing_matrix([origin], [destination],
                                                          mode, max_duration, request, **kwargs)
        assert len(resp.rows[0].routing_response) == 1
        assert resp.rows[0].routing_response[0].duration == 107
        assert resp.rows[0].routing_response[0].routing_status == response_pb2.reached

        max_duration = 106
        resp = instance.get_street_network_routing_matrix([origin], [destination],
                                                          mode, max_duration, request, **kwargs)
        assert len(resp.rows[0].routing_response) == 1
        assert resp.rows[0].routing_response[0].duration == 0
        assert resp.rows[0].routing_response[0].routing_status == response_pb2.unreached

    def test_intersection_objects(self):
        # The coordinates of arrival and the stop point are separated by 20m
        r = self.query('/v1/coverage/main_routing_test/journeys?from=stopA&to=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&datetime=20120614080000&')
        assert len(r['journeys'][0]['sections']) == 3

        # destination of crow_fly section and origin of next pt section should be the same object.
        assert(r['journeys'][0]['sections'][0]['type'] == 'crow_fly')
        assert(r['journeys'][0]['sections'][1]['type'] == 'public_transport')
        assert(r['journeys'][0]['sections'][0]['to'] == r['journeys'][0]['sections'][1]['from'])

        # destination of pt section and origin of next street_network section should be the same object.
        assert(r['journeys'][0]['sections'][-1]['type'] == 'street_network')
        assert(r['journeys'][0]['sections'][1]['to'] == r['journeys'][0]['sections'][-1]['from'])

        r = self.query('/v1/coverage/main_routing_test/journeys?from=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&to=stopA&datetime=20120614080000')
        assert len(r['journeys'][0]['sections']) == 3

        # destination of crow_fly section and origin of next pt section should be the same object.
        assert(r['journeys'][0]['sections'][0]['type'] == 'street_network')
        assert(r['journeys'][0]['sections'][1]['type'] == 'public_transport')
        assert(r['journeys'][0]['sections'][0]['to'] == r['journeys'][0]['sections'][1]['from'])

        # destination of pt section and origin of next street_network section should be the same object.
        assert(r['journeys'][0]['sections'][-1]['type'] == 'crow_fly')
        assert(r['journeys'][0]['sections'][1]['to'] == r['journeys'][0]['sections'][-1]['from'])


@config({"scenario": "distributed"})
class TestDistributedJourneysWithPtref(JourneysWithPtref, NewDefaultScenarioAbstractTestFixture):
    pass


@config({"scenario": "distributed"})
class TestDistributedOnBasicRouting(OnBasicRouting, NewDefaultScenarioAbstractTestFixture):
    @skip("temporarily disabled")
    def test_isochrone(self):
        super(TestExperimentalOnBasicRouting, self).test_isochrone()
