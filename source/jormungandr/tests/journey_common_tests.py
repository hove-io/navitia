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

from six.moves.urllib.parse import quote

from .tests_mechanism import dataset
from jormungandr import i_manager
import mock
from .check_utils import *


def check_best(resp):
    assert not resp.get('journeys') or sum((1 for j in resp['journeys'] if j['type'] == "best")) == 1


@dataset({"main_routing_test": {}})
class JourneyCommon(object):

    """
    Test the structure of the journeys response
    """

    def test_journeys(self):
        #NOTE: we query /v1/coverage/main_routing_test/journeys and not directly /v1/journeys
        #not to use the jormungandr database
        response = self.query_region(journey_basic_query, display=True)
        check_best(response)
        self.is_valid_journey_response(response, journey_basic_query)

        feed_publishers = get_not_null(response, "feed_publishers")
        assert (len(feed_publishers) == 1)
        feed_publisher = feed_publishers[0]
        assert (feed_publisher["id"] == "builder")
        assert (feed_publisher["name"] == 'routing api data')
        assert (feed_publisher["license"] == "ODBL")
        assert (feed_publisher["url"] == "www.canaltp.fr")

        self.check_context(response)

    def test_error_on_journeys(self):
        """ if we got an error with kraken, an error should be returned"""

        query_out_of_production_bound = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}"\
            .format(from_coord="0.0000898312;0.0000898312",  # coordinate of S in the dataset
            to_coord="0.00188646;0.00071865",  # coordinate of R in the dataset
            datetime="20110614T080000")  # 2011 should not be in the production period

        response, status = self.query_region(query_out_of_production_bound, check=False)

        assert status != 200, "the response should not be valid"
        check_best(response)
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
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1

        assert response['journeys'][0]["type"] == "best"
        assert response['journeys'][0]['durations']['total'] == 99
        assert response['journeys'][0]['durations']['walking'] == 97
        assert response['journeys'][0]['durations']['bike'] == 0
        assert response['journeys'][0]['durations']['car'] == 0

    def test_other_filtering(self):
        """the basic query return a non pt walk journey and a best journey. we test the filtering of the non pt"""

        query = "{query}&type=non_pt_walk".format(query=journey_basic_query)
        response = self.query_region(query)
        # no best as non_pt_walk
        self.is_valid_journey_response(response, query)

        assert len(response['journeys']) == 1
        assert response['journeys'][0]["type"] == "non_pt_walk"

    def test_speed_factor_direct_path(self):
        """We test the coherence of the non pt walk solution with a speed factor"""

        query = "{query}&type=non_pt_walk&walking_speed=1.5".format(query=journey_basic_query)
        response = self.query_region(query)
        # no best as non_pt_walk
        self.is_valid_journey_response(response, query)

        journeys = response['journeys']
        assert journeys
        non_pt_walk_j = next((j for j in journeys if j['type'] == 'non_pt_walk'), None)
        assert non_pt_walk_j
        assert non_pt_walk_j['duration'] == non_pt_walk_j['sections'][0]['duration']
        # duration has floor round value: duration = 310 / 1.5 (speed-factor) = 206.66..
        assert non_pt_walk_j['duration'] == 206
        assert non_pt_walk_j['durations']['total'] == 206
        assert non_pt_walk_j['durations']['walking'] == 206

        assert non_pt_walk_j['departure_date_time'] == non_pt_walk_j['sections'][0]['departure_date_time']
        assert non_pt_walk_j['departure_date_time'] == '20120614T080000'
        assert non_pt_walk_j['arrival_date_time'] == non_pt_walk_j['sections'][0]['arrival_date_time']
        assert non_pt_walk_j['arrival_date_time'] == '20120614T080326'

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

        m = "parameter \"type\" invalid: The type argument must be in list [u'all', u'non_pt_bss', " \
            "u'non_pt_bike', u'fastest', u'less_fallback_bss', u'best', u'less_fallback_bike', " \
            "u'rapid', u'car', u'comfort', u'no_train', u'less_fallback_walk', u'non_pt_walk'], " \
            "you gave sponge_bob\n" \
            "type description: DEPRECATED, desired type of journey."
        assert response['message'] == m

    def test_journeys_no_bss_and_walking(self):
        query = journey_basic_query + "&first_section_mode=walking&first_section_mode=bss"
        response = self.query_region(query)

        check_best(response)
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

    def test_datetime_represents_arrival(self):
        """
        Checks journeys when datetime == start date of production datetime.
        """
        query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}&"\
                "min_nb_journeys=3&_night_bus_filter_base_factor=86400&"\
                "datetime_represents=arrival"\
                .format(from_coord=s_coord, to_coord=r_coord, datetime="20120614T185500")
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response["journeys"]) >= 3

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
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response["journeys"]) >= 3

    def test_min_nb_journeys_with_night_bus_filter(self):
        """
        Tests the combination of parameters _night_bus_filter_base_factor,
        _night_bus_filter_max_factor and min_nb_journeys

        1. _night_bus_filter_base_factor and _night_bus_filter_max_factor are used to limit the next
        journey with datetime of best response of last answer.

        2 To obtain at least "min_nb_journeys" journeys in the result each call to kraken
         in a loop is made with datatime + 1 of de best journey but not of global request.
        """

        #In this request only two journeys are found
        query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}&"\
                "min_nb_journeys=3&_night_bus_filter_base_factor=0&_night_bus_filter_max_factor=2"\
                .format(from_coord=s_coord, to_coord=r_coord, datetime="20120614T075500")
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response["journeys"]) == 3

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
        check_best(response)
        self.is_valid_journey_response(response, journey_basic_query)

        #and the second should be 0 initialized
        journeys = get_not_null(response, "journeys")
        assert journeys[0]["requested_date_time"] == "20120614T080000"

    def test_journeys_date_no_minute_no_second(self):
        """giving no minutes and no second in the date we should not be a problem"""

        query = "journeys?from={from_coord}&to={to_coord}&datetime={d}"\
            .format(from_coord=s_coord, to_coord=r_coord, d="20120614T08")

        response = self.query_region(query)
        check_best(response)
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
        assert "unable to parse datetime, unknown string format" in response['message'].lower()

    def test_journeys_date_invalid(self):
        """giving the date with mmsshh (56 45 12) should be a problem"""

        query = "journeys?from={from_coord}&to={to_coord}&datetime={d}"\
            .format(from_coord=s_coord, to_coord=r_coord, d="20120614T564512")

        response, status_code = self.query_no_assert("v1/coverage/main_routing_test/" + query)

        assert not 'journeys' in response
        assert 'message' in response
        assert "Unable to parse datetime, hour must be in 0..23" in response['message']

    def test_journeys_date_valid_invalid(self):
        """some format of date are bizarrely interpreted, and can result in date in 800"""

        query = "journeys?from={from_coord}&to={to_coord}&datetime={d}"\
            .format(from_coord=s_coord, to_coord=r_coord, d="T0800")

        response, status_code = self.query_no_assert("v1/coverage/main_routing_test/" + query)

        assert not 'journeys' in response
        assert 'message' in response
        assert "Unable to parse datetime, date is too early!" in response['message']

    def test_journeys_bad_speed(self):
        """speed <= 0 is invalid"""

        for speed in ["0", "-1"]:
            for sn in ["walking", "bike", "bss", "car"]:
                query = "journeys?from={from_coord}&to={to_coord}&datetime={d}&{sn}_speed={speed}"\
                    .format(from_coord=s_coord, to_coord=r_coord, d="20120614T133700", sn=sn, speed=speed)

                response, status_code = self.query_no_assert("v1/coverage/main_routing_test/" + query)

                assert not 'journeys' in response
                assert 'message' in response
                assert "The {sn}_speed argument has to be > 0, you gave : {speed}".format(sn=sn, speed=speed)\
                       in response['message']

    def test_journeys_date_valid_not_zeropadded(self):
        """giving the date with non zero padded month should be a problem"""

        query = "journeys?from={from_coord}&to={to_coord}&datetime={d}"\
            .format(from_coord=s_coord, to_coord=r_coord, d="2012614T081025")

        response, status_code = self.query_no_assert("v1/coverage/main_routing_test/" + query)

        assert not 'journeys' in response
        assert 'message' in response
        assert "Unable to parse datetime, year is out of range" in response['message']

    def test_journeys_do_not_loose_precision(self):
        """do we have a good precision given back in the id"""

        # this id was generated by giving an id to the api, and
        # copying it here the resulting id
        id = "8.98311981954709e-05;0.000898311281954"
        query = "journeys?from={id}&to={id}&datetime={d}".format(id=id, d="20120614T080000")
        response = self.query_region(query)
        # no best as non_pt_walk
        self.is_valid_journey_response(response, query)

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
        query = journey_basic_query + "&traveler_type=wheelchair"
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        assert(len(response['journeys']) == 2)

        def compare(l1, l2):
            assert sorted(l1) == sorted(l2)

        #Note: we do not test order, because that can change depending on the scenario
        compare(get_used_vj(response), [[], ['vjB']])
        compare(get_arrivals(response), ['20120614T080613', '20120614T180250'])

        # same response if we just give the wheelchair=True
        query = journey_basic_query + "&traveler_type=wheelchair&wheelchair=True"
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        assert(len(response['journeys']) == 2)
        compare(get_used_vj(response), [[], ['vjB']])
        compare(get_arrivals(response), ['20120614T080613', '20120614T180250'])

        # but with the wheelchair profile, if we explicitly accept non accessible solutions (not very
        # consistent, but anyway), we should take the non accessible bus that arrive at 08h
        query = journey_basic_query + "&traveler_type=wheelchair&wheelchair=False"
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        assert(len(response['journeys']) == 2)
        compare(get_used_vj(response), [['vjA'], []])
        compare(get_arrivals(response), ['20120614T080250', '20120614T080613'])

    def test_journeys_float_night_bus_filter_max_factor(self):
        """night_bus_filter_max_factor can be a float (and can be null)"""

        query = "journeys?from={from_coord}&to={to_coord}&datetime={d}&" \
                         "_night_bus_filter_max_factor={_night_bus_filter_max_factor}"\
            .format(from_coord=s_coord, to_coord=r_coord, d="20120614T080000",
                    _night_bus_filter_max_factor=2.8)

        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        query = "journeys?from={from_coord}&to={to_coord}&datetime={d}&" \
                         "_night_bus_filter_max_factor={_night_bus_filter_max_factor}"\
            .format(from_coord=s_coord, to_coord=r_coord, d="20120614T080000",
                    _night_bus_filter_max_factor=0)

        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

    def test_sp_to_sp(self):
        """
        Test journeys from stop point to stop point
        """

        query = "journeys?from=stop_point:uselessA&to=stop_point:stopB&datetime=20120615T080000"

        # with street network desactivated
        response = self.query_region(query + "&max_duration_to_pt=0")
        assert('journeys' not in response)

        # with street network activated
        query += "&max_duration_to_pt=1"
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1
        assert response['journeys'][0]['sections'][0]['from']['id'] == 'stop_point:uselessA'
        assert response['journeys'][0]['sections'][0]['to']['id'] == 'stop_point:stopA'
        assert response['journeys'][0]['sections'][0]['type'] == 'street_network'
        assert response['journeys'][0]['sections'][0]['mode'] == 'walking'
        assert response['journeys'][0]['sections'][0]['duration'] == 0

        query = "journeys?from=stop_point:stopA&to=stop_point:stopB&datetime=20120615T080000"
        query += "&max_duration_to_pt=0"
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1
        assert response['journeys'][0]['departure_date_time'] == u'20120615T080100'
        assert response['journeys'][0]['arrival_date_time'] == u'20120615T080102'


    @mock.patch.object(i_manager, 'dispatch')
    def test_max_duration_to_pt(self, mock):
        q = "v1/coverage/main_routing_test/journeys?max_duration_to_pt=0&from=toto&to=tata"
        self.query(q)
        max_walking = i_manager.dispatch.call_args[0][0]["max_walking_duration_to_pt"]
        max_bike = i_manager.dispatch.call_args[0][0]['max_bike_duration_to_pt']
        max_bss = i_manager.dispatch.call_args[0][0]['max_bss_duration_to_pt']
        max_car = i_manager.dispatch.call_args[0][0]['max_car_duration_to_pt']
        assert max_walking == 0
        assert max_bike == 0
        assert max_bss == 0
        assert max_car == 0

    def test_traveler_type(self):
        q_fast_walker = journey_basic_query + "&traveler_type=fast_walker"
        response_fast_walker = self.query_region(q_fast_walker)
        check_best(response_fast_walker)
        self.is_valid_journey_response(response_fast_walker, q_fast_walker)
        basic_response = self.query_region(journey_basic_query)
        check_best(basic_response)
        self.is_valid_journey_response(basic_response, journey_basic_query)

        def bike_in_journey(fast_walker):
            return any(sect_fast_walker["mode"] == 'bike' for sect_fast_walker in fast_walker['sections']
                       if 'mode' in sect_fast_walker)

        def no_bike_in_journey(journey):
            return all(section['mode'] != 'bike' for section in journey['sections'] if 'mode' in section)

        assert any(bike_in_journey(journey_fast_walker) for journey_fast_walker in response_fast_walker['journeys'])
        assert all(no_bike_in_journey(journey) for journey in basic_response['journeys'])

    def test_shape_in_geojson(self):
        """
        Test if, in the first journey, the second section:
         - is public_transport
         - len of stop_date_times is 2
         - len of geojson/coordinates is 3 (and thus,
           stop_date_times is not used to create the geojson)
        """
        response = self.query_region(journey_basic_query, display=False)
        check_best(response)
        self.is_valid_journey_response(response, journey_basic_query)
        #print response['journeys'][0]['sections'][1]
        assert len(response['journeys']) == 2
        assert len(response['journeys'][0]['sections']) == 3
        assert response['journeys'][0]['co2_emission']['value'] == 0.58
        assert response['journeys'][0]['co2_emission']['unit'] == 'gEC'
        assert response['journeys'][0]['sections'][1]['type'] == 'public_transport'
        assert len(response['journeys'][0]['sections'][1]['stop_date_times']) == 2
        assert len(response['journeys'][0]['sections'][1]['geojson']['coordinates']) == 3
        assert response['journeys'][0]['sections'][1]['co2_emission']['value'] == 0.58
        assert response['journeys'][0]['sections'][1]['co2_emission']['unit'] == 'gEC'
        assert response['journeys'][1]['duration'] == 276
        assert response['journeys'][1]['durations']['total'] == 276
        assert response['journeys'][1]['durations']['walking'] == 276

    def test_max_duration_to_pt_equals_to_0(self):
        query = journey_basic_query + \
            "&first_section_mode[]=bss" + \
            "&first_section_mode[]=walking" + \
            "&first_section_mode[]=bike" + \
            "&first_section_mode[]=car" + \
            "&debug=true"
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 4

        assert response['journeys'][0]['durations']['bike'] == 62
        assert response['journeys'][0]['durations']['total'] == 62
        assert response['journeys'][0]['distances']['bike'] == 257
        assert response['journeys'][0]['durations']['walking'] == 0
        assert response['journeys'][1]['durations']['car'] == 16
        assert response['journeys'][1]['durations']['walking'] == 106
        assert response['journeys'][1]['durations']['total'] == 123
        assert response['journeys'][1]['distances']['car'] == 186
        assert response['journeys'][1]['distances']['walking'] == 119

        query += "&max_duration_to_pt=0"
        response, status = self.query_no_assert(query)
        # pas de solution
        assert status == 404
        assert('journeys' not in response)

    def test_max_duration_to_pt_equals_to_0_from_stop_point(self):
        query = "journeys?from=stop_point%3AstopA&to=stop_point%3AstopC&datetime=20120614T080000"
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 2

        query += "&max_duration_to_pt=0"
        #There is no direct_path but a journey using Metro
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        jrnys = response['journeys']
        assert len(jrnys) == 1
        section = jrnys[0]['sections'][0]
        assert section['type'] == 'public_transport'
        assert section['from']['id'] == 'stop_point:stopA'
        assert section['to']['id'] == 'stop_point:stopC'

    def test_max_duration_to_pt_equals_to_0_from_stop_area(self):
        query = "journeys?from=stopA&to=stopB&datetime=20120614T080000"
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 2

        query += "&max_duration_to_pt=0"
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        jrnys = response['journeys']
        assert len(jrnys) == 1
        section = jrnys[0]['sections'][0]
        # from departure(a stop area) to stop_point
        assert section['type'] == 'crow_fly'
        assert section['from']['id'] == 'stopA'
        assert section['to']['id'] == 'stop_point:stopA'

        section = jrnys[0]['sections'][1]
        assert section['type'] == 'public_transport'
        assert section['from']['id'] == 'stop_point:stopA'
        assert section['to']['id'] == 'stop_point:stopB'

        section = jrnys[0]['sections'][2]
        # from a stop point to destination(a stop area)
        assert section['type'] == 'crow_fly'
        assert section['from']['id'] == 'stop_point:stopB'
        assert section['to']['id'] == 'stopB'

    def test_max_duration_equals_to_0(self):
        query = journey_basic_query + \
            "&first_section_mode[]=bss" + \
            "&first_section_mode[]=walking" + \
            "&first_section_mode[]=bike" + \
            "&first_section_mode[]=car" + \
            "&debug=true"
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 4

        query += "&max_duration=0"
        response = self.query_region(query)
        # the pt journey is eliminated
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 3

        check_best(response)

        # first is bike
        assert('bike' in response['journeys'][0]['tags'])
        assert(response['journeys'][0]['debug']['internal_id'])
        assert len(response['journeys'][0]['sections']) == 1

        # second is car
        assert('car' in response['journeys'][1]['tags'])
        assert(response['journeys'][1]['debug']['internal_id'])
        assert len(response['journeys'][1]['sections']) == 3

        # last is walking
        assert('walking' in response['journeys'][-1]['tags'])
        assert(response['journeys'][-1]['debug']['internal_id'])
        assert len(response['journeys'][-1]['sections']) == 1

    def test_journey_stop_area_to_stop_point(self):
        """
        When the departure is stop_area:A and the destination is stop_point:B belonging to stop_area:B
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}"\
            .format(from_sa='stopA', to_sa='stop_point:stopB', datetime="20120614T080000")
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        jrnys = response['journeys']

        j = next((j for j in jrnys if j['type'] == 'non_pt_walk'), None)
        assert j
        assert j['sections'][0]['from']['id'] == 'stopA'
        assert j['sections'][0]['to']['id'] == 'stop_point:stopB'
        assert 'walking' in j['tags']

    def test_journey_from_non_valid_stop_area(self):
        """
        When the departure is a non valid stop_area, the response status should be 404
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}"\
            .format(from_sa='stop_area:non_valid', to_sa='stop_point:stopB', datetime="20120614T080000")
        response = self.query_region(query, check=False)
        assert response[1] == 404
        assert response[0]['error']['message'] == u'The entry point: stop_area:non_valid is not valid'
        assert response[0]['error']['id'] == u'unknown_object'

    def test_crow_fly_sections(self):
        """
        When the departure is a stop_area...
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}"\
            .format(from_sa='stopA', to_sa='stopB', datetime="20120614T080000")
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        jrnys = response['journeys']
        assert len(jrnys) == 2
        section_0 = jrnys[0]['sections'][0]
        assert section_0['type'] == 'crow_fly'
        assert section_0['from']['id'] == 'stopA'
        assert section_0['to']['id'] == 'stop_point:stopA'

        section_2 = jrnys[0]['sections'][2]
        assert section_2['type'] == 'crow_fly'
        assert section_2['from']['id'] == 'stop_point:stopB'
        assert section_2['to']['id'] == 'stopB'

    def test_no_origin_point(self):
        """
        Here we verify no destination point error
        """
        query_out_of_production_bound = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}"\
            .format(from_coord="0.9008898312;0.0019898312",  # coordinate out of range in the dataset
            to_coord="0.00188646;0.00071865",  # coordinate of R in the dataset
            datetime="20120614T080000")

        response, status = self.query_region(query_out_of_production_bound, check=False)

        assert status != 200, "the response should not be valid"
        check_best(response)
        assert response['error']['id'] == "no_origin"
        assert response['error']['message'] == "no origin point"

        #and no journey is to be provided
        assert 'journeys' not in response or len(response['journeys']) == 0

    def test_no_destination_point(self):
        """
        Here we verify no destination point error
        """
        query_out_of_production_bound = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}"\
            .format(from_coord="0.0000898312;0.0000898312",  # coordinate of S in the dataset
            to_coord="0.00188646;0.90971865",  # coordinate out of range in the dataset
            datetime="20120614T080000")

        response, status = self.query_region(query_out_of_production_bound, check=False)

        assert status != 200, "the response should not be valid"
        check_best(response)
        assert response['error']['id'] == "no_destination"
        assert response['error']['message'] == "no destination point"

        #and no journey is to be provided
        assert 'journeys' not in response or len(response['journeys']) == 0

    def test_no_origin_nor_destination(self):
        """
        Here we verify no origin nor destination point error
        """
        query_out_of_production_bound = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}"\
            .format(from_coord="0.9008898312;0.0019898312",  # coordinate out of range in the dataset
            to_coord="0.00188646;0.90971865",  # coordinate out of range in the dataset
            datetime="20120614T080000")

        response, status = self.query_region(query_out_of_production_bound, check=False)

        assert status != 200, "the response should not be valid"
        check_best(response)
        assert response['error']['id'] == "no_origin_nor_destination"
        assert response['error']['message'] == "no origin point nor destination point"

        #and no journey is to be provided
        assert 'journeys' not in response or len(response['journeys']) == 0

    def test_no_solution(self):
        """
        Here we verify no destination point error
        """
        query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}"\
            .format(from_coord="0.0000898312;0.0000898312",  # coordinate out of range in the dataset
            to_coord="0.00188646;0.00071865",  # coordinate out of range in the dataset
            datetime="20120901T220000")

        response, status = self.query_region(query + "&max_duration=1&max_duration_to_pt=100", check=False)

        assert status == 200
        check_best(response)
        assert response['error']['id'] == "no_solution"
        assert response['error']['message'] == "no solution found for this journey"

        #and no journey is to be provided
        assert 'journeys' not in response or len(response['journeys']) == 0

    def test_call_kraken_foreach_mode(self):
        '''
        test if the different pt computation do not interfer
        '''
        query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}&first_section_mode[]=walking&first_section_mode[]=bike&debug=true"\
            .format(from_coord="0.0000898312;0.0000898312",
                    to_coord="0.00188646;0.00071865",
                    datetime="20120614T080000")

        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 3
        assert len(response['journeys'][0]['sections']) == 1
        assert response['journeys'][0]['sections'][0]['mode'] == 'bike'
        assert len(response['journeys'][1]['sections']) == 3
        assert response['journeys'][1]['sections'][0]['mode'] == 'walking'
        assert len(response['journeys'][2]['sections']) == 1
        assert response['journeys'][2]['sections'][0]['mode'] == 'walking'

        query += '&bike_speed=1.5'
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 4
        print(response['journeys'][0]['tags'])
        print(response['journeys'][1]['tags'])
        print(response['journeys'][2]['tags'])
        print(response['journeys'][3]['tags'])
        assert len(response['journeys'][0]['sections']) == 3
        assert response['journeys'][0]['sections'][0]['mode'] == 'bike'
        assert len(response['journeys'][1]['sections']) == 3
        assert response['journeys'][1]['sections'][0]['mode'] == 'walking'
        assert len(response['journeys'][2]['sections']) == 1
        assert response['journeys'][2]['sections'][0]['mode'] == 'bike'
        assert len(response['journeys'][3]['sections']) == 1
        assert response['journeys'][3]['sections'][0]['mode'] == 'walking'


    def test_call_kraken_boarding_alighting(self):
        '''
        test that boarding and alighting sections are present
        '''
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}&debug=true&max_duration_to_pt=0"\
                    .format(from_sa="stopA",
                            to_sa="stopB",
                            datetime="20120614T223000")

        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        assert len(response['journeys']) == 1
        journey = response['journeys'][0]

        assert journey['sections'][0]['from']['id'] == 'stopA'
        assert journey['sections'][0]['to']['id'] == 'stop_point:stopA'
        assert journey['sections'][0]['type'] == 'crow_fly'
        assert journey['sections'][0]['duration'] == 0
        assert journey['sections'][0]['departure_date_time'] == '20120614T223200'
        assert journey['sections'][0]['arrival_date_time'] == '20120614T223200'

        assert journey['sections'][1]['from']['id'] == 'stop_point:stopA'
        assert journey['sections'][1]['to']['id'] == 'stop_point:stopA'
        assert journey['sections'][1]['type'] == 'boarding'
        assert journey['sections'][1]['duration'] == 1800
        assert journey['sections'][1]['departure_date_time'] == '20120614T223200'
        assert journey['sections'][1]['arrival_date_time'] == '20120614T230200'

        assert journey['sections'][2]['from']['id'] == 'stop_point:stopA'
        assert journey['sections'][2]['to']['id'] == 'stop_point:stopB'
        assert journey['sections'][2]['type'] == 'public_transport'
        assert journey['sections'][2]['duration'] == 180
        assert journey['sections'][2]['departure_date_time'] == '20120614T230200'
        assert journey['sections'][2]['arrival_date_time'] == '20120614T230500'

        assert journey['sections'][3]['from']['id'] == 'stop_point:stopB'
        assert journey['sections'][3]['to']['id'] == 'stop_point:stopB'
        assert journey['sections'][3]['type'] == 'alighting'
        assert journey['sections'][3]['duration'] == 1800
        assert journey['sections'][3]['departure_date_time'] == '20120614T230500'
        assert journey['sections'][3]['arrival_date_time'] == '20120614T233500'

        assert journey['sections'][4]['from']['id'] == 'stop_point:stopB'
        assert journey['sections'][4]['to']['id'] == 'stopB'
        assert journey['sections'][4]['type'] == 'crow_fly'
        assert journey['sections'][4]['duration'] == 0
        assert journey['sections'][4]['departure_date_time'] == '20120614T233500'
        assert journey['sections'][4]['arrival_date_time'] == '20120614T233500'

    def test_journey_from_non_valid_object_type(self):
        """
        When the departure is not a Place, the response status should be 404
        """
        query = "vehicle_journeys/vehicle_journey:SNC/journeys?datetime={datetime}".format(datetime="20120614T080000")
        response, status = self.query_region(query, check=False)
        assert status == 404
        assert response['error']['id'] == u'unknown_object'
        assert response['error']['message'] == u'The entry point: vehicle_journey:SNC is not valid'

    def test_free_radius_from(self):
        # The coordinates of departure and the stop point are separated by 20m
        # Query journeys with free_radius = 0
        r = self.query('/v1/coverage/main_routing_test/journeys?from=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&to=stopA&datetime=20120614080000')
        assert(r['journeys'][0]['sections'][0]['type'] == 'street_network')
        assert(r['journeys'][0]['sections'][0]['duration'] != 0)

        # Query journeys with free_radius = 19
        r = self.query('/v1/coverage/main_routing_test/journeys?from=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&to=stopA&datetime=20120614080000&free_radius_from=19')
        assert(r['journeys'][0]['sections'][0]['type'] == 'street_network')
        assert(r['journeys'][0]['sections'][0]['duration'] != 0)

        # Query journeys with free_radius = 20
        r = self.query('/v1/coverage/main_routing_test/journeys?from=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&to=stopA&datetime=20120614080000&free_radius_from=20')
        assert(r['journeys'][0]['sections'][0]['type'] == 'crow_fly')
        assert(r['journeys'][0]['sections'][0]['duration'] == 0)

        # The time of departure of PT is 08:01:00 and it takes 17s to walk to the station
        # If the requested departure time is 08:00:50, the PT journey shouldn't be displayed
        r = self.query('/v1/coverage/main_routing_test/journeys?from=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&to=stopA&datetime=20120614T080050&datetime_represents=departure&')
        assert(r['journeys'][0]['sections'][0]['type'] == 'street_network')

        # With the free_radius, the PT journey is displayed thanks to the 'free' crow_fly
        r = self.query('/v1/coverage/main_routing_test/journeys?from=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&to=stopA&datetime=20120614T080100&free_radius_from=20&datetime_represents=departure&')
        assert(len(r['journeys'][0]['sections']) > 1)
        assert(r['journeys'][0]['sections'][0]['type'] == 'crow_fly')
        assert(r['journeys'][0]['sections'][0]['duration'] == 0)
        assert(r['journeys'][0]['sections'][1]['type'] == 'public_transport')

    def test_free_radius_to(self):
        # The coordinates of arrival and the stop point are separated by 20m
        # Query journeys with free_radius = 0
        r = self.query('/v1/coverage/main_routing_test/journeys?from=stopA&to=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&datetime=20120614080000&')
        assert(r['journeys'][0]['sections'][-1]['type'] == 'street_network')
        assert(r['journeys'][0]['sections'][-1]['duration'] != 0)

        # Query journeys with free_radius = 19
        r = self.query('/v1/coverage/main_routing_test/journeys?from=stopA&to=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&datetime=20120614080000&free_radius_from=19')
        assert(r['journeys'][0]['sections'][-1]['type'] == 'street_network')
        assert(r['journeys'][0]['sections'][-1]['duration'] != 0)

        # Query journeys with free_radius = 20
        r = self.query('/v1/coverage/main_routing_test/journeys?from=stopA&to=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&datetime=20120614080000&free_radius_to=20&')
        assert(r['journeys'][0]['sections'][-1]['type'] == 'crow_fly')
        assert(r['journeys'][0]['sections'][-1]['duration'] == 0)

        # The estimated time of arrival without free_radius is 08:01:19
        # If the requested arrival time is before 08:01:19, the PT journey shouldn't be displayed
        r = self.query('/v1/coverage/main_routing_test/journeys?from=stopA&to=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&datetime=20120614T080118&free_radius_to=19&datetime_represents=arrival&')
        assert(r['journeys'][0]['sections'][0]['type'] == 'street_network')

        # With the free_radius, the PT journey is displayed thanks to the 'free' crow_fly
        r = self.query('/v1/coverage/main_routing_test/journeys?from=stopA&to=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&datetime=20120614T080118&free_radius_to=20&_override_scenario=experimental&datetime_represents=arrival&')
        assert(len(r['journeys'][0]['sections']) > 1)
        assert(r['journeys'][0]['sections'][-1]['type'] == 'crow_fly')
        assert(r['journeys'][0]['sections'][-1]['duration'] == 0)
        assert(r['journeys'][0]['sections'][-2]['type'] == 'public_transport')

    def test_shared_section(self):
        # Query a journey from stopB to stopA
        r = self.query('/v1/coverage/main_routing_test/journeys?to=stopA&from=stopB&datetime=20120614T080100&')
        assert r['journeys'][0]['type'] == 'best'
        assert r['journeys'][0]['sections'][1]['type'] == 'public_transport'
        first_journey_pt = r['journeys'][0]['sections'][1]['display_informations']['name']

        # Query same journey schedules
        # A new journey vjM is available
        r = self.query('v1/coverage/main_routing_test/journeys?allowed_id%5B%5D=stop_point%3AstopA&allowed_id%5B%5D=stop_point%3AstopB&first_section_mode%5B%5D=walking&last_section_mode%5B%5D=walking&is_journey_schedules=True&datetime=20120614T080100&to=stopA&min_nb_journeys=5&min_nb_transfers=0&direct_path=none&from=stopB&')
        assert r['journeys'][0]['sections'][1]['display_informations']['name'] == first_journey_pt
        assert r['journeys'][0]['sections'][1]['type'] == 'public_transport'
        next_journey_pt = r['journeys'][1]['sections'][1]['display_informations']['name']
        assert next_journey_pt != first_journey_pt

        # Activate 'no_shared_section' parameter and query the same journey scehdules again
        # The journey vjM isn't available as it is a shared section
        r = self.query('v1/coverage/main_routing_test/journeys?allowed_id%5B%5D=stop_point%3AstopA&allowed_id%5B%5D=stop_point%3AstopB&first_section_mode%5B%5D=walking&last_section_mode%5B%5D=walking&is_journey_schedules=True&datetime=20120614T080100&to=stopA&min_nb_journeys=5&min_nb_transfers=0&direct_path=none&from=stopB&no_shared_section=True&')
        assert r['journeys'][0]['sections'][1]['display_informations']['name'] == first_journey_pt
        assert not r['journeys'][1]


@dataset({"main_stif_test": {}})
class AddErrorFieldInJormun(object):
    def test_add_error_field(self):
            """
            The goal of this test is to put forward the addition of the error
            in the Jormungandr part.
            Kraken send journeys but after the Jormungandr filtering, there is
            no more journeys field. So Jormungandr have to add an error field
            with "no_solution" id and "no solution found for this journey"
            message.
            """
            query = "journeys?from={from_sp}&to={to_sp}&datetime={datetime}" \
                "&datetime_represents=arrival&_max_successive_physical_mode=3&_max_additional_connections=10"\
            .format(from_sp="stopP", to_sp="stopT", datetime="20140614T193000")

            response, status = self.query_region(query + "&max_duration=1&max_duration_to_pt=100", check=False)

            assert status == 200
            check_best(response)
            assert response['error']['id'] == "no_solution"
            assert response['error']['message'] == "no solution found for this journey"
            #and no journey is to be provided
            assert 'journeys' not in response or len(response['journeys']) == 0

@dataset({"main_routing_test": {}})
class DirectPath(object):
    def test_journey_direct_path(self):
        query = journey_basic_query + \
                "&first_section_mode[]=bss" + \
                "&first_section_mode[]=walking" + \
                "&first_section_mode[]=bike" + \
                "&first_section_mode[]=car" + \
                "&debug=true"
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 4

        # first is bike
        assert('bike' in response['journeys'][0]['tags'])
        assert len(response['journeys'][0]['sections']) == 1

        # second is car
        assert('car' in response['journeys'][1]['tags'])
        assert len(response['journeys'][1]['sections']) == 3

        # last is walking
        assert('walking' in response['journeys'][-1]['tags'])
        assert len(response['journeys'][-1]['sections']) == 1

    def test_journey_direct_path_only(self):
        query = journey_basic_query + \
                "&first_section_mode[]=walking" + \
                "&direct_path=only"
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1

        # only walking
        assert('non_pt' in response['journeys'][0]['tags'])
        assert len(response['journeys'][0]['sections']) == 1

    def test_journey_direct_path_none(self):
        query = journey_basic_query + \
                "&first_section_mode[]=walking" + \
                "&direct_path=none"
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1

        # only pt journey
        assert('non_pt' not in response['journeys'][0]['tags'])
        assert(len(response['journeys'][0]['sections']) > 1)

    def test_journey_direct_path_indifferent(self):
        query = journey_basic_query + \
                "&first_section_mode[]=walking" + \
                "&direct_path=indifferent"
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 2

        # first is pt journey
        assert('non_pt' not in response['journeys'][0]['tags'])
        assert(len(response['journeys'][0]['sections']) > 1)
        # second is walking
        assert('non_pt' in response['journeys'][1]['tags'])
        assert len(response['journeys'][1]['sections']) == 1


@dataset({})
class JourneysNoRegion():
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
class OnBasicRouting():
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
        check_best(response)
        #self.is_valid_journey_response(response, query)# linestring with 1 value (0,0)
        assert len(response['journeys']) == 1
        assert response['journeys'][0]['arrival_date_time'] ==  "20120614T160000"
        assert response['journeys'][0]['type'] == "best"

        assert len(response["disruptions"]) == 0
        feed_publishers = response["feed_publishers"]
        assert len(feed_publishers) == 2
        for feed_publisher in feed_publishers:
            is_valid_feed_publisher(feed_publisher)

        feed_publisher = next(f for f in feed_publishers if f['id'] == "base_contributor")
        assert (feed_publisher["name"] == "base contributor")
        assert (feed_publisher["license"] == "L-contributor")
        assert (feed_publisher["url"] == "www.canaltp.fr")

        osm = next(f for f in feed_publishers if f['id'] == "osm")
        assert (osm["name"] == "openstreetmap")
        assert (osm["license"] == "ODbL")
        assert (osm["url"] == "https://www.openstreetmap.org/copyright")

    def test_sp_outside_georef(self):
        """
        departure from '5.;5.' coordinates outside street network
        """
        query = "journeys?from={coord}&to={to_sa}&datetime={datetime}"\
            .format(coord="5.;5.", to_sa="H", datetime="20120615T170000")

        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert('journeys' in response)
        assert len(response['journeys']) == 1
        assert response['journeys'][0]['sections'][0]['to']['id'] == 'G'
        assert response['journeys'][0]['sections'][0]['type'] == 'crow_fly'
        assert response['journeys'][0]['sections'][0]['mode'] == 'walking'

        assert response['journeys'][0]['sections'][1]['from']['id'] == 'G'
        assert response['journeys'][0]['sections'][1]['to']['id'] == 'H'
        assert response['journeys'][0]['sections'][1]['type'] == 'public_transport'
        assert response['journeys'][0]['duration'] == 3600

    def test_novalidjourney_on_first_call_debug(self):
        """
        On this call the first call to kraken returns a journey
        with a too long waiting duration.
        The second call to kraken must return a valid journey
        We had a debug argument, hence 2 journeys are returned, only one is typed
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}&debug=true"\
            .format(from_sa="A", to_sa="D", datetime="20120614T080000")
        response = self.query_region(query, display=True)
        check_best(response)
        #self.is_valid_journey_response(response, query)# linestring with 1 value (0,0)
        assert len(response['journeys']) == 2
        assert response['journeys'][0]['arrival_date_time'] == "20120614T150000"
        assert('to_delete' in response['journeys'][0]['tags'])
        assert response['journeys'][1]['arrival_date_time'] == "20120614T160000"
        assert response['journeys'][1]['type'] == "fastest"

    def test_datetime_error(self):
        """
        datetime invalid, we got an error
        """
        def journey(dt):
            return self.query_region(
                "journeys?from={from_sa}&to={to_sa}&datetime={datetime}&debug=true"
                    .format(from_sa="A", to_sa="D", datetime=dt),
                check=False)

        # invalid month
        resp, code = journey("20121501T100000")
        assert code == 400
        assert 'month must be in 1..12' in resp['message']

        # too much digit
        _, code = journey("201215001T100000")
        assert code == 400

    def test_datetime_withtz(self):
        """
        datetime invalid, we got an error
        """
        def journey(dt):
            return self.query_region("journeys?from={from_sa}&to={to_sa}&datetime={datetime}&debug=true"
                                     .format(from_sa="A", to_sa="D", datetime=dt))

        # those should not raise an error
        journey("20120615T080000Z")
        journey("2012-06-15T08:00:00.222Z")

        # it should work also with another timezone (and for fun another format)
        journey(quote("2012-06-15 08-00-00+02"))

    def test_remove_one_journey_from_batch(self):
        """
        Kraken returns two journeys, the earliest arrival one returns a too
        long waiting duration, therefore it must be deleted.
        The second one must be returned
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}"\
            .format(from_sa="A", to_sa="D", datetime="20120615T080000")

        response = self.query_region(query, display=False)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1
        assert response['journeys'][0]['arrival_date_time'] == u'20120615T151000'
        assert response['journeys'][0]['type'] == "best"

    def test_max_attemps(self):
        """
        Kraken always retrieves journeys with non_pt_duration > max_non_pt_duration
        No journeys should be typed, but get_journeys should stop quickly
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}"\
            .format(from_sa="E", to_sa="H", datetime="20120615T080000")

        response = self.query_region(query, display=False)
        assert(not "journeys" in response or len(response['journeys']) == 0)

    def test_max_attemps_debug(self):
        """
        Kraken always retrieves journeys with non_pt_duration > max_non_pt_duration
        No journeys should be typed, but get_journeys should stop quickly
        We had the debug argument, hence a non-typed journey is returned
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}&debug=true"\
            .format(from_sa="E", to_sa="H", datetime="20120615T080000")

        response = self.query_region(query, display=False)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1

    def test_sp_to_sp(self):
        """
        Test journeys from stop point to stop point without street network
        """
        query = "journeys?from=stop_point:uselessA&to=stop_point:B&datetime=20120615T080000"

        # with street network desactivated
        response = self.query_region(query + "&max_duration_to_pt=0")
        assert('journeys' not in response)

        # with street network activated
        query = query + "&max_duration_to_pt=1"
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1
        assert response['journeys'][0]['sections'][0]['from']['id'] == 'stop_point:uselessA'
        assert response['journeys'][0]['sections'][0]['to']['id'] == 'A'
        assert response['journeys'][0]['sections'][0]['type'] == 'crow_fly'
        assert response['journeys'][0]['sections'][0]['mode'] == 'walking'
        assert response['journeys'][0]['sections'][0]['duration'] == 0

    def test_isochrone(self):
        response = self.query_region("journeys?from=I1&datetime=20120615T070000&max_duration=36000")
        assert(len(response['journeys']) == 2)

    def test_odt_admin_to_admin(self):
        """
        Test journeys from admin to admin using odt
        """
        query = "journeys?from=admin:93700&to=admin:75000&datetime=20120615T145500"

        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1
        assert response['journeys'][0]['sections'][0]['from']['id'] == 'admin:93700'
        assert response['journeys'][0]['sections'][0]['to']['id'] == 'admin:75000'
        assert response['journeys'][0]['sections'][0]['type'] == 'public_transport'
        assert response['journeys'][0]['sections'][0]['additional_informations'][0] == 'odt_with_zone'

    def test_heat_map_without_sn(self):
        """
        heat map call on a dataset with no streetnetwork
        we should get a nice error
        """
        r, status = self.query_region('heat_maps?from=stop_point:B&datetime=20120615T080000&max_duration=60',
                              check=False)

        assert r['error']['id'] == "no_solution"
        assert r['error']['message'] == "no street network data, impossible to compute a heat_map"


@dataset({"main_routing_test": {},
          "basic_routing_test": {'check_killed': False}})
class OneDeadRegion():
    """
    Test if we still responds when one kraken is dead
    """

    def test_one_dead_region(self):
        self.krakens_pool["basic_routing_test"].kill()

        query = "v1/journeys?from=stop_point:stopA&to=stop_point:stopB&datetime=20120614T080000&debug=true&max_duration_to_pt=0"
        response = self.query(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        assert len(response['journeys']) == 1
        assert len(response['journeys'][0]['sections']) == 1
        assert response['journeys'][0]['sections'][0]['type'] == 'public_transport'
        assert len(response['debug']['regions_called']) == 1
        assert response['debug']['regions_called'][0] == "main_routing_test"


@dataset({"main_routing_without_pt_test": {'priority': 42}, "main_routing_test": {'priority': 10}})
class WithoutPt():
    def test_one_region_without_pt(self):
        """
        Test if we still responds when one kraken has no pt solution
        """
        query = "v1/"+journey_basic_query+"&debug=true"
        response = self.query(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        assert len(response['journeys']) == 2
        assert len(response['journeys'][0]['sections']) == 3
        assert response['journeys'][0]['sections'][1]['type'] == 'public_transport'
        assert len(response['debug']['regions_called']) == 2
        assert response['debug']['regions_called'][0] == "main_routing_without_pt_test"
        assert response['debug']['regions_called'][1] == "main_routing_test"

    def test_one_region_without_pt_new_default(self):
        """
        Test if we still responds when one kraken has no pt solution using new_default
        """
        query = "v1/"+journey_basic_query+"&debug=true"
        response = self.query(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        assert len(response['journeys']) == 2
        assert len(response['journeys'][0]['sections']) == 3
        assert response['journeys'][0]['sections'][1]['type'] == 'public_transport'
        assert len(response['debug']['regions_called']) == 2
        assert response['debug']['regions_called'][0] == "main_routing_without_pt_test"
        assert response['debug']['regions_called'][1] == "main_routing_test"


@dataset({"main_ptref_test": {}})
class JourneysWithPtref():
    """
    Test all scenario with ptref_test data
    """
    def test_strange_line_name(self):
        query = "journeys?from=stop_area:stop2&to=stop_area:stop1&datetime=20140107T100000"
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        assert len(response['journeys']) == 1


@dataset({"main_routing_test": {}})
class JourneyMinBikeMinCar(object):
    def test_first_section_mode_and_last_section_mode_bike(self):
        query = '{sub_query}&last_section_mode[]=bike&first_section_mode[]=bike&' \
                'datetime={datetime}'.format(sub_query=sub_query, datetime="20120614T080000")
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1
        assert len(response['journeys'][0]['sections']) == 1
        assert response['journeys'][0]['sections'][0]['mode'] == 'bike'
        assert response['journeys'][0]['sections'][0]['duration'] == 62

    def test_first_section_mode_bike_and_last_section_mode_walking(self):
        query = '{sub_query}&last_section_mode[]=bike&first_section_mode[]=walking&' \
                'datetime={datetime}'.format(sub_query=sub_query, datetime="20120614T080000")
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 2
        assert len(response['journeys'][0]['sections']) == 3
        assert response['journeys'][0]['sections'][0]['mode'] == 'walking'
        assert response['journeys'][0]['sections'][0]['duration'] == 17
        assert response['journeys'][0]['sections'][-1]['mode'] == 'bike'
        assert response['journeys'][0]['sections'][-1]['duration'] == 21

        assert len(response['journeys'][1]['sections']) == 1
        assert response['journeys'][1]['sections'][0]['mode'] == 'walking'
        assert response['journeys'][1]['sections'][0]['duration'] == 276

    def test_last_section_mode_bike_only(self):
        query = '{sub_query}&last_section_mode[]=bike&datetime={datetime}'.format(sub_query=sub_query,
                                                                                  datetime="20120614T080000")
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 2
        assert len(response['journeys'][0]['sections']) == 3
        assert response['journeys'][0]['sections'][0]['mode'] == 'walking'
        assert response['journeys'][0]['sections'][0]['duration'] == 17
        assert response['journeys'][0]['sections'][-1]['mode'] == 'bike'
        assert response['journeys'][0]['sections'][-1]['duration'] == 21

        assert len(response['journeys'][1]['sections']) == 1
        assert response['journeys'][1]['sections'][0]['mode'] == 'walking'
        assert response['journeys'][1]['sections'][0]['duration'] == 276



    def test_first_section_mode_walking_and_last_section_mode_bike(self):
        query = '{sub_query}&last_section_mode[]=walking&first_section_mode[]=bike&' \
                'datetime={datetime}'.format(sub_query=sub_query, datetime="20120614T080000")
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1
        assert len(response['journeys'][0]['sections']) == 1
        assert response['journeys'][0]['sections'][0]['mode'] == 'bike'
        assert response['journeys'][0]['sections'][0]['duration'] == 62

    def test_first_section_mode_bike_only(self):
        query = '{sub_query}&first_section_mode[]=bike&datetime={datetime}'.format(sub_query=sub_query,
                                                                                   datetime="20120614T080000")
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1
        assert len(response['journeys'][0]['sections']) == 1
        assert response['journeys'][0]['sections'][0]['mode'] == 'bike'
        assert response['journeys'][0]['sections'][0]['duration'] == 62

    def test_first_section_mode_bike_walking_only(self):
        query = '{sub_query}&first_section_mode[]=walking&first_section_mode[]=bike&' \
                'datetime={datetime}'.format(sub_query=sub_query, datetime="20120614T080000")
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 2
        assert len(response['journeys'][0]['sections']) == 3
        assert response['journeys'][0]['sections'][0]['mode'] == 'walking'
        assert response['journeys'][0]['sections'][0]['duration'] == 17

        assert response['journeys'][0]['sections'][-1]['mode'] == 'walking'
        assert response['journeys'][0]['sections'][-1]['duration'] == 80

        assert len(response['journeys'][-1]['sections']) == 1
        assert response['journeys'][-1]['sections'][0]['mode'] == 'walking'
        assert response['journeys'][-1]['sections'][0]['duration'] == 276

    def test_first_section_mode_and_last_section_mode_car(self):
        query = '{sub_query}&last_section_mode[]=car&first_section_mode[]=car&' \
                'datetime={datetime}'.format(sub_query=sub_query, datetime="20120614T070000")
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1
        assert len(response['journeys'][0]['sections']) == 3
        assert response['journeys'][0]['sections'][0]['mode'] == 'car'
        assert response['journeys'][0]['sections'][0]['duration'] == 16

        assert response['journeys'][0]['sections'][-1]['mode'] == 'walking'
        assert response['journeys'][0]['sections'][-1]['duration'] == 106



    def test_activate_min_car_bike(self):
        modes = [
            'last_section_mode[]=car&first_section_mode[]=car&last_section_mode[]=walking&first_section_mode[]=walking',
            'last_section_mode[]=bike&first_section_mode[]=bike&last_section_mode[]=walking&first_section_mode[]=walking',
            'last_section_mode[]=bike&first_section_mode[]=car&last_section_mode[]=walking&first_section_mode[]=walking',
            'last_section_mode[]=car&first_section_mode[]=bike&last_section_mode[]=walking&first_section_mode[]=walking',
            'last_section_mode[]=car&last_section_mode[]=walking',
            'first_section_mode[]=car&first_section_mode[]=walking',
        ]
        for mode in modes:
            query = '{sub_query}&{mode}&datetime={datetime}'.format(sub_query=sub_query,
                                                                    mode=mode, datetime="20120614T080000")
            response = self.query_region(query)
            self.is_valid_journey_response(response, query)
            assert len(response['journeys']) == 2
            assert len(response['journeys'][0]['sections']) == 3
            assert response['journeys'][0]['sections'][0]['mode'] == 'walking'
            assert response['journeys'][0]['sections'][0]['duration'] == 17
            assert response['journeys'][0]['sections'][1]['type'] == 'public_transport'
            assert response['journeys'][0]['sections'][1]['duration'] == 2
            assert response['journeys'][0]['sections'][-1]['mode'] == 'walking'
            assert response['journeys'][0]['sections'][-1]['duration'] == 80

            assert len(response['journeys'][1]['sections']) == 1
            assert response['journeys'][1]['sections'][0]['mode'] == 'walking'
            assert response['journeys'][1]['sections'][0]['duration'] == 276
