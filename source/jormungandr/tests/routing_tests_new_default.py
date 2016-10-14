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
from datetime import timedelta
from .tests_mechanism import AbstractTestFixture
from .tests_mechanism import config
from .check_utils import *
from nose.tools import eq_
from jormungandr.scenarios.qualifier import min_from_criteria
from .journey_common_tests import *
from unittest import skip

'''
This unit runs all the common tests in journey_common_tests.py along with locals tests added in this
unit for scenario new_default
'''

@config({"scenario": "new_default"})
class TestJourneysNewDefault(JourneyCommon,  DirectPath, AbstractTestFixture):
    """
    Test the new default scenario
    All the tests are defined in "TestJourneys" class, we only change the scenario


    NOTE: for the moment we cannot import all routing tests, so we only get 2, but we need to add some more

    the new default scenario override the way the prev/next link are created
    """
    @staticmethod
    def check_next_datetime_link(dt, response, clockwise):
        if not response.get('journeys'):
            return
        """default next behaviour is 1s after the best or the soonest"""
        j_to_compare = min_from_criteria(generate_pt_journeys(response),
                                         new_default_pagination_journey_comparator(clockwise=clockwise))

        j_departure = get_valid_datetime(j_to_compare['departure_date_time'])
        eq_(j_departure + timedelta(seconds=1), dt)

    @staticmethod
    def check_previous_datetime_link(dt, response, clockwise):
        # All journeys in file with clockwise=true
        if not response.get('journeys'):
            return
        """default previous behaviour is 1s before the best or the latest """
        j_to_compare = min_from_criteria(generate_pt_journeys(response),
                                         new_default_pagination_journey_comparator(clockwise=clockwise))

        j_departure = get_valid_datetime(j_to_compare['arrival_date_time'])
        eq_(j_departure - timedelta(seconds=1), dt)

    @skip("temporarily disabled")
    def test_best_filtering(self):
        super(JourneyCommon, self).test_best_filtering()

    @skip("temporarily disabled")
    def test_journeys_wheelchair_profile(self):
        super(JourneyCommon, self).test_journeys_wheelchair_profile()

    @skip("temporarily disabled")
    def test_not_existent_filtering(self):
        super(JourneyCommon, self).test_not_existent_filtering()

    @skip("temporarily disabled")
    def test_other_filtering(self):
        super(JourneyCommon, self).test_other_filtering()

    @skip("temporarily disabled")
    def test_speed_factor_direct_path(self):
        super(JourneyCommon, self).test_speed_factor_direct_path()

    def test_first_bss_last_bss_section_mode(self):
        query = "journeys?from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&datetime=20120614T075500&"\
                "first_section_mode[]={first}&last_section_mode[]={last}"
        response = self.query_region(query.format(first='bss', last='bss'))
        check_journeys(response)
        assert len(response["journeys"]) == 2
        assert response["journeys"][0]["type"] == "best"
        assert "bss" in response["journeys"][0]["tags"]

    def test_first_walking_last_walking_section_mode(self):
        query = "journeys?from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&datetime=20120614T075500&"\
                "first_section_mode[]={first}&last_section_mode[]={last}"
        response = self.query_region(query.format(first='walking', last='walking'))
        check_journeys(response)
        assert len(response["journeys"]) == 2
        assert response["journeys"][0]["type"] == "best"
        assert "walking" in response["journeys"][0]["tags"]

    def test_first_bike_last_walking_section_mode(self):
        query = "journeys?from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&datetime=20120614T075500&"\
                "first_section_mode[]={first}&last_section_mode[]={last}&_min_bike=50"
        response = self.query_region(query.format(first='bike', last='walking'))
        check_journeys(response)
        assert len(response["journeys"]) == 1
        assert response["journeys"][0]["type"] == "best"
        assert "bike" in response["journeys"][0]["tags"]

    def test_first_car_last_walking_section_mode(self):
        query = "journeys?from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&datetime=20120614T075500&"\
                "first_section_mode[]={first}&last_section_mode[]={last}&_min_car=10"
        response = self.query_region(query.format(first='car', last='walking'))
        check_journeys(response)
        assert len(response["journeys"]) == 1
        assert response["journeys"][0]["type"] == "best"
        assert "car" in response["journeys"][0]["tags"]

    def test_first_bike_last_bss_section_mode(self):
        query = "journeys?from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&datetime=20120614T075500&"\
                "first_section_mode[]={first}&last_section_mode[]={last}&_min_bike=50"
        response = self.query_region(query.format(first='bike', last='bss'))
        check_journeys(response)
        assert len(response["journeys"]) == 1
        assert response["journeys"][0]["type"] == "best"
        assert "bike" in response["journeys"][0]["tags"]

    def test_first_car_last_bss_section_mode(self):
        query = "journeys?from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&datetime=20120614T075500&"\
                "first_section_mode[]={first}&last_section_mode[]={last}&_min_car=10"
        response = self.query_region(query.format(first='car', last='bss'))
        check_journeys(response)
        assert len(response["journeys"]) == 1
        assert response["journeys"][0]["type"] == "best"
        assert "car" in response["journeys"][0]["tags"]

    def test_ecologic_tag(self):
        """test that the tag ecologic is present when the journey doesn't
        produce too much CO2 compared to the car journey.
        """
        query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}&"\
                "first_section_mode[]=car&first_section_mode[]=walking&last_section_mode[]=walking"\
                .format(from_coord=s_coord, to_coord=r_coord, datetime="20120614T075500")
        response = self.query_region(query)
        check_journeys(response)
        assert len(response["journeys"]) == 2
        for j in response["journeys"]:
            assert "ecologic" in j["tags"]

    def test_ecologic_tag_with_car(self):
        """test that the tag ecologic is present when the journey doesn't
        produce too much CO2 compared to the car journey.
        """
        query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}&"\
                "first_section_mode[]=car&first_section_mode[]=walking&"\
                "last_section_mode[]=walking&_min_car=0"\
                .format(from_coord=s_coord, to_coord=r_coord, datetime="20120614T075500")
        response = self.query_region(query)
        check_journeys(response)
        assert len(response["journeys"]) == 3
        for j in response["journeys"]:
            if j["type"] == "best":
                assert "ecologic" not in j["tags"]
            else:
                assert "ecologic" in j["tags"]

    def test_context_car_co2_emission_with_car(self):
        """
        Test if car_co2_emission in context is the value in car journey
        """
        query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}&"\
                "first_section_mode[]=car&first_section_mode[]=walking&"\
                "last_section_mode[]=walking&_min_car=0"\
                .format(from_coord=s_coord, to_coord=r_coord, datetime="20120614T075500")
        response = self.query_region(query)
        check_journeys(response)
        assert 'context' in response
        assert 'car_direct_path' in response['context']
        assert 'co2_emission' in response['context']['car_direct_path']
        assert len(response["journeys"]) == 3

        assert response['context']['car_direct_path']['co2_emission']['value'] == \
               response['journeys'][0]['co2_emission']['value'] # car co2 emission

        assert response['context']['car_direct_path']['co2_emission']['unit'] == \
               response['journeys'][0]['co2_emission']['unit']

    def test_context_car_co2_emission_without_car(self):
        """
        Test if car_co2_emission in context is the value in car journey
        """
        query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}&"\
                "&first_section_mode[]=walking&last_section_mode[]=walking"\
                .format(from_coord=s_coord, to_coord=r_coord, datetime="20120614T075500")
        response = self.query_region(query)
        check_journeys(response)
        assert len(response["journeys"]) == 2

        assert 'context' in response
        assert 'car_direct_path' in response['context']
        assert 'co2_emission' in response['context']['car_direct_path']

        assert response['context']['car_direct_path']['co2_emission']['value'] == 52.5908892104
        assert response['context']['car_direct_path']['co2_emission']['unit'] == 'gEC'
        for j in response["journeys"]:
            assert "ecologic" in j["tags"]

    def test_mode_tag(self):
        query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}&"\
                "first_section_mode[]=car&first_section_mode[]=walking&"\
                "first_section_mode[]=bike&first_section_mode[]=bss&"\
                "last_section_mode[]=walking&last_section_mode[]=bss&"\
                "&_min_car=0"\
                .format(from_coord=s_coord, to_coord=r_coord, datetime="20120614T075500")
        response = self.query_region(query)
        check_journeys(response)
        assert len(response["journeys"]) == 4
        assert 'car' in response["journeys"][0]["tags"]
        assert 'bss' in response["journeys"][1]["tags"]
        assert not 'walking' in response["journeys"][1]["tags"]
        assert not 'bike' in response["journeys"][1]["tags"]
        assert 'walking' in response["journeys"][2]["tags"]
        assert 'walking' in response["journeys"][3]["tags"]

    @skip("temporarily disabled")
    def test_traveler_type(self):
        super(JourneyCommon, self).test_traveler_type()


