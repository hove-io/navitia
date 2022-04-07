# Copyright (c) 2001-2015, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.kisio.com).
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
from .tests_mechanism import NewDefaultScenarioAbstractTestFixture
from .tests_mechanism import config
from .journey_common_tests import *

"""
This unit runs all the common tests in journey_common_tests.py along with locals tests added in this
unit for scenario new_default
"""


@config({"scenario": "new_default"})
class TestJourneysNewDefault(
    JourneyCommon,
    DirectPath,
    JourneyMinBikeMinCar,
    NewDefaultScenarioAbstractTestFixture,
    JourneysDirectPathMode,
):
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
        query = (
            "journeys?from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&datetime=20120614T075500&"
            "first_section_mode[]={first}&last_section_mode[]={last}".format(first='bss', last='bss')
        )
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
        query = (
            "journeys?from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&datetime=20120614T075500&"
            "first_section_mode[]={first}&last_section_mode[]={last}".format(first='walking', last='walking')
        )
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response["journeys"]) == 2
        assert response["journeys"][0]["type"] == "best"
        assert "walking" in response["journeys"][0]["tags"]

    def test_first_bike_last_walking_section_mode(self):
        query = (
            "journeys?from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&datetime=20120614T075500&"
            "first_section_mode[]={first}&last_section_mode[]={last}&_min_bike=50".format(
                first='bike', last='walking'
            )
        )
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response["journeys"]) == 1
        assert response["journeys"][0]["type"] == "best"
        assert "bike" in response["journeys"][0]["tags"]

    def test_first_car_last_walking_section_mode(self):
        query = (
            "journeys?from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&datetime=20120614T075500&"
            "first_section_mode[]={first}&last_section_mode[]={last}&_min_car=10".format(
                first='car', last='walking'
            )
        )
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response["journeys"]) == 1
        assert response["journeys"][0]["type"] == "best"
        assert "car" in response["journeys"][0]["tags"]

    def test_first_bike_last_bss_section_mode(self):
        query = (
            "journeys?from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&datetime=20120614T075500&"
            "first_section_mode[]={first}&last_section_mode[]={last}&_min_bike=50".format(
                first='bike', last='bss'
            )
        )
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response["journeys"]) == 1
        assert response["journeys"][0]["type"] == "best"
        assert "bike" in response["journeys"][0]["tags"]

    def test_first_car_last_bss_section_mode(self):
        query = (
            "journeys?from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&datetime=20120614T075500&"
            "first_section_mode[]={first}&last_section_mode[]={last}&_min_car=10".format(first='car', last='bss')
        )
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
        query = (
            "journeys?from={from_coord}&to={to_coord}&datetime={datetime}&"
            "first_section_mode[]=car&first_section_mode[]=walking&last_section_mode[]=walking".format(
                from_coord=s_coord, to_coord=r_coord, datetime="20120614T075500"
            )
        )
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
        query = (
            "journeys?from={from_coord}&to={to_coord}&datetime={datetime}&"
            "first_section_mode[]=car&first_section_mode[]=walking&"
            "last_section_mode[]=walking&_min_car=0".format(
                from_coord=s_coord, to_coord=r_coord, datetime="20120614T075500"
            )
        )
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
        query = (
            "journeys?from={from_coord}&to={to_coord}&datetime={datetime}&"
            "first_section_mode[]=car&first_section_mode[]=walking&"
            "last_section_mode[]=walking&_min_car=0".format(
                from_coord=s_coord, to_coord=r_coord, datetime="20120614T075500"
            )
        )
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert 'context' in response
        assert 'car_direct_path' in response['context']
        assert 'co2_emission' in response['context']['car_direct_path']
        assert len(response["journeys"]) == 3

        assert (
            response['context']['car_direct_path']['co2_emission']['value']
            == response['journeys'][0]['co2_emission']['value']
        )  # car co2 emission

        assert (
            response['context']['car_direct_path']['co2_emission']['unit']
            == response['journeys'][0]['co2_emission']['unit']
        )

    def test_context_car_co2_emission_without_car(self):
        """
        Test if car_co2_emission in context is the value in car journey
        """
        query = (
            "journeys?from={from_coord}&to={to_coord}&datetime={datetime}&"
            "first_section_mode[]=walking&last_section_mode[]=walking".format(
                from_coord=s_coord, to_coord=r_coord, datetime="20120614T075500"
            )
        )
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
        query = (
            "journeys?from={from_coord}&to={to_coord}&datetime={datetime}&"
            "first_section_mode[]=car&first_section_mode[]=walking&"
            "last_section_mode[]=walking&"
            "_min_car=0".format(from_coord=s_coord, to_coord=r_coord, datetime="20120614T075500")
        )
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response["journeys"]) == 3
        assert 'car' in response["journeys"][0]["tags"]
        assert not 'walking' in response["journeys"][0]["tags"]
        assert not 'bike' in response["journeys"][0]["tags"]
        assert 'walking' in response["journeys"][1]["tags"]
        assert not 'bike' in response["journeys"][1]["tags"]
        assert 'walking' in response["journeys"][2]["tags"]
        assert not 'bike' in response["journeys"][2]["tags"]

        query = (
            "journeys?from={from_coord}&to={to_coord}&datetime={datetime}&"
            "first_section_mode[]=bike&first_section_mode[]=walking&"
            "last_section_mode[]=walking&"
            "_min_bike=0".format(from_coord=s_coord, to_coord=r_coord, datetime="20120614T075500")
        )
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response["journeys"]) == 3
        assert 'bike' in response["journeys"][0]["tags"]
        assert not 'walking' in response["journeys"][0]["tags"]
        assert 'walking' in response["journeys"][1]["tags"]
        assert 'walking' in response["journeys"][2]["tags"]

        query = (
            "journeys?from={from_coord}&to={to_coord}&datetime={datetime}&"
            "first_section_mode[]=bss&first_section_mode[]=walking&"
            "last_section_mode[]=walking&last_section_mode[]=bss".format(
                from_coord=s_coord, to_coord=r_coord, datetime="20120614T075500"
            )
        )
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response["journeys"]) == 3
        assert 'bss' in response["journeys"][0]["tags"]
        assert not 'walking' in response["journeys"][0]["tags"]
        assert 'walking' in response["journeys"][1]["tags"]
        assert 'walking' in response["journeys"][2]["tags"]

    def test_first_ridesharing_no_config(self):
        query = (
            "journeys?from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&datetime=20120614T075500&"
            "first_section_mode[]={first}&last_section_mode[]={last}&debug=true".format(
                first='ridesharing', last='walking'
            )
        )
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

        query = (
            "journeys?from=0.0000898312;0.0000898312&datetime=20120614T075500&"
            "first_section_mode[]={first}&last_section_mode[]={last}".format(first='ridesharing', last='walking')
        )
        exec_and_check(query)

        query = (
            "isochrones?from=0.0000898312;0.0000898312&datetime=20120614T075500&"
            "first_section_mode[]={first}&last_section_mode[]={last}&max_duration=2".format(
                first='ridesharing', last='walking'
            )
        )
        exec_and_check(query)

        query = (
            "heat_maps?from=0.0000898312;0.0000898312&datetime=20120614T075500&"
            "first_section_mode[]={first}&last_section_mode[]={last}&max_duration=2".format(
                first='ridesharing', last='walking'
            )
        )
        exec_and_check(query)

    def test_no_origin_or_destination_point_in_request(self):
        query = "journeys?datetime=20120614T075500"
        response = self.query_region(query, check=False)
        assert response[1] == 400
        assert "you should at least provide either a 'from' or a 'to' argument" in response[0]["message"]

    def test_same_origin_and_destination_point_in_request(self):
        query = "journeys?datetime=20120614T075500&from=0.0000898312;0.0000898312&to=0.0000898312;0.0000898312"
        response = self.query_region(query, check=False)
        assert response[1] == 400
        assert "your origin and destination points are the same" in response[0]["message"]

    def test_walking_transfer_penalty_parameter(self):
        """
        This feature is no longer supported
        """
        pass


