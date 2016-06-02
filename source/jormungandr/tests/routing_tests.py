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

from __future__ import absolute_import, print_function, unicode_literals, division
from .tests_mechanism import AbstractTestFixture, dataset
from .check_utils import *
from nose.tools import eq_


@dataset({"main_routing_test": {}})
class TestJourneys(AbstractTestFixture):
    """
    Test the structure of the journeys response
    """

    def test_journeys(self):
        #NOTE: we query /v1/coverage/main_routing_test/journeys and not directly /v1/journeys
        #not to use the jormungandr database
        response = self.query_region(journey_basic_query, display=True)

        self.is_valid_journey_response(response, journey_basic_query)

        feed_publishers = get_not_null(response, "feed_publishers")
        assert (len(feed_publishers) == 1)
        feed_publisher = feed_publishers[0]
        assert (feed_publisher["id"] == "builder")
        assert (feed_publisher["name"] == 'routing api data')
        assert (feed_publisher["license"] == "ODBL")
        assert (feed_publisher["url"] == "www.canaltp.fr")

    def test_error_on_journeys(self):
        """ if we got an error with kraken, an error should be returned"""

        query_out_of_production_bound = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}"\
            .format(from_coord="0.0000898312;0.0000898312",  # coordinate of S in the dataset
            to_coord="0.00188646;0.00071865",  # coordinate of R in the dataset
            datetime="20110614T080000")  # 2011 should not be in the production period

        response, status = self.query_no_assert("v1/coverage/main_routing_test/" + query_out_of_production_bound)

        assert status != 200, "the response should not be valid"

        assert response['error']['id'] == "date_out_of_bounds"
        assert response['error']['message'] == "date is not in data production period"

        #and no journey is to be provided
        assert 'journeys' not in response or len(response['journeys']) == 0

    def test_missing_params(self):
        """we should at least provide a from or a to on the /journeys api"""
        query = "journeys?datetime=20120614T080000"

        response, status = self.query_no_assert("v1/coverage/main_routing_test/" + query)

        assert status == 400
        get_not_null(response, 'message')

    def test_best_filtering(self):
        """Filter to get the best journey, we should have only one journey, the best one"""
        query = "{query}&type=best".format(query=journey_basic_query)
        response = self.query_region(query)

        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1

        assert response['journeys'][0]["type"] == "best"

    def test_other_filtering(self):
        """the basic query return a non pt walk journey and a best journey. we test the filtering of the non pt"""

        response = self.query_region("{query}&type=non_pt_walk".
                                     format(query=journey_basic_query))

        assert len(response['journeys']) == 1
        assert response['journeys'][0]["type"] == "non_pt_walk"

    def test_speed_factor_direct_path(self):
        """We test the coherence of the non pt walk solution with a speed factor"""

        response = self.query_region("{query}&type=non_pt_walk&walking_speed=1.5".
                                     format(query=journey_basic_query))

        assert len(response['journeys']) == 1
        assert response['journeys'][0]["type"] == "non_pt_walk"
        assert len(response['journeys'][0]['sections']) == 1
        assert response['journeys'][0]['duration'] == response['journeys'][0]['sections'][0]['duration']
        assert response['journeys'][0]['duration'] == 205
        assert response['journeys'][0]['departure_date_time'] == response['journeys'][0]['sections'][0]['departure_date_time']
        assert response['journeys'][0]['departure_date_time'] == '20120614T080000'
        assert response['journeys'][0]['arrival_date_time'] == response['journeys'][0]['sections'][0]['arrival_date_time']
        assert response['journeys'][0]['arrival_date_time'] == '20120614T080325'

    def test_not_existent_filtering(self):
        """if we filter with a real type but not present, we don't get any journey, but we got a nice error"""

        response = self.query_region("{query}&type=car".
                                     format(query=journey_basic_query))

        assert not 'journeys' in response or len(response['journeys']) == 0
        assert 'error' in response
        assert response['error']['id'] == 'no_solution'
        assert response['error']['message'] == 'No journey found, all were filtered'

    def test_dumb_filtering(self):
        """if we filter with non existent type, we get an error"""

        response, status = self.query_region("{query}&type=sponge_bob"
                                             .format(query=journey_basic_query), check=False)

        assert status == 400, "the response should not be valid"

        assert response['message'].startswith("The type argument must be in list")

    def test_journeys_no_bss_and_walking(self):
        query = journey_basic_query + "&first_section_mode=walking&first_section_mode=bss"
        response = self.query_region(query)

        self.is_valid_journey_response(response, query)
        #Note: we need to mock the kraken instances to check that only one call has been made and not 2
        #(only one for bss because walking should not have been added since it duplicate bss)

        # we explicitly check that we find both mode in the responses link
        # (is checked in is_valid_journey, but never hurts to check twice)
        links = get_links_dict(response)
        for l in ["prev", "next", "first", "last"]:
            assert l in links
            url = links[l]['href']
            url_dict = query_from_str(url)
            assert url_dict['first_section_mode'] == ['walking', 'bss']

    def test_min_nb_journeys(self):
        """Checks if min_nb_journeys works.

        _night_bus_filter_base_factor is used because we need to find
        2 journeys, and we can only take the bus the day after.
        datetime is modified because, as the bus begins at 8, we need
        to check that we don't do the next on the direct path starting
        datetime.
        """
        query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}&"\
                "min_nb_journeys=3&_night_bus_filter_base_factor=86400"\
                .format(from_coord=s_coord, to_coord=r_coord, datetime="20120614T075500")
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)
        assert len(response["journeys"]) >= 3

    """
    test on date format
    """
    def test_journeys_no_date(self):
        """
        giving no date, we should have a response
        BUT, since without date we take the current date, it will not be in the production period,
        so we have a 'not un production period error'
        """

        query = "journeys?from={from_coord}&to={to_coord}"\
            .format(from_coord=s_coord, to_coord=r_coord)

        response, status_code = self.query_no_assert("v1/coverage/main_routing_test/" + query)

        assert status_code != 200, "the response should not be valid"

        assert response['error']['id'] == "date_out_of_bounds"
        assert response['error']['message'] == "date is not in data production period"

    def test_journeys_date_no_second(self):
        """giving no second in the date we should not be a problem"""

        query = "journeys?from={from_coord}&to={to_coord}&datetime={d}"\
            .format(from_coord=s_coord, to_coord=r_coord, d="20120614T0800")

        response = self.query_region(query)
        self.is_valid_journey_response(response, journey_basic_query)

        #and the second should be 0 initialized
        journeys = get_not_null(response, "journeys")
        assert journeys[0]["requested_date_time"] == "20120614T080000"

    def test_journeys_date_no_minute_no_second(self):
        """giving no minutes and no second in the date we should not be a problem"""

        query = "journeys?from={from_coord}&to={to_coord}&datetime={d}"\
            .format(from_coord=s_coord, to_coord=r_coord, d="20120614T08")

        response = self.query_region(query)
        self.is_valid_journey_response(response, journey_basic_query)

        #and the second should be 0 initialized
        journeys = get_not_null(response, "journeys")
        assert journeys[0]["requested_date_time"] == "20120614T080000"

    def test_journeys_date_too_long(self):
        """giving an invalid date (too long) should be a problem"""

        query = "journeys?from={from_coord}&to={to_coord}&datetime={d}"\
            .format(from_coord=s_coord, to_coord=r_coord, d="20120614T0812345")

        response, status_code = self.query_no_assert("v1/coverage/main_routing_test/" + query)

        assert not 'journeys' in response
        assert 'message' in response
        eq_(response['message'].lower(), "unable to parse datetime, unknown string format")

    def test_journeys_date_invalid(self):
        """giving the date with mmsshh (56 45 12) should be a problem"""

        query = "journeys?from={from_coord}&to={to_coord}&datetime={d}"\
            .format(from_coord=s_coord, to_coord=r_coord, d="20120614T564512")

        response, status_code = self.query_no_assert("v1/coverage/main_routing_test/" + query)

        assert not 'journeys' in response
        assert 'message' in response
        assert response['message'] == "Unable to parse datetime, hour must be in 0..23"

    def test_journeys_date_valid_invalid(self):
        """some format of date are bizarrely interpreted, and can result in date in 800"""

        query = "journeys?from={from_coord}&to={to_coord}&datetime={d}"\
            .format(from_coord=s_coord, to_coord=r_coord, d="T0800")

        response, status_code = self.query_no_assert("v1/coverage/main_routing_test/" + query)

        assert not 'journeys' in response
        assert 'message' in response
        assert response['message'] == "Unable to parse datetime, date is too early!"

    def test_journeys_bad_speed(self):
        """speed <= 0 is invalid"""

        for speed in ["0", "-1"]:
            for sn in ["walking", "bike", "bss", "car"]:
                query = "journeys?from={from_coord}&to={to_coord}&datetime={d}&{sn}_speed={speed}"\
                    .format(from_coord=s_coord, to_coord=r_coord, d="20120614T133700", sn=sn, speed=speed)

                response, status_code = self.query_no_assert("v1/coverage/main_routing_test/" + query)

                assert not 'journeys' in response
                assert 'message' in response
                assert response['message'] == \
                    "The {sn}_speed argument has to be > 0, you gave : {speed}"\
                        .format(sn=sn, speed=speed)

    def test_journeys_date_valid_not_zeropadded(self):
        """giving the date with non zero padded month should be a problem"""

        query = "journeys?from={from_coord}&to={to_coord}&datetime={d}"\
            .format(from_coord=s_coord, to_coord=r_coord, d="2012614T081025")

        response, status_code = self.query_no_assert("v1/coverage/main_routing_test/" + query)

        assert not 'journeys' in response
        assert 'message' in response
        assert response['message'] == "Unable to parse datetime, year is out of range"

    def test_journeys_do_not_loose_precision(self):
        """do we have a good precision given back in the id"""

        # this id was generated by giving an id to the api, and
        # copying it here the resulting id
        id = "8.98311981954709e-05;0.000898311281954"
        response = self.query_region("journeys?from={id}&to={id}&datetime={d}"
                                     .format(id=id, d="20120614T080000"))
        assert(len(response['journeys']) > 0)
        for j in response['journeys']:
            assert(j['sections'][0]['from']['id'] == id)
            assert(j['sections'][0]['from']['address']['id'] == id)
            assert(j['sections'][-1]['to']['id'] == id)
            assert(j['sections'][-1]['to']['address']['id'] == id)

    def test_journeys_wheelchair_profile(self):
        """
        Test a query with a wheelchair profile.
        We want to go from S to R after 8h as usual, but between S and R, the first VJ is not accessible,
        so we have to wait for the bus at 18h to leave
        """

        response = self.query_region(journey_basic_query + "&traveler_type=wheelchair")
        assert(len(response['journeys']) == 2)
        #Note: we do not test order, because that can change depending on the scenario
        eq_(sorted(get_used_vj(response)), sorted([[], ['vjB']]))
        eq_(sorted(get_arrivals(response)), sorted(['20120614T080612', '20120614T180250']))

        # same response if we just give the wheelchair=True
        response = self.query_region(journey_basic_query + "&traveler_type=wheelchair&wheelchair=True")
        assert(len(response['journeys']) == 2)
        eq_(sorted(get_used_vj(response)), sorted([[], ['vjB']]))
        eq_(sorted(get_arrivals(response)), sorted(['20120614T080612', '20120614T180250']))

        # but with the wheelchair profile, if we explicitly accept non accessible solutions (not very
        # consistent, but anyway), we should take the non accessible bus that arrive at 08h
        response = self.query_region(journey_basic_query + "&traveler_type=wheelchair&wheelchair=False")
        assert(len(response['journeys']) == 2)
        eq_(sorted(get_used_vj(response)), sorted([['vjA'], []]))
        eq_(sorted(get_arrivals(response)), sorted(['20120614T080250', '20120614T080612']))

    def test_journeys_float_night_bus_filter_max_factor(self):
        """night_bus_filter_max_factor can be a float (and can be null)"""

        query = "journeys?from={from_coord}&to={to_coord}&datetime={d}&" \
                         "_night_bus_filter_max_factor={_night_bus_filter_max_factor}"\
            .format(from_coord=s_coord, to_coord=r_coord, d="20120614T080000",
                    _night_bus_filter_max_factor=2.8)

        response = self.query_region(query)
        self.is_valid_journey_response(response, query)

        query = "journeys?from={from_coord}&to={to_coord}&datetime={d}&" \
                         "_night_bus_filter_max_factor={_night_bus_filter_max_factor}"\
            .format(from_coord=s_coord, to_coord=r_coord, d="20120614T080000",
                    _night_bus_filter_max_factor=0)

        response = self.query_region(query)
        self.is_valid_journey_response(response, query)


