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
from .tests_mechanism import NewDefaultScenarioAbstractTestFixture
from .tests_mechanism import config
from .check_utils import *
from .journey_common_tests import *
from unittest import skip

'''
This unit runs all the common tests in journey_common_tests.py along with locals tests added in this
unit for scenario new_default
'''

@config({"scenario": "new_default"})
class TestJourneysNewDefault(JourneyCommon,  DirectPath, JourneyMinBikeMinCar, NewDefaultScenarioAbstractTestFixture):
    """
    Test the new default scenario
    All the tests are defined in "TestJourneys" class, we only change the scenario


    NOTE: for the moment we cannot import all routing tests, so we only get 2, but we need to add some more

    the new default scenario override the way the prev/next link are created
    """
    def test_best_filtering(self):
        """
        This feature is no longer supported
        """
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

    def test_first_bss_last_bss_section_mode(self):
        query = "journeys?from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&datetime=20120614T075500&"\
                "first_section_mode[]={first}&last_section_mode[]={last}"\
                .format(first='bss', last='bss')
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response["journeys"]) == 2
        assert response["journeys"][0]["type"] == "best"
        assert "bss" in response["journeys"][0]["tags"]
        context = response['context']
        assert 'car_direct_path' in context
        assert 'co2_emission' in context['car_direct_path']

    def test_first_walking_last_walking_section_mode(self):
        query = "journeys?from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&datetime=20120614T075500&"\
                "first_section_mode[]={first}&last_section_mode[]={last}"\
                .format(first='walking', last='walking')
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response["journeys"]) == 2
        assert response["journeys"][0]["type"] == "best"
        assert "walking" in response["journeys"][0]["tags"]

    def test_first_bike_last_walking_section_mode(self):
        query = "journeys?from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&datetime=20120614T075500&"\
                "first_section_mode[]={first}&last_section_mode[]={last}&_min_bike=50"\
                .format(first='bike', last='walking')
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response["journeys"]) == 1
        assert response["journeys"][0]["type"] == "best"
        assert "bike" in response["journeys"][0]["tags"]

    def test_first_car_last_walking_section_mode(self):
        query = "journeys?from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&datetime=20120614T075500&"\
                "first_section_mode[]={first}&last_section_mode[]={last}&_min_car=10"\
                .format(first='car', last='walking')
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response["journeys"]) == 1
        assert response["journeys"][0]["type"] == "best"
        assert "car" in response["journeys"][0]["tags"]

    def test_first_bike_last_bss_section_mode(self):
        query = "journeys?from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&datetime=20120614T075500&"\
                "first_section_mode[]={first}&last_section_mode[]={last}&_min_bike=50"\
                .format(first='bike', last='bss')
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response["journeys"]) == 1
        assert response["journeys"][0]["type"] == "best"
        assert "bike" in response["journeys"][0]["tags"]

    def test_first_car_last_bss_section_mode(self):
        query = "journeys?from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&datetime=20120614T075500&"\
                "first_section_mode[]={first}&last_section_mode[]={last}&_min_car=10"\
                .format(first='car', last='bss')
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
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
        check_best(response)
        self.is_valid_journey_response(response, query)
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
        check_best(response)
        self.is_valid_journey_response(response, query)
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
        check_best(response)
        self.is_valid_journey_response(response, query)
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
                "first_section_mode[]=walking&last_section_mode[]=walking"\
                .format(from_coord=s_coord, to_coord=r_coord, datetime="20120614T075500")
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
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
                "_min_car=0"\
                .format(from_coord=s_coord, to_coord=r_coord, datetime="20120614T075500")
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response["journeys"]) == 4
        assert 'car' in response["journeys"][0]["tags"]
        assert 'bss' in response["journeys"][1]["tags"]
        assert not 'walking' in response["journeys"][1]["tags"]
        assert not 'bike' in response["journeys"][1]["tags"]
        assert 'walking' in response["journeys"][2]["tags"]
        assert 'walking' in response["journeys"][3]["tags"]

    def test_first_ridesharing_no_config(self):
        query = "journeys?from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&datetime=20120614T075500&"\
                "first_section_mode[]={first}&last_section_mode[]={last}&debug=true"\
                .format(first='ridesharing', last='walking')
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response["journeys"]) == 1
        assert response["journeys"][0]["type"] == "best"
        assert "ridesharing" in response["journeys"][0]["tags"]
        assert "to_delete" in response["journeys"][0]["tags"]

    def test_first_ridesharing_section_mode_forbidden_no_config(self):
        def exec_and_check(query):
            response, status = self.query_region(query, check=False)
            check_best(response)
            assert status == 400
            assert "message" in response
            assert "ridesharing" in response['message']

        query = "journeys?from=0.0000898312;0.0000898312&datetime=20120614T075500&"\
                "first_section_mode[]={first}&last_section_mode[]={last}"\
                .format(first='ridesharing', last='walking')
        exec_and_check(query)

        query = "isochrones?from=0.0000898312;0.0000898312&datetime=20120614T075500&"\
                "first_section_mode[]={first}&last_section_mode[]={last}&max_duration=2"\
                .format(first='ridesharing', last='walking')
        exec_and_check(query)

        query = "heat_maps?from=0.0000898312;0.0000898312&datetime=20120614T075500&"\
                "first_section_mode[]={first}&last_section_mode[]={last}&max_duration=2"\
                .format(first='ridesharing', last='walking')
        exec_and_check(query)

    def test_free_radius_from(self):
        """
        TO DO : Enable when NAVP-819 is available
        """
        pass

    def test_free_radius_to(self):
        """
        TO DO : Enable when NAVP-819 is available
        """
        pass