@config(
    {
        "scenario": "new_default",
        'instance_config': {
            "ridesharing": [
                {
                    "class": "jormungandr.scenarios.ridesharing.instant_system.InstantSystem",
                    "args": {
                        "service_url": "http://wtf",
                        "api_key": "key",
                        "network": "Super Covoit",
                        "rating_scale_min": 0,
                        "rating_scale_max": 5,
                    },
                }
            ]
        },
    }
)
class TestJourneysRidesharingNewDefault(
    JourneysRidesharing, JourneyCommon, DirectPath, JourneyMinBikeMinCar, NewDefaultScenarioAbstractTestFixture
):
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

    def test_walking_transfer_penalty_parameter(self):
        """
        This feature is no longer supported
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


@config({"scenario": "new_default"})
class TestNewDefaultMinNbJourneys(JourneysMinNbJourneys, NewDefaultScenarioAbstractTestFixture):
    pass


@config({"scenario": "new_default"})
class TestNewDefaultWithNightBusFilter(JourneysWithNightBusFilter, NewDefaultScenarioAbstractTestFixture):
    pass


@config({"scenario": "new_default"})
class TestNewDefaultTimeFrameDuration(JourneysTimeFrameDuration, NewDefaultScenarioAbstractTestFixture):
    pass


@config({"scenario": "new_default"})
class TestNewDefaultJourneyTickets(JourneysTickets, NewDefaultScenarioAbstractTestFixture):
    pass


@config({"scenario": "new_default"})
class TestNewDefaultJourneyTicketsWithDebug(JourneysTicketsWithDebug, NewDefaultScenarioAbstractTestFixture):
    pass


@config({"scenario": "new_default"})
class TestNewDefaultJourneyNoCoverageParams(NoCoverageParams, NewDefaultScenarioAbstractTestFixture):
    pass


@dataset({"main_routing_test": {"scenario": "new_default"}})
class TestTaxiNewDefault(NewDefaultScenarioAbstractTestFixture):
    def test_taxi_with_new_default(self):
        query = sub_query + "&datetime=20120614T075000" + "&first_section_mode[]=taxi" + "&debug=true"
        response = self.query_region(query, check=False)
        assert response[1] == 400
        assert "taxi is not available with new_default scenario" in response[0]['message']