@dataset({})
class TestJourneysNoRegion(AbstractTestFixture):
    """
    If no region loaded we must have a polite error while asking for a journey
    """

    def test_with_region(self):
        response, status = self.query_no_assert("v1/coverage/non_existent_region/" + journey_basic_query)

        assert status != 200, "the response should not be valid"

        assert response['error']['id'] == "unknown_object"
        assert response['error']['message'] == "The region non_existent_region doesn't exists"

    def test_no_region(self):
        response, status = self.query_no_assert("v1/" + journey_basic_query)

        assert status != 200, "the response should not be valid"

        assert response['error']['id'] == "unknown_object"

        error_regexp = re.compile('^No region available for the coordinates.*')
        assert error_regexp.match(response['error']['message'])


@dataset({"basic_routing_test": {}})
class TestLongWaitingDurationFilter(AbstractTestFixture):
    """
    Test if the filter on long waiting duration is working
    """

    def test_novalidjourney_on_first_call(self):
        """
        On this call the first call to kraken returns a journey
        with a too long waiting duration.
        The second call to kraken must return a valid journey
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}"\
            .format(from_sa="A", to_sa="D", datetime="20120614T080000")

        response = self.query_region(query, display=False)
        eq_(len(response['journeys']), 1)
        logging.getLogger(__name__).info("arrival date: {}".format(response['journeys'][0]['arrival_date_time']))
        eq_(response['journeys'][0]['arrival_date_time'],  "20120614T160000")
        eq_(response['journeys'][0]['type'], "best")

        assert len(response["disruptions"]) == 0
        feed_publishers = response["feed_publishers"]
        for feed_publisher in feed_publishers:
            is_valid_feed_publisher(feed_publisher)

        feed_publisher = feed_publishers[0]
        assert (feed_publisher["id"] == "builder")
        assert (feed_publisher["name"] == "canal tp")
        assert (feed_publisher["license"] == "ODBL")
        assert (feed_publisher["url"] == "www.canaltp.fr")

        feed_publisher = feed_publishers[1]
        assert (feed_publisher["id"] == "base_contributor")
        assert (feed_publisher["name"] == "base contributor")
        assert (feed_publisher["license"] == "L-contributor")
        assert (feed_publisher["url"] == "www.canaltp.fr")

    def test_novalidjourney_on_first_call_debug(self):
        """
        On this call the first call to kraken returns a journey
        with a too long waiting duration.
        The second call to kraken must return a valid journey
        We had a debug argument, hence 2 journeys are returned, only one is typed
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}&debug=true"\
            .format(from_sa="A", to_sa="D", datetime="20120614T080000")

        response = self.query_region(query, display=False)
        eq_(len(response['journeys']), 2)
        eq_(response['journeys'][0]['arrival_date_time'], "20120614T150000")
        eq_(response['journeys'][0]['type'], "")
        eq_(response['journeys'][1]['arrival_date_time'], "20120614T160000")
        eq_(response['journeys'][1]['type'], "best")

    def test_datetime_error(self):
        """
        datetime invalid, we got an error
        """
        datetimes = ["20120614T080000Z", "2012-06-14T08:00:00.222Z"]
        for datetime in datetimes:
            query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}&debug=true"\
                .format(from_sa="A", to_sa="D", datetime=datetime)

            response, error_code = self.query_region(query, check=False)

            assert error_code == 400

            error = get_not_null(response, "error")

            assert error["message"] == "Unable to parse datetime, Not naive datetime (tzinfo is already set)"
            assert error["id"] == "unable_to_parse"


    def test_journeys_without_show_codes(self):
        '''
        Test journeys api without show_codes.
        The API's response contains the codes
        '''
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}"\
            .format(from_sa="A", to_sa="D", datetime="20120614T080000")

        response = self.query_region(query, display=False)
        eq_(len(response['journeys']), 1)
        eq_(len(response['journeys'][0]['sections']), 4)
        first_section = response['journeys'][0]['sections'][0]
        eq_(first_section['from']['stop_point']['codes'][0]['type'], 'external_code')
        eq_(first_section['from']['stop_point']['codes'][0]['value'], 'stop_point:A')

    def test_journeys_with_show_codes(self):
        '''
        Test journeys api with show_codes = false.
        The API's response contains the codes
        '''
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}&show_codes=false"\
            .format(from_sa="A", to_sa="D", datetime="20120614T080000")

        response = self.query_region(query, display=False)
        eq_(len(response['journeys']), 1)
        eq_(len(response['journeys'][0]['sections']), 4)
        first_section = response['journeys'][0]['sections'][0]
        eq_(first_section['from']['stop_point']['codes'][0]['type'], 'external_code')
        eq_(first_section['from']['stop_point']['codes'][0]['value'], 'stop_point:A')

    def test_remove_one_journey_from_batch(self):
        """
        Kraken returns two journeys, the earliest arrival one returns a too
        long waiting duration, therefore it must be deleted.
        The second one must be returned
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}"\
            .format(from_sa="A", to_sa="D", datetime="20120615T080000")

        response = self.query_region(query, display=False)
        eq_(len(response['journeys']), 1)
        eq_(response['journeys'][0]['arrival_date_time'], u'20120615T151000')
        eq_(response['journeys'][0]['type'], "best")


    def test_max_attemps(self):
        """
        Kraken always retrieves journeys with non_pt_duration > max_non_pt_duration
        No journeys should be typed, but get_journeys should stop quickly
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}"\
            .format(from_sa="E", to_sa="H", datetime="20120615T080000")

        response = self.query_region(query, display=False)
        assert(not "journeys" in response or len(response['journeys']) ==  0)


    def test_max_attemps_debug(self):
        """
        Kraken always retrieves journeys with non_pt_duration > max_non_pt_duration
        No journeys should be typed, but get_journeys should stop quickly
        We had the debug argument, hence a non-typed journey is returned
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}&debug=true"\
            .format(from_sa="E", to_sa="H", datetime="20120615T080000")

        response = self.query_region(query, display=False)
        eq_(len(response['journeys']), 1)


@dataset({"main_routing_test": {}})
class TestShapeInGeoJson(AbstractTestFixture):
    """
    Test if the shape is used in the GeoJson
    """

    def test_shape_in_geojson(self):
        """
        Test if, in the first journey, the second section:
         - is public_transport
         - len of stop_date_times is 2
         - len of geojson/coordinates is 3 (and thus,
           stop_date_times is not used to create the geojson)
        """
        response = self.query_region(journey_basic_query, display=False)
        #print response['journeys'][0]['sections'][1]
        eq_(len(response['journeys']), 2)
        eq_(len(response['journeys'][0]['sections']), 3)
        eq_(response['journeys'][0]['co2_emission']['value'], 0.58)
        eq_(response['journeys'][0]['co2_emission']['unit'], 'gEC')
        eq_(response['journeys'][0]['sections'][1]['type'], 'public_transport')
        eq_(len(response['journeys'][0]['sections'][1]['stop_date_times']), 2)
        eq_(len(response['journeys'][0]['sections'][1]['geojson']['coordinates']), 3)
        eq_(response['journeys'][0]['sections'][1]['co2_emission']['value'], 0.58)
        eq_(response['journeys'][0]['sections'][1]['co2_emission']['unit'], 'gEC')


@dataset({"main_routing_test": {}, "basic_routing_test": {'check_killed': False}})
class TestOneDeadRegion(AbstractTestFixture):
    """
    Test if we still responds when one kraken is dead
    """

    def test_one_dead_region(self):
        self.krakens_pool["basic_routing_test"].kill()

        response = self.query("v1/journeys?from=stop_point:stopA&"
            "to=stop_point:stopB&datetime=20120614T080000&debug=true")
        eq_(len(response['journeys']), 1)
        eq_(len(response['journeys'][0]['sections']), 1)
        eq_(response['journeys'][0]['sections'][0]['type'], 'public_transport')
        eq_(len(response['debug']['regions_called']), 1)
        eq_(response['debug']['regions_called'][0], "main_routing_test")


@dataset({"basic_routing_test": {}})
class TestIsochrone(AbstractTestFixture):
    def test_isochrone(self):
        response = self.query_region("journeys?from=I1&datetime=20120615T070000&max_duration=36000")
        assert(len(response['journeys']) == 2)


@dataset({"main_routing_without_pt_test": {'priority': 42}, "main_routing_test": {'priority': 10}})
class TestWithoutPt(AbstractTestFixture):
    """
    Test if we still responds when one kraken is dead
    """
    def test_one_region_wihout_pt(self):
        response = self.query("v1/"+journey_basic_query+"&debug=true",
                              display=False)
        eq_(len(response['journeys']), 2)
        eq_(len(response['journeys'][0]['sections']), 3)
        eq_(response['journeys'][0]['sections'][1]['type'], 'public_transport')
        eq_(len(response['debug']['regions_called']), 2)
        eq_(response['debug']['regions_called'][0], "main_routing_without_pt_test")
        eq_(response['debug']['regions_called'][1], "main_routing_test")