@config({"scenario": "new_default",
         'instance_config': {
             "ridesharing": [
             {
                 "class": "jormungandr.scenarios.ridesharing.instant_system.InstantSystem",
                 "args": {
                     "service_url": "http://wtf",
                     "api_key": "key",
                     "network": "Super Covoit",
                     "rating_scale_min": 0,
                     "rating_scale_max": 5
                 }
             }
         ]}})
class TestJourneysRidesharingNewDefault(JourneyCommon,  DirectPath, JourneyMinBikeMinCar,
                                        NewDefaultScenarioAbstractTestFixture):
    def test_best_filtering(self):
        """
        This feature is no longer supported
        """
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

    def test_first_ridesharing_last_walking_section_mode(self):
        query = "journeys?from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&datetime=20120614T075500&"\
                "first_section_mode[]={first}&last_section_mode[]={last}&debug=true"\
                .format(first='ridesharing', last='walking')
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response["journeys"]) == 1
        assert response["journeys"][0]["type"] == "best"
        rs_journey = response["journeys"][0]
        assert "ridesharing" in rs_journey["tags"]
        assert rs_journey["requested_date_time"] == "20120614T075500"
        assert rs_journey["departure_date_time"] == "20120614T075500"
        assert rs_journey["arrival_date_time"] == "20120614T075513"
        assert rs_journey["distances"]["ridesharing"] == 94
        assert rs_journey["duration"] == 13
        assert rs_journey["durations"]["ridesharing"] == rs_journey["duration"]
        assert rs_journey["durations"]["total"] == rs_journey["duration"]
        assert 'to_delete' in rs_journey["tags"] # no response provided for ridesharing: to_delete
        rs_section = rs_journey["sections"][0]
        assert rs_section["departure_date_time"] == rs_journey["departure_date_time"]
        assert rs_section["arrival_date_time"] == rs_journey["arrival_date_time"]
        assert rs_section["duration"] == rs_journey["duration"]
        assert rs_section["mode"] == "ridesharing"
        assert rs_section["type"] == "street_network"
        assert rs_section["id"] # check that id is provided
        assert rs_section["geojson"]["properties"][0]["length"] == rs_journey["distances"]["ridesharing"]
        assert (rs_section["geojson"]["coordinates"][0][0] - float(rs_section["from"]["address"]["coord"]["lon"])) < 0.000001
        assert (rs_section["geojson"]["coordinates"][0][1] - float(rs_section["from"]["address"]["coord"]["lat"])) < 0.000001
        assert (rs_section["geojson"]["coordinates"][1][0] - float(rs_section["to"]["address"]["coord"]["lon"])) < 0.000001
        assert (rs_section["geojson"]["coordinates"][1][1] - float(rs_section["to"]["address"]["coord"]["lat"])) < 0.000001

    def test_first_ridesharing_section_mode_forbidden(self):
        def exec_and_check(query):
            response, status = self.query_region(query, check=False)
            check_best(response)
            assert status == 400
            assert "message" in response
            assert "ridesharing" in response['message']

        query = "journeys?from=0.0000898312;0.0000898312&datetime=20120614T075500&"\
                "first_section_mode[]={first}&last_section_mode[]={last}"\
                .format(first='ridesharing', last='walking')
        exec_and_check(query)

        query = "isochrones?from=0.0000898312;0.0000898312&datetime=20120614T075500&"\
                "first_section_mode[]={first}&last_section_mode[]={last}&max_duration=2"\
                .format(first='ridesharing', last='walking')
        exec_and_check(query)

        query = "heat_maps?from=0.0000898312;0.0000898312&datetime=20120614T075500&"\
                "first_section_mode[]={first}&last_section_mode[]={last}&max_duration=2"\
                .format(first='ridesharing', last='walking')
        exec_and_check(query)

    def test_free_radius_from(self):
        """
        This feature is not supported for this scenario
        """
        pass

    def test_free_radius_to(self):
        """
        This feature is not supported for this scenario
        """
        pass


@config({"scenario": "new_default"})
class TestNewDefaultJourneysWithPtref(JourneysWithPtref, NewDefaultScenarioAbstractTestFixture):
    pass


@config({"scenario": "new_default"})
class TestNewDefaultJourneysNoRegion(JourneysNoRegion, NewDefaultScenarioAbstractTestFixture):
    pass

@config({"scenario": "new_default"})
class TestErrorFieldInJormun(AddErrorFieldInJormun, NewDefaultScenarioAbstractTestFixture):
    pass


@config({"scenario": "new_default"})
class TestNewDefaultOnBasicRouting(OnBasicRouting, NewDefaultScenarioAbstractTestFixture):
    pass

@config({"scenario": "new_default"})
class TestNewDefaultOneDeadRegion(OneDeadRegion, NewDefaultScenarioAbstractTestFixture):
    pass


@config({"scenario": "new_default"})
class TestNewDefaultWithoutPt(WithoutPt, NewDefaultScenarioAbstractTestFixture):
    pass