@config({"scenario": "new_default"})
class TestNewDefaultJourneysWithPtref(JourneysWithPtref, AbstractTestFixture):
    pass


@config({"scenario": "new_default"})
class TestNewDefaultJourneysNoRegion(JourneysNoRegion, AbstractTestFixture):
    pass



@dataset({"basic_routing_test": {"scenario": "new_default"}})
class TestNewDefaultOnBasicRouting(OnBasicRouting, AbstractTestFixture):

    @skip("temporarily disabled")
    def test_journeys_with_show_codes(self):
        super(OnBasicRouting, self).test_journeys_with_show_codes()

    @skip("temporarily disabled")
    def test_journeys_without_show_codes(self):
        super(OnBasicRouting, self).test_journeys_without_show_codes()

    @skip("temporarily disabled")
    def test_novalidjourney_on_first_call(self):
        super(OnBasicRouting, self).test_novalidjourney_on_first_call()

    @skip("temporarily disabled")
    def test_novalidjourney_on_first_call_debug(self):
        super(OnBasicRouting, self).test_novalidjourney_on_first_call_debug()

@config({"scenario": "new_default"})
class TestNewDefaultOneDeadRegion(OneDeadRegion, AbstractTestFixture):
    pass


@config({"scenario": "new_default"})
class TestNewDefaultWithoutPt(WithoutPt, AbstractTestFixture):
    pass
