# -*- coding: utf-8 -*-
# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
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

import pytest
from six.moves.urllib.parse import quote
from .tests_mechanism import dataset
from jormungandr import i_manager
from .check_utils import *
import mock
from pytest import approx
from navitiacommon.parser_args_type import SpeedRange


def check_best(resp):
    assert not resp.get('journeys') or sum((1 for j in resp['journeys'] if j['type'] == "best")) == 1


def get_directpath_count_by_mode(resp, mode):
    directpath_count = 0
    for journey in resp["journeys"]:
        if len(journey['sections']) == 1 and (mode in journey['tags'] and 'non_pt' in journey['tags']):
            directpath_count += 1
    return directpath_count


def find_with_tag(journeys, tag):
    return next((j for j in journeys if tag in j['tags'] and 'non_pt' in j['tags']), None)


@dataset({"main_routing_test": {}})
class JourneyCommon(object):

    """
    Test the structure of the journeys response
    """

    def test_journeys(self):
        # NOTE: we query /v1/coverage/main_routing_test/journeys and not directly /v1/journeys
        # not to use the jormungandr database
        response = self.query_region(journey_basic_query, display=True)
        check_best(response)
        self.is_valid_journey_response(response, journey_basic_query)

        feed_publishers = get_not_null(response, "feed_publishers")
        assert len(feed_publishers) == 1
        feed_publisher = feed_publishers[0]
        assert feed_publisher["id"] == "builder"
        assert feed_publisher["name"] == 'routing api data'
        assert feed_publisher["license"] == "ODBL"
        assert feed_publisher["url"] == "www.canaltp.fr"

        self.check_context(response)

    def test_address_in_stop_points_on_journeys(self):
        response = self.query_region(journey_basic_query)
        journeys = [journey for journey in response['journeys']]
        assert len(journeys) == 2
        sections = journeys[0]["sections"] + journeys[1]["sections"]
        ft_func = lambda section_list, ft: [section[ft] for section in sections if "stop_point" in section[ft]]
        from_to = ft_func(sections, "from") + ft_func(sections, "to")
        assert len(from_to) == 4
        for s in from_to:
            address = get_not_null(s['stop_point'], 'address')
            is_valid_address(address, depth_check=0)

    def test_error_on_journeys_out_of_bounds(self):
        """ if we got an error with kraken, an error should be returned"""

        query_out_of_production_bound = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}".format(
            from_coord="0.0000898312;0.0000898312",  # coordinate of S in the dataset
            to_coord="0.00188646;0.00071865",  # coordinate of R in the dataset
            datetime="20110614T080000",
        )  # 2011 should not be in the production period

        response, status = self.query_region(query_out_of_production_bound, check=False)

        assert status != 200, "the response should not be valid"
        check_best(response)
        assert response['error']['id'] == "date_out_of_bounds"
        assert response['error']['message'] == "date is not in data production period"

        # and no journey is to be provided
        assert 'journeys' not in response or len(response['journeys']) == 0

    def test_error_on_journeys_too_early(self):
        """ datetime is > 1970y """

        query_out_of_production_bound = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}".format(
            from_coord="0.0000898312;0.0000898312", to_coord="0.00188646;0.00071865", datetime="19700101T000000"
        )

        response, status = self.query_region(query_out_of_production_bound, check=False)

        assert status != 200, "the response should not be valid"
        check_best(response)
        assert (
            response['message']
            == 'parameter \"datetime\" invalid: Unable to parse datetime, date is too early!\ndatetime description: Date and time to go/arrive (see `datetime_represents`).\nNote: the datetime must be in the coverage’s publication period.'
        )

        # and no journey is to be provided
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
        non_pt_walk_j = next((j for j in journeys if "non_pt_walking" in j['tags']), None)
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

        response = self.query_region("{query}&type=car".format(query=journey_basic_query))

        assert not 'journeys' in response or len(response['journeys']) == 0
        assert 'error' in response
        assert response['error']['id'] == 'no_solution'
        assert response['error']['message'] == 'No journey found, all were filtered'

    def test_dumb_filtering(self):
        """if we filter with non existent type, we get an error"""

        response, status = self.query_region(
            "{query}&type=sponge_bob".format(query=journey_basic_query), check=False
        )

        assert status == 400, "the response should not be valid"

        m = 'parameter "type" invalid: The type argument must be in list'
        assert m in response['message']

    def test_journeys_no_bss_and_walking(self):
        query = journey_basic_query + "&first_section_mode=walking&first_section_mode=bss"
        response = self.query_region(query)

        check_best(response)
        self.is_valid_journey_response(response, query)
        # Note: we need to mock the kraken instances to check that only one call has been made and not 2
        # (only one for bss because walking should not have been added since it duplicate bss)

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
        query = (
            "journeys?from={from_coord}&to={to_coord}&datetime={datetime}&"
            "min_nb_journeys=3&_night_bus_filter_base_factor=86400&"
            "datetime_represents=arrival".format(
                from_coord=s_coord, to_coord=r_coord, datetime="20120614T185500"
            )
        )
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response["journeys"]) >= 3

    def test_walking_transfer_penalty_parameter(self):
        """
        Test use of newly added parameter _walking_transfer_penalty
        Default value is 120
        """
        query = (
            "journeys?from={from_coord}&to={to_coord}&datetime={datetime}&"
            "min_nb_transfers=2&debug=true".format(
                from_coord='0.00148221477022527;0.0007186495855637672',
                to_coord='0.0002694935945864127;8.98311981954709e-05',
                datetime="20120614080000",
            )
        )
        response = self.query_region(query + '&_walking_transfer_penalty=0')
        assert len(response["journeys"]) == 3

        response = self.query_region(query + '&_walking_transfer_penalty=120')
        assert len(response["journeys"]) == 3

        response = self.query_region(query)
        assert len(response["journeys"]) == 3

        response = self.query_region(query + '&_walking_transfer_penalty=480')
        assert len(response["journeys"]) == 2

    """
    test on date format
    """

    def test_journeys_no_date(self):
        """
        giving no date, we should have a response
        BUT, since without date we take the current date, it will not be in the production period,
        so we have a 'not un production period error'
        """

        query = "journeys?from={from_coord}&to={to_coord}".format(from_coord=s_coord, to_coord=r_coord)

        response, status_code = self.query_no_assert("v1/coverage/main_routing_test/" + query)

        assert status_code != 200, "the response should not be valid"

        assert response['error']['id'] == "date_out_of_bounds"
        assert response['error']['message'] == "date is not in data production period"

    def test_journeys_date_no_second(self):
        """giving no second in the date we should not be a problem"""

        query = "journeys?from={from_coord}&to={to_coord}&datetime={d}".format(
            from_coord=s_coord, to_coord=r_coord, d="20120614T0800"
        )

        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, journey_basic_query)

        # and the second should be 0 initialized
        journeys = get_not_null(response, "journeys")
        assert journeys[0]["requested_date_time"] == "20120614T080000"

    def test_journeys_date_no_minute_no_second(self):
        """giving no minutes and no second in the date we should not be a problem"""

        query = "journeys?from={from_coord}&to={to_coord}&datetime={d}".format(
            from_coord=s_coord, to_coord=r_coord, d="20120614T08"
        )

        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, journey_basic_query)

        # and the second should be 0 initialized
        journeys = get_not_null(response, "journeys")
        assert journeys[0]["requested_date_time"] == "20120614T080000"

    def test_journeys_date_too_long(self):
        """giving an invalid date (too long) should be a problem"""

        query = "journeys?from={from_coord}&to={to_coord}&datetime={d}".format(
            from_coord=s_coord, to_coord=r_coord, d="20120614T0812345"
        )

        response, status_code = self.query_no_assert("v1/coverage/main_routing_test/" + query)

        assert not 'journeys' in response
        assert 'message' in response
        assert "unable to parse datetime, unknown string format" in response['message'].lower()

    def test_journeys_date_invalid(self):
        """giving the date with mmsshh (56 45 12) should be a problem"""

        query = "journeys?from={from_coord}&to={to_coord}&datetime={d}".format(
            from_coord=s_coord, to_coord=r_coord, d="20120614T564512"
        )

        response, status_code = self.query_no_assert("v1/coverage/main_routing_test/" + query)

        assert not 'journeys' in response
        assert 'message' in response
        assert "Unable to parse datetime, hour must be in 0..23" in response['message']

    def test_journeys_date_valid_invalid(self):
        """some format of date are bizarrely interpreted, and can result in date in 800"""

        query = "journeys?from={from_coord}&to={to_coord}&datetime={d}".format(
            from_coord=s_coord, to_coord=r_coord, d="T0800"
        )

        response, status_code = self.query_no_assert("v1/coverage/main_routing_test/" + query)

        assert not 'journeys' in response
        assert 'message' in response
        assert "Unable to parse datetime, date is too early!" in response['message']

    def test_journeys_bad_speed(self):
        """speed not in range"""

        speed_range = SpeedRange.map_range
        speed_test = {}
        for sn in ["walking", "bike", "bss", "car", "car_no_park", "taxi", "ridesharing"]:
            sp_range = speed_range["{sn}_speed".format(sn=sn)]
            speed_test[sn] = [sp_range[0] - 1, sp_range[1] + 1]

        for sn in ["walking", "bike", "bss", "car", "car_no_park", "taxi", "ridesharing"]:
            (speed_min, speed_max) = speed_range["{sn}_speed".format(sn=sn)]
            for speed in speed_test[sn]:
                query = "journeys?from={from_coord}&to={to_coord}&datetime={d}&{sn}_speed={speed}".format(
                    from_coord=s_coord, to_coord=r_coord, d="20120614T133700", sn=sn, speed=speed
                )
                response, status_code = self.query_no_assert("v1/coverage/main_routing_test/" + query)

                assert not 'journeys' in response
                assert 'message' in response
                assert (
                    "The {sn}_speed argument has to be in range [{speed_min}, {speed_max}], you gave : {speed}".format(
                        sn=sn, speed=speed, speed_min=speed_min, speed_max=speed_max
                    )
                    in response['message']
                )

    def test_journeys_date_valid_not_zeropadded(self):
        """giving the date with non zero padded month should be a problem"""

        query = "journeys?from={from_coord}&to={to_coord}&datetime={d}".format(
            from_coord=s_coord, to_coord=r_coord, d="2012614T081025"
        )

        response, status_code = self.query_no_assert("v1/coverage/main_routing_test/" + query)

        assert not 'journeys' in response
        assert 'message' in response
        assert "Unable to parse datetime, year" in response['message']

    def _check_id_precision(self, coord_from, coord_to):
        query = "journeys?from={coord_from}&to={coord_to}&datetime={d}".format(
            coord_from=coord_from, coord_to=coord_to, d="20120614T080000"
        )
        response = self.query_region(query)
        # no best as non_pt_walk
        self.is_valid_journey_response(response, query)

        assert len(response['journeys']) > 0
        for j in response['journeys']:
            assert j['sections'][0]['from']['id'] == coord_from
            assert j['sections'][0]['from']['address']['id'] == coord_from
            assert j['sections'][0]['to']['id'] == coord_to
            assert j['sections'][0]['to']['address']['id'] == coord_to

    def test_journeys_do_not_loose_precision(self):
        """do we have a good precision given back in the id"""

        # this id was generated by giving an id to the api, and
        # copying it here the resulting id
        id_with_precision = "8.98311981954709e-05;0.000898311281954"
        id_without_precision = "0.00188646;0.00071865"

        self._check_id_precision(id_with_precision, id_without_precision)
        self._check_id_precision(id_without_precision, id_with_precision)

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

        assert len(response['journeys']) == 2

        def compare(l1, l2):
            assert sorted(l1) == sorted(l2)

        # Note: we do not test order, because that can change depending on the scenario
        compare(get_used_vj(response), [[], ['vjB']])
        compare(get_arrivals(response), ['20120614T080613', '20120614T180250'])

        # same response if we just give the wheelchair=True
        query = journey_basic_query + "&traveler_type=wheelchair&wheelchair=True"
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        assert len(response['journeys']) == 2
        compare(get_used_vj(response), [[], ['vjB']])
        compare(get_arrivals(response), ['20120614T080613', '20120614T180250'])

        # but with the wheelchair profile, if we explicitly accept non accessible solutions (not very
        # consistent, but anyway), we should take the non accessible bus that arrive at 08h
        query = journey_basic_query + "&traveler_type=wheelchair&wheelchair=False"
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        assert len(response['journeys']) == 2
        compare(get_used_vj(response), [['vjA'], []])
        compare(get_arrivals(response), ['20120614T080250', '20120614T080613'])

    def test_journeys_float_night_bus_filter_max_factor(self):
        """night_bus_filter_max_factor can be a float (and can be null)"""

        query = (
            "journeys?from={from_coord}&to={to_coord}&datetime={d}&"
            "_night_bus_filter_max_factor={_night_bus_filter_max_factor}".format(
                from_coord=s_coord, to_coord=r_coord, d="20120614T080000", _night_bus_filter_max_factor=2.8
            )
        )

        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        query = (
            "journeys?from={from_coord}&to={to_coord}&datetime={d}&"
            "_night_bus_filter_max_factor={_night_bus_filter_max_factor}".format(
                from_coord=s_coord, to_coord=r_coord, d="20120614T080000", _night_bus_filter_max_factor=0
            )
        )

        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

    def test_sp_to_sp(self):
        """
        Test journeys from stop point to stop point
        """

        query = "journeys?from=stop_point:uselessA&to=stop_point:stopB&datetime=20120615T080000"

        if self.data_sets.get('main_routing_test', {}).get('scenario') == 'distributed':
            # In distributed scenario, we must deactivate direct path so that we can reuse the same
            # test code for all scenarios
            query += "&max_walking_direct_path_duration=0"

        # with street network desactivated
        response = self.query_region(query + "&max_duration_to_pt=0")
        assert 'journeys' not in response

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

        if self.data_sets.get('main_routing_test', {}).get('scenario') == 'distributed':
            # In distributed scenario, we must deactivate direct path so that we can reuse the same
            # test code for all scenarios
            query += "&max_walking_direct_path_duration=0"

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
            return any(
                sect_fast_walker["mode"] == 'bike'
                for sect_fast_walker in fast_walker['sections']
                if 'mode' in sect_fast_walker
            )

        def no_bike_in_journey(journey):
            return all(section['mode'] != 'bike' for section in journey['sections'] if 'mode' in section)

        assert any(
            bike_in_journey(journey_fast_walker) for journey_fast_walker in response_fast_walker['journeys']
        )
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

    def test_some_pt_section_objects_without_parameter_depth_or_with_depth_0_and_1(self):
        """
        Test that stop_area as well it's codes are displayed in
         - section.from
         - section.to
         Test that stop_area is not displayed in section.stop_date_times[].stop_point
        """

        def check_result(resp):
            assert len(resp['journeys']) == 2
            assert len(resp['journeys'][0]['sections']) == 3
            assert resp['journeys'][0]['sections'][1]['type'] == 'public_transport'
            assert len(resp['journeys'][0]['sections'][1]['stop_date_times']) == 2

            from_sp = resp['journeys'][0]['sections'][1]['from']['stop_point']
            assert 'stop_area' in from_sp
            assert from_sp['stop_area']['codes'][0]['type'] == 'UIC8'
            assert from_sp['stop_area']['codes'][0]['value'] == '80142281'
            to_sp = resp['journeys'][0]['sections'][1]['to']['stop_point']
            assert 'stop_area' in to_sp
            assert to_sp['stop_area']['codes'][1]['type'] == 'UIC8'
            assert to_sp['stop_area']['codes'][1]['value'] == '80110684'
            assert 'stop_area' not in resp['journeys'][0]['sections'][1]['stop_date_times'][0]['stop_point']
            assert 'stop_area' not in resp['journeys'][0]['sections'][1]['stop_date_times'][1]['stop_point']

        # Request without parameter depth:
        journey_query = journey_basic_query
        response = self.query_region(journey_query, display=False)
        check_best(response)
        self.is_valid_journey_response(response, journey_query)
        check_result(response)

        # Request with parameter depth=0
        journey_query = journey_basic_query + '&depth=0'
        response = self.query_region(journey_query, display=False)
        check_best(response)
        self.is_valid_journey_response(response, journey_query)
        check_result(response)

        # Request with parameter depth=1 (equivalent to request without parameter)
        journey_query = journey_basic_query + '&depth=1'
        response = self.query_region(journey_query, display=False)
        check_best(response)
        self.is_valid_journey_response(response, journey_query)
        check_result(response)

    def test_some_pt_section_objects_with_parameter_depth_2(self):
        """
        Test that stop_area as well it's codes are displayed in
         - section.from
         - section.to
         - section.stop_date_times[].stop_point
        """

        journey_query = journey_basic_query + '&depth=2'
        response = self.query_region(journey_query, display=False)
        check_best(response)
        self.is_valid_journey_response(response, journey_query)
        assert len(response['journeys']) == 2
        assert len(response['journeys'][0]['sections']) == 3
        assert response['journeys'][0]['sections'][1]['type'] == 'public_transport'
        assert len(response['journeys'][0]['sections'][1]['stop_date_times']) == 2

        from_sp = response['journeys'][0]['sections'][1]['from']['stop_point']
        assert 'stop_area' in from_sp
        assert from_sp['stop_area']['codes'][0]['type'] == 'UIC8'
        assert from_sp['stop_area']['codes'][0]['value'] == '80142281'
        to_sp = response['journeys'][0]['sections'][1]['to']['stop_point']
        assert 'stop_area' in to_sp
        assert to_sp['stop_area']['codes'][1]['type'] == 'UIC8'
        assert to_sp['stop_area']['codes'][1]['value'] == '80110684'

        sp1 = response['journeys'][0]['sections'][1]['stop_date_times'][0]['stop_point']
        assert 'stop_area' in sp1
        assert sp1['stop_area']['codes'][0]['type'] == 'UIC8'
        assert sp1['stop_area']['codes'][0]['value'] == '80142281'
        sp2 = response['journeys'][0]['sections'][1]['stop_date_times'][1]['stop_point']
        assert 'stop_area' in sp2
        assert sp2['stop_area']['codes'][1]['type'] == 'UIC8'
        assert sp2['stop_area']['codes'][1]['value'] == '80110684'

    def test_max_duration_to_pt_equals_to_0(self):
        query = (
            journey_basic_query
            + "&first_section_mode[]=bss"
            + "&first_section_mode[]=walking"
            + "&first_section_mode[]=bike"
            + "&first_section_mode[]=car"
            + "&debug=true"
        )
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 4

        bike_journey = find_with_tag(response['journeys'], 'bike')
        assert bike_journey
        assert bike_journey['durations']['bike'] == 62
        assert bike_journey['durations']['total'] == 62
        assert bike_journey['distances']['bike'] == 257
        assert bike_journey['durations']['walking'] == 0

        car_journey = find_with_tag(response['journeys'], 'car')
        # TODO: fix this in a new PR
        if 'Distributed' in self.__class__.__name__:
            assert car_journey['durations']['total'] == 422
        else:
            assert car_journey['durations']['total'] == 123

        query += "&max_duration_to_pt=0"
        response, status = self.query_no_assert(query)
        # pas de solution
        assert status == 404
        assert 'journeys' not in response

    def test_max_duration_to_pt_equals_to_0_from_stop_point(self):
        query = "journeys?from=stop_point%3AstopA&to=stop_point%3AstopC&datetime=20120614T080000"
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1

        query += "&max_duration_to_pt=0"
        if self.data_sets.get('main_routing_test', {}).get('scenario') == 'distributed':
            # In distributed scenario, we must deactivate direct path so that we can reuse the same
            # test code for all scenarios
            query += "&max_walking_direct_path_duration=0"

        # There is no direct_path but a journey using Metro
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
        assert len(response['journeys']) == 1

        query += "&max_duration_to_pt=0"
        if self.data_sets.get('main_routing_test', {}).get('scenario') == 'distributed':
            # In distributed scenario, we must deactivate direct path so that we can reuse the same
            # test code for all scenarios
            query += "&max_walking_direct_path_duration=0"

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

        # verify distances and durations in a journey with crow_fly
        assert jrnys[0]['durations']['walking'] == 0
        assert jrnys[0]['distances']['walking'] == 0

    def test_max_duration_equals_to_0(self):
        query = (
            journey_basic_query
            + "&first_section_mode[]=bss"
            + "&first_section_mode[]=walking"
            + "&first_section_mode[]=bike"
            + "&first_section_mode[]=car"
            + "&debug=true"
        )
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

        bike_journey = find_with_tag(response['journeys'], 'bike')
        assert bike_journey
        assert bike_journey['debug']['internal_id']
        assert len(bike_journey['sections']) == 1

        car_journey = find_with_tag(response['journeys'], 'car')
        assert car_journey
        assert car_journey['debug']['internal_id']

        assert len(car_journey['sections']) == 3

        walking_journey = find_with_tag(response['journeys'], 'walking')
        assert walking_journey
        assert walking_journey['debug']['internal_id']
        assert len(walking_journey['sections']) == 1

    def test_journey_stop_area_to_stop_point(self):
        """
        When the departure is stop_area:A and the destination is stop_point:B belonging to stop_area:B
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}".format(
            from_sa='stopA', to_sa='stop_point:stopB', datetime="20120614T080000"
        )
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        jrnys = response['journeys']
        j = jrnys[0]
        assert j
        assert j['sections'][0]['from']['id'] == 'stopA'
        assert j['sections'][0]['to']['id'] == 'stop_point:stopB'
        assert 'walking' in j['tags']

    def test_journey_from_non_valid_stop_area(self):
        """
        When the departure is a non valid stop_area, the response status should be 404
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}".format(
            from_sa='stop_area:non_valid', to_sa='stop_point:stopB', datetime="20120614T080000"
        )
        response = self.query_region(query, check=False)
        assert response[1] == 404
        assert response[0]['error']['message'] == u'The entry point: stop_area:non_valid is not valid'
        assert response[0]['error']['id'] == u'unknown_object'

    def test_crow_fly_sections(self):
        """
        When the departure is a stop_area...
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}&walking_speed=0.1".format(
            from_sa='stopA', to_sa='stopB', datetime="20120614T080000"
        )
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        jrnys = response['journeys']
        assert len(jrnys) >= 1
        section_0 = jrnys[0]['sections'][0]
        assert section_0['type'] == 'crow_fly'
        assert section_0['mode'] == 'walking'
        assert section_0['duration'] == 0
        assert section_0['from']['id'] == 'stopA'
        assert section_0['to']['id'] == 'stop_point:stopA'
        assert section_0['geojson']
        assert section_0['geojson']['type'] == 'LineString'
        assert section_0['geojson']['coordinates'][0] == [0.0010779744, 0.0007186496]
        assert section_0['geojson']['coordinates'][1] == [0.0010779744, 0.0007186496]

        section_2 = jrnys[0]['sections'][2]
        assert section_2['type'] == 'crow_fly'
        assert section_2['mode'] == 'walking'
        assert section_2['duration'] == 0
        assert section_2['from']['id'] == 'stop_point:stopB'
        assert section_2['to']['id'] == 'stopB'
        assert section_2['geojson']
        assert section_2['geojson']['type'] == 'LineString'
        assert section_2['geojson']['coordinates'][0] == [8.98312e-05, 0.0002694936]
        assert section_2['geojson']['coordinates'][1] == [8.98312e-05, 0.0002694936]

    def test_no_origin_point(self):
        """
        Here we verify no destination point error
        """
        query_out_of_production_bound = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}".format(
            from_coord="0.9008898312;0.0019898312",  # coordinate out of range in the dataset
            to_coord="0.00188646;0.00071865",  # coordinate of R in the dataset
            datetime="20120614T080000",
        )

        response, status = self.query_region(query_out_of_production_bound, check=False)

        assert status != 200, "the response should not be valid"
        check_best(response)
        assert response['error']['id'] == "no_origin"
        assert response['error']['message'] == "Public transport is not reachable from origin"

        # and no journey is to be provided
        assert 'journeys' not in response or len(response['journeys']) == 0

    def test_no_destination_point(self):
        """
        Here we verify no destination point error
        """
        query_out_of_production_bound = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}".format(
            from_coord="0.0000898312;0.0000898312",  # coordinate of S in the dataset
            to_coord="0.00188646;0.90971865",  # coordinate out of range in the dataset
            datetime="20120614T080000",
        )

        response, status = self.query_region(query_out_of_production_bound, check=False)

        assert status != 200, "the response should not be valid"
        check_best(response)
        assert response['error']['id'] == "no_destination"
        assert response['error']['message'] == "Public transport is not reachable from destination"

        # and no journey is to be provided
        assert 'journeys' not in response or len(response['journeys']) == 0

    def test_no_origin_nor_destination(self):
        """
        Here we verify no origin nor destination point error
        """
        query_out_of_production_bound = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}".format(
            from_coord="0.9008898312;0.0019898312",  # coordinate out of range in the dataset
            to_coord="0.00188646;0.90971865",  # coordinate out of range in the dataset
            datetime="20120614T080000",
        )

        response, status = self.query_region(query_out_of_production_bound, check=False)

        assert status != 200, "the response should not be valid"
        check_best(response)
        assert response['error']['id'] == "no_origin_nor_destination"
        assert response['error']['message'] == "Public transport is not reachable from origin nor destination"

        # and no journey is to be provided
        assert 'journeys' not in response or len(response['journeys']) == 0

    def test_no_solution(self):
        """
        Here we verify no destination point error
        """
        query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}".format(
            from_coord="0.0000898312;0.0000898312",  # coordinate out of range in the dataset
            to_coord="0.00188646;0.00071865",  # coordinate out of range in the dataset
            datetime="20120901T220000",
        )
        if self.data_sets.get('main_routing_test', {}).get('scenario') == 'distributed':
            # In distributed scenario, we must deactivate direct path so that we can reuse the same
            # test code for all scenarios
            query += "&max_walking_direct_path_duration=0"

        response, status = self.query_region(query + "&max_duration=1&max_duration_to_pt=100", check=False)

        assert status == 200
        check_best(response)
        assert response['error']['id'] == "no_solution"
        assert response['error']['message'] == "no solution found for this journey"

        # and no journey is to be provided
        assert 'journeys' not in response or len(response['journeys']) == 0

    def test_call_kraken_foreach_mode(self):
        """
        test if the different pt computation do not interfer
        """
        query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}&first_section_mode[]=walking&first_section_mode[]=bike&debug=true".format(
            from_coord="0.0000898312;0.0000898312", to_coord="0.00188646;0.00071865", datetime="20120614T080000"
        )

        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 3
        assert len(response['journeys'][0]['sections']) == 1
        assert response['journeys'][0]['sections'][0]['mode'] == 'bike'
        assert response['journeys'][0]['durations']['total'] == 62
        assert response['journeys'][0]['durations']['bike'] == 62
        assert response['journeys'][0]['distances']['bike'] == 257
        assert len(response['journeys'][1]['sections']) == 3
        assert response['journeys'][1]['sections'][0]['mode'] == 'walking'
        assert response['journeys'][1]['durations']['walking'] == 97
        assert response['journeys'][1]['distances']['walking'] == 108
        assert response['journeys'][1]['durations']['total'] == 99
        assert len(response['journeys'][2]['sections']) == 1
        assert response['journeys'][2]['sections'][0]['mode'] == 'walking'
        assert response['journeys'][2]['durations']['total'] == 276
        assert response['journeys'][2]['durations']['walking'] == 276
        assert response['journeys'][2]['distances']['walking'] == 309

        query += '&bike_speed=3'
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 3
        assert len(response['journeys'][0]['sections']) == 1
        assert response['journeys'][0]['sections'][0]['mode'] == 'bike'
        assert response['journeys'][0]['durations']['total'] == 85
        assert response['journeys'][0]['durations']['bike'] == 85
        assert len(response['journeys'][1]['sections']) == 3
        assert response['journeys'][1]['sections'][0]['mode'] == 'walking'
        assert response['journeys'][1]['durations']['walking'] == 97
        assert response['journeys'][1]['durations']['total'] == 99
        assert len(response['journeys'][2]['sections']) == 1
        assert response['journeys'][2]['sections'][0]['mode'] == 'walking'
        assert response['journeys'][2]['durations']['total'] == 276
        assert response['journeys'][2]['durations']['walking'] == 276

    def test_call_kraken_boarding_alighting(self):
        """
        test that boarding and alighting sections are present
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}&debug=true&max_duration_to_pt=0".format(
            from_sa="stopA", to_sa="stopB", datetime="20120614T223000"
        )

        if self.data_sets.get('main_routing_test', {}).get('scenario') == 'distributed':
            # In distributed scenario, we must deactivate direct path so that we can reuse the same
            # test code for all scenarios
            query += "&max_walking_direct_path_duration=0"

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
        query = "vehicle_journeys/vehicle_journey:SNC/journeys?datetime={datetime}".format(
            datetime="20120614T080000"
        )
        response, status = self.query_region(query, check=False)
        assert status == 404
        assert response['error']['id'] == u'unknown_object'
        assert response['error']['message'] == u'The entry point: vehicle_journey:SNC is not valid'

    def test_free_radius_from(self):
        # The coordinates of departure and the stop point are separated by 20m
        # Query journeys with free_radius = 0
        r = self.query(
            '/v1/coverage/main_routing_test/journeys?from=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&to=stopA&datetime=20120614080000'
        )
        assert r['journeys'][0]['sections'][0]['type'] == 'street_network'
        assert r['journeys'][0]['sections'][0]['duration'] != 0
        # Verify distances and durations in a journey with street_network
        assert r['journeys'][0]['durations']['walking'] != 0
        assert r['journeys'][0]['distances']['walking'] != 0

        # Query journeys with free_radius = 19
        r = self.query(
            '/v1/coverage/main_routing_test/journeys?from=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&to=stopA&datetime=20120614080000&free_radius_from=19'
        )
        assert r['journeys'][0]['sections'][0]['type'] == 'street_network'
        assert r['journeys'][0]['sections'][0]['duration'] != 0

        # Query journeys with free_radius = 20
        r = self.query(
            '/v1/coverage/main_routing_test/journeys?from=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&to=stopA&datetime=20120614080000&free_radius_from=20'
        )
        assert r['journeys'][0]['sections'][0]['type'] == 'crow_fly'
        assert r['journeys'][0]['sections'][0]['duration'] == 0
        # Verify distances and durations in a journey with crow_fly
        assert r['journeys'][0]['distances']['walking'] == 0
        assert r['journeys'][0]['durations']['walking'] == 0

        # The time of departure of PT is 08:01:00 and it takes 17s to walk to the station
        # If the requested departure time is 08:00:50, the PT journey shouldn't be displayed
        r = self.query(
            '/v1/coverage/main_routing_test/journeys?from=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&to=stopA&datetime=20120614T080050&datetime_represents=departure&'
        )
        assert r['journeys'][0]['sections'][0]['type'] == 'street_network'

        # With the free_radius, the PT journey is displayed thanks to the 'free' crow_fly
        r = self.query(
            '/v1/coverage/main_routing_test/journeys?from=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&to=stopA&datetime=20120614T080100&free_radius_from=20&datetime_represents=departure&'
        )
        assert len(r['journeys'][0]['sections']) > 1
        assert r['journeys'][0]['sections'][0]['type'] == 'crow_fly'
        assert r['journeys'][0]['sections'][0]['duration'] == 0
        assert r['journeys'][0]['sections'][1]['type'] == 'public_transport'

    def test_free_radius_to(self):
        # The coordinates of arrival and the stop point are separated by 20m
        # Query journeys with free_radius = 0
        r = self.query(
            '/v1/coverage/main_routing_test/journeys?from=stopA&to=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&datetime=20120614080000&'
        )
        assert r['journeys'][0]['sections'][-1]['type'] == 'street_network'
        assert r['journeys'][0]['sections'][-1]['duration'] != 0
        # Verify distances and durations in a journey with street_network
        assert r['journeys'][0]['durations']['walking'] != 0
        assert r['journeys'][0]['distances']['walking'] != 0

        # Query journeys with free_radius = 19
        r = self.query(
            '/v1/coverage/main_routing_test/journeys?from=stopA&to=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&datetime=20120614080000&free_radius_from=19'
        )
        assert r['journeys'][0]['sections'][-1]['type'] == 'street_network'
        assert r['journeys'][0]['sections'][-1]['duration'] != 0

        # Query journeys with free_radius = 20
        r = self.query(
            '/v1/coverage/main_routing_test/journeys?from=stopA&to=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&datetime=20120614080000&free_radius_to=20&'
        )
        assert r['journeys'][0]['sections'][-1]['type'] == 'crow_fly'
        assert r['journeys'][0]['sections'][-1]['duration'] == 0
        # Verify distances and durations in a journey with crow_fly
        assert r['journeys'][0]['durations']['walking'] == 0
        assert r['journeys'][0]['distances']['walking'] == 0

        # The estimated time of arrival without free_radius is 08:01:19
        # If the requested arrival time is before 08:01:19, the PT journey shouldn't be displayed
        r = self.query(
            '/v1/coverage/main_routing_test/journeys?from=stopA&to=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&datetime=20120614T080118&free_radius_to=19&datetime_represents=arrival&'
        )
        assert r['journeys'][0]['sections'][0]['type'] == 'street_network'

        # With the free_radius, the PT journey is displayed thanks to the 'free' crow_fly
        r = self.query(
            '/v1/coverage/main_routing_test/journeys?from=stopA&to=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&datetime=20120614T080118&free_radius_to=20&_override_scenario=experimental&datetime_represents=arrival&'
        )
        assert len(r['journeys'][0]['sections']) > 1
        assert r['journeys'][0]['sections'][-1]['type'] == 'crow_fly'
        assert r['journeys'][0]['sections'][-1]['duration'] == 0
        assert r['journeys'][0]['sections'][-2]['type'] == 'public_transport'

    def test_free_radius_bss(self):
        r = self.query(
            '/v1/coverage/main_routing_test/journeys?'
            'from=stopA&to=coord%3A8.98311981954709e-05%3A8.98311981954709e-05&'
            'datetime=20120614080000&'
            'free_radius_from=20&'
            'free_radius_to=20&'
            'first_section_mode[]=bss&'
            'last_section_mode[]=bss'
        )
        assert r['journeys'][0]['sections'][0]['type'] == 'crow_fly'
        assert r['journeys'][0]['sections'][0]['duration'] == 0
        # Verify distances and durations in a journey with crow_fly
        assert r['journeys'][0]['distances']['walking'] == 0
        assert r['journeys'][0]['durations']['walking'] == 0

    def test_shared_section(self):
        # Query a journey from stopB to stopA
        r = self.query(
            '/v1/coverage/main_routing_test/journeys?to=stopA&from=stopB&datetime=20120614T080100&walking_speed=0.5'
        )
        assert r['journeys'][0]['type'] == 'best'
        assert r['journeys'][0]['sections'][1]['type'] == 'public_transport'
        # Here the heassign is modified by the headsign at stop.
        assert r['journeys'][0]['sections'][1]['display_informations']['trip_short_name'] == 'vjA'
        assert r['journeys'][0]['sections'][1]['display_informations']['headsign'] == 'A00'
        first_journey_pt = r['journeys'][0]['sections'][1]['display_informations']['name']

        # we can also verify the properties of the vehicle_journey in the section
        vj = self.query(
            '/v1/coverage/main_routing_test/vehicle_journeys/vehicle_journey:vjA?datetime=20120614T080100'
        )
        assert vj['vehicle_journeys'][0]['name'] == 'vjA'
        assert vj['vehicle_journeys'][0]['headsign'] == 'vjA_hs'

        # Query same journey schedules
        # A new journey vjM is available
        r = self.query(
            'v1/coverage/main_routing_test/journeys?_no_shared_section=False&allowed_id%5B%5D=stop_point%3AstopA&allowed_id%5B%5D=stop_point%3AstopB&first_section_mode%5B%5D=walking&last_section_mode%5B%5D=walking&is_journey_schedules=True&datetime=20120614T080100&to=stopA&min_nb_journeys=5&min_nb_transfers=0&direct_path=none&from=stopB&walking_speed=0.5'
        )
        assert r['journeys'][0]['sections'][1]['display_informations']['name'] == first_journey_pt
        assert r['journeys'][0]['sections'][1]['type'] == 'public_transport'
        assert len(r['journeys']) > 1
        next_journey_pt = r['journeys'][1]['sections'][1]['display_informations']['name']
        assert next_journey_pt != first_journey_pt
        # here we have the same headsign as well as trip_short_name
        assert r['journeys'][1]['sections'][1]['display_informations']['trip_short_name'] == 'vjM'
        assert r['journeys'][1]['sections'][1]['display_informations']['headsign'] == 'vjM'

        # Activate 'no_shared_section' parameter and query the same journey schedules
        # The parameter 'no_shared_section' shouldn't be taken into account
        r = self.query(
            'v1/coverage/main_routing_test/journeys?allowed_id%5B%5D=stop_point%3AstopA&allowed_id%5B%5D=stop_point%3AstopB&first_section_mode%5B%5D=walking&last_section_mode%5B%5D=walking&is_journey_schedules=True&datetime=20120614T080100&to=stopA&min_nb_journeys=5&min_nb_transfers=0&direct_path=none&from=stopB&_no_shared_section=True&walking_speed=0.5'
        )
        assert len(r['journeys']) == 4

        # Query the same journey schedules without 'is_journey_schedules' that deletes the parameter 'no_shared_section'
        # The journey vjM isn't available as it is a shared section
        r = self.query(
            'v1/coverage/main_routing_test/journeys?allowed_id%5B%5D=stop_point%3AstopA&allowed_id%5B%5D=stop_point%3AstopB&first_section_mode%5B%5D=walking&last_section_mode%5B%5D=walking&datetime=20120614T080100&to=stopA&min_nb_journeys=5&min_nb_transfers=0&direct_path=none&from=stopB&_no_shared_section=True&walking_speed=0.5'
        )
        assert r['journeys'][0]['sections'][1]['display_informations']['name'] == first_journey_pt
        assert len(r['journeys']) == 1

    def test_section_fare_zone(self):
        """
        In a 'stop_point', the section 'fare_zone' should be present if the info is available
        """
        r = self.query('/v1/coverage/main_routing_test/stop_points')
        # Only stop point 'stopA' has fare zone info
        assert r['stop_points'][0]['name'] == 'stop_point:stopA'
        assert r['stop_points'][0]['fare_zone']['name'] == "2"
        # Other stop points don't have the fare zone info
        assert not 'fare_zone' in r['stop_points'][1]

    def test_when_min_max_nb_journeys_equal_0(self):
        """
        max_nb_journeys should be greater than 0
        min_nb_journeys should be greater than 0 or equal to 0
        """
        for nb in (-42, 0):
            query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}&max_nb_journeys={max_nb_journeys}".format(
                from_sa="stopA", to_sa="stopB", datetime="20120614T223000", max_nb_journeys=nb
            )

            response = self.query_region(query, check=False)
            assert response[1] == 400
            assert (
                'parameter "max_nb_journeys" invalid: Unable to evaluate, invalid positive int\nmax_nb_journeys description: Maximum number of different suggested journeys, must be > 0'
                in response[0]['message']
            )

        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}&min_nb_journeys={min_nb_journeys}".format(
            from_sa="stopA", to_sa="stopB", datetime="20120614T223000", min_nb_journeys=int(-42)
        )

        response = self.query_region(query, check=False)
        assert response[1] == 400
        assert (
            'parameter "min_nb_journeys" invalid: Unable to evaluate, invalid unsigned int\nmin_nb_journeys description: Minimum number of different suggested journeys, must be >= 0'
            in response[0]['message']
        )

    def test_journeys_distances(self):
        query = "journeys?from=0.0001796623963909418;8.98311981954709e-05&to=0.0018864551621048887;0.0007186495855637672&datetime=20120614080000&"
        response = self.query_region(query)
        pt_journey = next((j for j in response['journeys'] if j['type'] != 'non_pt_walk'), None)
        assert pt_journey

        def get_length(s):
            return s['geojson']['properties'][0]['length']

        total_walking = get_length(pt_journey['sections'][0]) + get_length(pt_journey['sections'][2])

        distances = pt_journey['distances']
        assert distances['walking'] == total_walking
        assert distances['car'] == distances['bike'] == distances['ridesharing'] == 0

    def test_journeys_too_short_heavy_mode_fallback_filter(self):
        template = (
            'journeys?from=8.98311981954709e-05;8.98311981954709e-05'
            '&to=0.0018864551621048887;0.0007186495855637672'
            '&datetime=20120614080000'
            '&first_section_mode[]=car'
            '&first_section_mode[]=walking'
            '&car_speed=0.1'
            '&_min_car={min_car}'
            '&debug=true'
        )

        query = template.format(min_car=1)
        response = self.query_region(query)
        assert all('to_delete' not in j['tags'] for j in response['journeys'])

        car_fallback_pt_journey = next((j for j in response['journeys'] if j['type'] == 'car'), None)

        assert car_fallback_pt_journey
        assert car_fallback_pt_journey['sections'][0]['mode'] == 'car'

        car_fallback_duration = car_fallback_pt_journey['sections'][0]['duration']

        query = template.format(min_car=car_fallback_duration + 1)

        response = self.query_region(query)
        car_fallback_pt_journey = next((j for j in response['journeys'] if j['type'] == 'car'), None)

        assert car_fallback_pt_journey
        assert car_fallback_pt_journey['sections'][0]['mode'] == 'car'

        assert 'deleted_because_too_short_heavy_mode_fallback' in car_fallback_pt_journey['tags']


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
        query = (
            "journeys?from={from_sp}&to={to_sp}&datetime={datetime}"
            "&datetime_represents=arrival&_max_successive_physical_mode=3&_max_additional_connections=10".format(
                from_sp="stopP", to_sp="stopT", datetime="20140614T193000"
            )
        )

        response, status = self.query_region(query + "&max_duration=1&max_duration_to_pt=100", check=False)

        assert status == 200
        check_best(response)
        assert response['error']['id'] == "no_solution"
        assert response['error']['message'] == "no solution found for this journey"
        # and no journey is to be provided
        assert 'journeys' not in response or len(response['journeys']) == 0


@dataset({"main_routing_test": {}})
class DirectPath(object):
    def test_journey_direct_path(self):
        query = (
            journey_basic_query
            + "&first_section_mode[]=bss"
            + "&first_section_mode[]=walking"
            + "&first_section_mode[]=bike"
            + "&first_section_mode[]=car"
            + "&debug=true"
        )
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 4

        bike_journey = find_with_tag(response['journeys'], 'non_pt_bike')
        assert bike_journey
        assert len(bike_journey['sections']) == 1

        car_journey = find_with_tag(response['journeys'], 'non_pt_car')
        assert car_journey
        assert len(car_journey['sections']) == 3
        # without filter we use the parking poi:parking_2
        assert car_journey['sections'][0]['to']['id'] == 'poi:parking_2'

        walking_journey = find_with_tag(response['journeys'], 'non_pt_walking')
        assert walking_journey
        assert len(walking_journey['sections']) == 1

        # Test car_journey with filter to forbid the parking used by car_journey above
        query_with_fu = query + '&forbidden_uris[]=poi:parking_2'
        response = self.query_region(query_with_fu)
        check_best(response)
        self.is_valid_journey_response(response, query_with_fu)
        car_journey = find_with_tag(response['journeys'], 'car')
        assert car_journey
        assert len(car_journey['sections']) == 3
        # when the parking 'poi:parking_2' is forbidden we use the parking poi:parking_1
        if self.data_sets['main_routing_test']['scenario'] == u"distributed":
            assert car_journey['sections'][0]['to']['id'] == 'poi:parking_1'

        # Test car_journey with filter to force the parking 'poi:parking_1', not used without any filter on parking
        query_with_ai = query + '&allowed_id[]=poi:parking_1'
        response = self.query_region(query_with_ai)
        check_best(response)
        self.is_valid_journey_response(response, query_with_ai)
        car_journey = find_with_tag(response['journeys'], 'car')
        assert car_journey
        assert len(car_journey['sections']) == 3
        # when the parking 'poi:parking_1' is forced in parameter, we use it
        if self.data_sets['main_routing_test']['scenario'] == u"distributed":
            assert car_journey['sections'][0]['to']['id'] == 'poi:parking_1'

        # Test car_journey with more than one allowed_id, uses only one of the parking
        # as itinerary using another parking is deleted by the raptor
        query_with_mai = query + '&allowed_id[]=poi:parking_1&allowed_id[]=poi:parking_2'
        response = self.query_region(query_with_mai)
        check_best(response)
        self.is_valid_journey_response(response, query_with_mai)
        car_journey = find_with_tag(response['journeys'], 'car')
        assert car_journey
        assert len(car_journey['sections']) == 3
        assert car_journey['sections'][0]['to']['id'] == 'poi:parking_2'

        # Test car_journey with a forbidden_uris[] and an allowed_id[]
        query_with_both = query + '&allowed[]=poi:parking_1&forbidden_uris[]=poi:parking_2'
        response = self.query_region(query_with_both)
        check_best(response)
        self.is_valid_journey_response(response, query_with_both)
        car_journey = find_with_tag(response['journeys'], 'car')
        assert car_journey
        assert len(car_journey['sections']) == 3
        if self.data_sets['main_routing_test']['scenario'] == u"distributed":
            assert car_journey['sections'][0]['to']['id'] == 'poi:parking_1'

    def test_journey_direct_path_only(self):
        query = journey_basic_query + "&first_section_mode[]=walking" + "&direct_path=only"
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1

        # only walking
        assert 'non_pt' in response['journeys'][0]['tags']
        assert len(response['journeys'][0]['sections']) == 1

    def test_journey_direct_path_none(self):
        query = journey_basic_query + "&first_section_mode[]=walking" + "&direct_path=none"
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1

        # only pt journey
        assert 'non_pt' not in response['journeys'][0]['tags']
        assert len(response['journeys'][0]['sections']) > 1

    def test_journey_direct_path_indifferent(self):
        query = journey_basic_query + "&first_section_mode[]=walking" + "&direct_path=indifferent"
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 2

        # first is pt journey
        assert 'non_pt' not in response['journeys'][0]['tags']
        assert len(response['journeys'][0]['sections']) > 1
        # second is walking
        assert 'non_pt' in response['journeys'][1]['tags']
        assert len(response['journeys'][1]['sections']) == 1


@dataset({})
class JourneysNoRegion:
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
class OnBasicRouting:
    """
    Test if the filter on long waiting duration is working
    """

    def test_novalidjourney_on_first_call(self):
        """
        On this call the first call to kraken returns a journey
        with a too long waiting duration.
        The second call to kraken must return a valid journey
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}".format(
            from_sa="A", to_sa="D", datetime="20120614T080000"
        )

        response = self.query_region(query, display=False)
        check_best(response)
        # self.is_valid_journey_response(response, query)# linestring with 1 value (0,0)
        assert len(response['journeys']) == 1
        assert response['journeys'][0]['arrival_date_time'] == "20120614T160000"
        assert response['journeys'][0]['type'] == "best"

        assert len(response["disruptions"]) == 0
        feed_publishers = response["feed_publishers"]
        assert len(feed_publishers) == 2
        for feed_publisher in feed_publishers:
            is_valid_feed_publisher(feed_publisher)

        feed_publisher = next(f for f in feed_publishers if f['id'] == "base_contributor")
        assert feed_publisher["name"] == "base contributor"
        assert feed_publisher["license"] == "L-contributor"
        assert feed_publisher["url"] == "www.canaltp.fr"

        osm = next(f for f in feed_publishers if f['id'] == "osm")
        assert osm["name"] == "openstreetmap"
        assert osm["license"] == "ODbL"
        assert osm["url"] == "https://www.openstreetmap.org/copyright"

    def test_sp_outside_georef(self):
        """
        departure from '5.;5.' coordinates outside street network
        """
        query = "journeys?from={coord}&to={to_sa}&datetime={datetime}".format(
            coord="5.;5.", to_sa="H", datetime="20120615T170000"
        )

        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert 'journeys' in response
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
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}&debug=true".format(
            from_sa="A", to_sa="D", datetime="20120614T080000"
        )
        response = self.query_region(query, display=True)
        check_best(response)
        assert len(response['journeys']) == 2
        assert response['journeys'][0]['arrival_date_time'] == "20120614T150000"
        assert 'to_delete' in response['journeys'][0]['tags']
        assert response['journeys'][1]['arrival_date_time'] == "20120614T160000"
        assert response['journeys'][1]['type'] == "fastest"

    def test_datetime_error(self):
        """
        datetime invalid, we got an error
        """

        def journey(dt):
            return self.query_region(
                "journeys?from={from_sa}&to={to_sa}&datetime={datetime}&debug=true".format(
                    from_sa="A", to_sa="D", datetime=dt
                ),
                check=False,
            )

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
            return self.query_region(
                "journeys?from={from_sa}&to={to_sa}&datetime={datetime}&debug=true".format(
                    from_sa="A", to_sa="D", datetime=dt
                )
            )

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
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}".format(
            from_sa="A", to_sa="D", datetime="20120615T080000"
        )

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
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}".format(
            from_sa="E", to_sa="H", datetime="20120615T080000"
        )

        response = self.query_region(query, display=False)
        assert not "journeys" in response or len(response['journeys']) == 0

    def test_max_attemps_debug(self):
        """
        Kraken always retrieves journeys with non_pt_duration > max_non_pt_duration
        No journeys should be typed, but get_journeys should stop quickly
        We had the debug argument, hence a non-typed journey is returned
        """
        query = "journeys?from={from_sa}&to={to_sa}&datetime={datetime}&debug=true".format(
            from_sa="E", to_sa="H", datetime="20120615T080000"
        )

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
        assert 'journeys' not in response

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
        assert len(response['journeys']) == 2

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
        r, status = self.query_region(
            'heat_maps?from=stop_point:B&datetime=20120615T080000&max_duration=60', check=False
        )

        assert r['error']['id'] == "no_solution"
        assert r['error']['message'] == "no street network data, impossible to compute a heat_map"


@dataset({"main_routing_test": {}, "basic_routing_test": {'check_killed': False}})
class OneDeadRegion:
    """
    Test if we still responds when one kraken is dead
    """

    def test_one_dead_region(self):
        self.krakens_pool["basic_routing_test"].terminate()

        query = "v1/journeys?from=stop_point:stopA&to=stop_point:stopB&datetime=20120614T080000&debug=true&max_duration_to_pt=0"
        response = self.query(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        assert len(response['journeys']) == 1
        assert len(response['journeys'][0]['sections']) == 1
        assert response['journeys'][0]['sections'][0]['type'] == 'public_transport'
        assert len(response['debug']['regions_called']) == 1
        assert response['debug']['regions_called'][0]['name'] == "main_routing_test"


@dataset({"main_routing_without_pt_test": {'priority': 42}, "main_routing_test": {'priority': 10}})
class WithoutPt:
    def test_one_region_without_pt(self):
        """
        Test if we still responds when one kraken has no pt solution
        """
        query = "v1/" + journey_basic_query + "&debug=true"
        response = self.query(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        assert len(response['journeys']) == 2
        assert len(response['journeys'][0]['sections']) == 3
        assert response['journeys'][0]['sections'][1]['type'] == 'public_transport'
        assert len(response['debug']['regions_called']) == 2
        assert response['debug']['regions_called'][0]['name'] == "main_routing_without_pt_test"
        assert response['debug']['regions_called'][1]['name'] == "main_routing_test"

    def test_one_region_without_pt_new_default(self):
        """
        Test if we still responds when one kraken has no pt solution using new_default
        """
        query = "v1/" + journey_basic_query + "&debug=true"
        response = self.query(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        assert len(response['journeys']) == 2
        assert len(response['journeys'][0]['sections']) == 3
        assert response['journeys'][0]['sections'][1]['type'] == 'public_transport'
        assert len(response['debug']['regions_called']) == 2
        assert response['debug']['regions_called'][0]['name'] == "main_routing_without_pt_test"
        assert response['debug']['regions_called'][1]['name'] == "main_routing_test"


@dataset(
    {
        "main_routing_without_pt_test": {"priority": 42, "min_nb_journeys": 10},
        "main_routing_test": {"max_nb_journeys": 1},
    }
)
class NoCoverageParams:
    def test_db_params_when_no_coverage(self):
        """
        Test that parameters from coverage are applied when no coverage is set in the query
        Also, when several coverage are possible, test that each requested coverage parameters are set

        In this test, the dataset with the higher priority will be chosen first and its parameters set.
        However, as there's no PT, the request will be done on the second dataset and its parameters will also be set:
        the result will only 1 journey as the parameter "max_nb_journeys = 1" is set for this coverage only
        """
        query = "v1/" + journey_basic_query
        response = self.query(query)
        # With 'max_nb_journeys'=1, only 1 journey should be available in the response
        assert len(response["journeys"]) == 1

        query_debug = "v1/" + journey_basic_query + "&debug=true"
        response_debug = self.query(query_debug)
        # Check that more than 1 journey should be given if not for the coverage parameter
        assert len(response_debug["journeys"]) > 1


@dataset({"main_ptref_test": {}})
class JourneysWithPtref:
    """
    Test all scenario with ptref_test data
    """

    def test_strange_line_name(self):
        query = "journeys?from=stop_area:stop2&to=stop_area:stop1&datetime=20140107T100000"
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)

        assert len(response['journeys']) == 1

    def test_journeys_crowfly_distances(self):

        query = "journeys?from=9.001;9&to=stop_area:stop2&datetime=20140105T040000"
        response = self.query_region(query)

        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1

        pt_journey = next((j for j in response['journeys']), None)
        assert pt_journey

        distances = pt_journey['distances']
        assert distances['walking'] == pytest.approx(109, abs=2)
        assert distances['car'] == distances['bike'] == distances['ridesharing'] == 0
        durations = pt_journey['durations']
        assert durations['walking'] == pytest.approx(138, abs=2)
        assert durations['total'] == pytest.approx(3438, abs=2)
        assert durations['car'] == distances['bike'] == distances['ridesharing'] == 0


@dataset({"main_routing_test": {}})
class JourneyMinBikeMinCar(object):
    def test_first_section_mode_and_last_section_mode_bike(self):
        query = '{sub_query}&last_section_mode[]=bike&first_section_mode[]=bike&' 'datetime={datetime}'.format(
            sub_query=sub_query, datetime="20120614T080000"
        )
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1
        assert len(response['journeys'][0]['sections']) == 1
        assert response['journeys'][0]['sections'][0]['mode'] == 'bike'
        assert response['journeys'][0]['sections'][0]['duration'] == 62

    def test_first_section_mode_bike_and_last_section_mode_walking(self):
        query = (
            '{sub_query}&last_section_mode[]=bike&first_section_mode[]=walking&'
            'datetime={datetime}'.format(sub_query=sub_query, datetime="20120614T080000")
        )
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1
        assert len(response['journeys'][0]['sections']) == 1
        assert response['journeys'][0]['sections'][0]['mode'] == 'walking'
        assert response['journeys'][0]['sections'][0]['duration'] == 276

        query = (
            '{sub_query}&last_section_mode[]=bike&first_section_mode[]=walking&_min_bike=0&'
            'datetime={datetime}'.format(sub_query=sub_query, datetime="20120614T080000")
        )
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
        query = '{sub_query}&last_section_mode[]=bike&datetime={datetime}'.format(
            sub_query=sub_query, datetime="20120614T080000"
        )
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1
        assert len(response['journeys'][0]['sections']) == 1
        assert response['journeys'][0]['sections'][0]['mode'] == 'walking'
        assert response['journeys'][0]['sections'][0]['duration'] == 276

        query = '{sub_query}&_min_bike=0&last_section_mode[]=bike&datetime={datetime}'.format(
            sub_query=sub_query, datetime="20120614T080000"
        )
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
        query = (
            '{sub_query}&last_section_mode[]=walking&first_section_mode[]=bike&'
            'datetime={datetime}'.format(sub_query=sub_query, datetime="20120614T080000")
        )
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1
        assert len(response['journeys'][0]['sections']) == 1
        assert response['journeys'][0]['sections'][0]['mode'] == 'bike'
        assert response['journeys'][0]['sections'][0]['duration'] == 62

    def test_first_section_mode_bike_only(self):
        query = '{sub_query}&first_section_mode[]=bike&datetime={datetime}'.format(
            sub_query=sub_query, datetime="20120614T080000"
        )
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1
        assert len(response['journeys'][0]['sections']) == 1
        assert response['journeys'][0]['sections'][0]['mode'] == 'bike'
        assert response['journeys'][0]['sections'][0]['duration'] == 62

    def test_first_section_mode_bike_walking_only(self):
        query = (
            '{sub_query}&first_section_mode[]=walking&first_section_mode[]=bike&_min_bike=0&'
            'datetime={datetime}'.format(sub_query=sub_query, datetime="20120614T080000")
        )
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 3

        assert len(response['journeys'][0]['sections']) == 1
        assert response['journeys'][0]['sections'][0]['mode'] == 'bike'
        assert response['journeys'][0]['sections'][0]['duration'] == 62

        assert len(response['journeys'][1]['sections']) == 3
        assert response['journeys'][1]['sections'][0]['mode'] == 'walking'
        assert response['journeys'][1]['sections'][0]['duration'] == 17
        assert response['journeys'][1]['sections'][-1]['mode'] == 'walking'
        assert response['journeys'][1]['sections'][-1]['duration'] == 80

        assert len(response['journeys'][-1]['sections']) == 1
        assert response['journeys'][-1]['sections'][0]['mode'] == 'walking'
        assert response['journeys'][-1]['sections'][0]['duration'] == 276

    def test_first_section_mode_and_last_section_mode_car(self):
        query = '{sub_query}&last_section_mode[]=car&first_section_mode[]=car&' 'datetime={datetime}'.format(
            sub_query=sub_query, datetime="20120614T070000"
        )
        response = self.query_region(query)
        assert 'journeys' not in response

        query = (
            '{sub_query}&_min_car=0&last_section_mode[]=car&first_section_mode[]=car&'
            'datetime={datetime}'.format(sub_query=sub_query, datetime="20120614T070000")
        )
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1

        assert len(response['journeys'][0]['sections']) == 3

        assert response['journeys'][0]['sections'][0]['mode'] == 'car'
        car_duration = response['journeys'][0]['sections'][0]['duration']
        assert car_duration == 16

        assert response['journeys'][0]['sections'][-1]['mode'] == 'walking'
        walk_duration = response['journeys'][0]['sections'][-1]['duration']
        assert walk_duration == 106

        assert response['journeys'][0]['durations']['walking'] == walk_duration
        assert response['journeys'][0]['durations']['car'] == car_duration
        assert response['journeys'][0]['distances']['car'] == 186

    def test_activate_min_car_bike(self):
        modes = [
            'last_section_mode[]=car&first_section_mode[]=car&last_section_mode[]=walking&first_section_mode[]=walking',
            'last_section_mode[]=bike&first_section_mode[]=car&last_section_mode[]=walking&first_section_mode[]=walking',
            'last_section_mode[]=car&last_section_mode[]=walking',
            'first_section_mode[]=car&first_section_mode[]=walking',
        ]
        modes_2 = [
            'last_section_mode[]=bike&first_section_mode[]=bike&last_section_mode[]=walking&first_section_mode[]=walking'
        ]
        for mode in modes:
            query = '{sub_query}&{mode}&datetime={datetime}'.format(
                sub_query=sub_query, mode=mode, datetime="20120614T080000"
            )
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

        for mode in modes_2:
            query = '{sub_query}&{mode}&datetime={datetime}'.format(
                sub_query=sub_query, mode=mode, datetime="20120614T080000"
            )
            response = self.query_region(query)
            self.is_valid_journey_response(response, query)
            assert len(response['journeys']) == 3

            assert len(response['journeys'][0]['sections']) == 1
            assert response['journeys'][0]['sections'][0]['mode'] == 'bike'
            assert response['journeys'][0]['sections'][0]['duration'] == 62

            assert len(response['journeys'][1]['sections']) == 3
            assert response['journeys'][1]['sections'][0]['mode'] == 'walking'
            assert response['journeys'][1]['sections'][0]['duration'] == 17
            assert response['journeys'][1]['sections'][1]['type'] == 'public_transport'
            assert response['journeys'][1]['sections'][1]['duration'] == 2
            assert response['journeys'][1]['sections'][-1]['mode'] == 'walking'
            assert response['journeys'][1]['sections'][-1]['duration'] == 80

            assert len(response['journeys'][2]['sections']) == 1
            assert response['journeys'][2]['sections'][0]['mode'] == 'walking'
            assert response['journeys'][2]['sections'][0]['duration'] == 276

    def test_min_nb_transfers(self):
        query = '{sub_query}&datetime={datetime}'.format(sub_query=sub_query, datetime="20120614T080000")
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 2

        # set the min_nb_transfers to 1
        query = '{sub_query}&datetime={datetime}&min_nb_transfers=1&debug=true'.format(
            sub_query=sub_query, datetime="20120614T080000"
        )
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)
        # Among two direct_path with same duration and path, one is eliminated.
        assert len(response['journeys']) >= 3
        assert all("deleted_because_not_enough_connections" in j['tags'] for j in response['journeys'])


@dataset({"min_nb_journeys_test": {}})
class JourneysMinNbJourneys:
    """
    Test min_nb_journeys and late journey filter
    """

    def test_min_nb_journeys_options_with_minimum_value(self):
        """
        By default, the raptor computes 2 journeys, so the response returns at least 2 journeys.

        Note : The night bus filter is loaded with default parameters.
        With this data, night bus filter parameters doesn't filter anything.
        """
        query = 'journeys?from=2.39592;48.84838&to=2.36381;48.86750&datetime=20180309T080000&min_nb_journeys=0'
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) >= 2

    def test_minimum_journeys_with_min_nb_journeys_options(self):
        """
        By default, the raptor computes 2 journeys, so the response returns at least 2 journeys.

        Note : The night bus filter is loaded with default parameters.
        With this data, night bus filter parameters doesn't filter anything.
        """
        query = "journeys?from=2.39592;48.84838&to=2.36381;48.86750&datetime=20180309T080000&min_nb_journeys=1"
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) >= 2

    def test_min_nb_journeys_options(self):
        """
        The data contains only 6 journeys, so the response returns at least 3 journeys.

        Note : The night bus filter is loaded with default parameters.
        With this data, night bus filter parameters doesn't filter anything.
        """
        query = "journeys?from=2.39592;48.84838&to=2.36381;48.86750&datetime=20180309T080000&min_nb_journeys=3"
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) >= 3

    def test_maximum_journeys_with_min_nb_journeys_options(self):
        """
        The data contains only 6 journeys but we want 7 journeys.
        The response has to contain only 6 journeys.

        Note : The night bus filter is loaded with default parameters.
        With this data, night bus filter parameters doesn't filter anything.
        """
        query = "journeys?from=2.39592;48.84838&to=2.36381;48.86750&datetime=20180309T080000&min_nb_journeys=7"
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) <= 6


@dataset({"min_nb_journeys_test": {}})
class JourneysWithNightBusFilter:
    def classical_night_bus_filter_parameters(self):
        """
        The data contains 6 journeys with min_nb_journeys = 5.
        We active the min_nb_journeys option and change night bus filter parameters
        (ax + b with a = 1.2, b = 0) to filter the 2 latest journeys.
        """
        query = "journeys?from=2.39592;48.84838&to=2.36381;48.86750&datetime=20180309T080000&min_nb_journeys=5&_night_bus_filter_base_factor=0&_night_bus_filter_max_factor=1.2"
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 5

    def test_night_bus_filter_parameters_with_minimum_values_1(self):
        """
        Raptor computes 2 journeys.
        parameters :
        _night_bus_filter_base_factor = 0
        _night_bus_filter_max_factor = 0

        ax + b with a = 0, b = 0

        We compare 2 journeys but one of them is forced to 0
        We shouldn't have this configuration of filter, but rather ax + b with a = 1, b = 0
        to have a filter with no effect.
        """
        query = "journeys?from=2.39592;48.84838&to=2.36381;48.86750&datetime=20180309T080400&_night_bus_filter_base_factor=0&_night_bus_filter_max_factor=0"
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1

    def test_night_bus_filter_parameters_with_minimum_values_2(self):
        """
        Raptor computes 2 journeys without min_nb_journeys parameters.
        parameters :
        _night_bus_filter_base_factor = 0
        _night_bus_filter_max_factor = 1

        ax + b with a = 1, b = 0

        The filter is active but we compare directly the 2 journeys
        It's the tightest configuration
        """
        query = "journeys?from=2.39592;48.84838&to=2.36381;48.86750&datetime=20180309T080400&_night_bus_filter_base_factor=0&_night_bus_filter_max_factor=1"
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 1

    def test_night_bus_filter_parameters_with_maximum_values_3(self):
        """
        Raptor computes 2 journeys.
        parameters :
        _night_bus_filter_base_factor = 86040 (~ infinite)
        _night_bus_filter_max_factor = 1000 (~ infinite)

        ax + b with a = 1000, b = 86040

        Filter parameters are infinite, so the response contains all journey (This is not filtered)
        It's the widest configuration
        """
        query = "journeys?from=2.39592;48.84838&to=2.36381;48.86750&datetime=20180309T080400&_night_bus_filter_base_factor=86040&_night_bus_filter_max_factor=1000"
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)
        assert len(response['journeys']) == 2


@dataset({'min_nb_journeys_test': {}})
class JourneysTimeFrameDuration:
    def test_timeframe_duration_simple_case(self):
        """
        The data contains 20 journeys (every 10 min) + 1 journey 24H after the first.
        The first journeys is 20180315T080000.
        """

        # Time frame to catch only the first journeys, timeframe_duration = 10 min (60*10=600).
        # Even though a journey's departure is later than the timeframe, we still keep it
        query = (
            'journeys?from={_from}&' 'to={to}&' 'datetime={datetime}&' 'timeframe_duration={timeframe_duration}&'
        ).format(_from='stop_area:sa1', to='stop_area:sa3', datetime='20180315T080000', timeframe_duration=600)
        response = self.query_region(query)
        assert 1 <= len(response['journeys'])

        # Time frame to catch journeys in the first hour, timeframe_duration = 1 H (60*60=3600).
        query = (
            'journeys?from={_from}&' 'to={to}&' 'datetime={datetime}&' 'timeframe_duration={timeframe_duration}&'
        ).format(_from='stop_area:sa1', to='stop_area:sa3', datetime='20180315T080000', timeframe_duration=3600)
        response = self.query_region(query)
        assert 6 <= len(response['journeys'])

        # Time frame to catch only the first two journeys before the date time, because clockwise is active.
        query = (
            'journeys?from={_from}&'
            'to={to}&'
            'datetime={datetime}&'
            'datetime_represents={datetime_represents}&'
            'timeframe_duration={timeframe_duration}&'
        ).format(
            _from='stop_area:sa1',
            to='stop_area:sa3',
            datetime='20180315T083500',
            datetime_represents='arrival',
            timeframe_duration=1200,
        )
        response = self.query_region(query)
        assert 2 <= len(response['journeys'])

        assert response['journeys'][0]['departure_date_time'] == u'20180315T083000'
        assert response['journeys'][1]['departure_date_time'] == u'20180315T082000'

    def test_timeframe_duration_with_minimum_value(self):
        """
        The data contains 20 journeys (every 10 min) + 1 journey 24H after the first.
        The first journeys is 20180315T080000.

        If timeframe_duration is set to 0, the response should return 1 journey,
        as there is simply no contraint
        """
        query = (
            'journeys?from={_from}&' 'to={to}&' 'datetime={datetime}&' 'timeframe_duration={timeframe_duration}&'
        ).format(_from='stop_area:sa1', to='stop_area:sa3', datetime='20180315T080000', timeframe_duration=0)
        response = self.query_region(query)
        assert 1 == len(response['journeys'])

    def test_timeframe_duration_and_min_nb_journeys_with_minimum_value(self):
        """
        The data contains 20 journeys (every 10 min) + 1 journey 24H after the first.
        The first journeys is 20180315T080000.

        If timeframe_duration and min_nb_journeys are set to 0, the response should return 1 journey,
        as there is simply no constraints

        If timeframe_duration is set to 0 and min_nb_journeys is set to 2,
        the response should return at least 2 journeys
        """
        query = (
            'journeys?from={_from}&'
            'to={to}&'
            'datetime={datetime}&'
            'timeframe_duration={timeframe_duration}&'
            'min_nb_journeys={min_nb_journeys}'
        ).format(
            _from='stop_area:sa1',
            to='stop_area:sa3',
            datetime='20180315T080000',
            min_nb_journeys=0,
            timeframe_duration=0,
        )
        response = self.query_region(query)
        assert 1 <= len(response['journeys'])

        query = (
            'journeys?from={_from}&'
            'to={to}&'
            'datetime={datetime}&'
            'timeframe_duration={timeframe_duration}&'
            'min_nb_journeys={min_nb_journeys}'
        ).format(
            _from='stop_area:sa1',
            to='stop_area:sa3',
            datetime='20180315T080000',
            min_nb_journeys=2,
            timeframe_duration=0,
        )
        response = self.query_region(query)
        assert 2 <= len(response['journeys'])

    def test_timeframe_duration_with_maximum_value(self):
        """
        The data contains 20 journeys (every 10 min) + 1 journey 24H after the first.
        The first journeys is 20180315T080000.

        timeframe_duration is set to 24H + 15 min (86400 + 60*15).
        The response must not contains the last jouneys because we filter with a max time frame of 24H
        """
        query = (
            'journeys?from={_from}&' 'to={to}&' 'datetime={datetime}&' 'timeframe_duration={timeframe_duration}&'
        ).format(_from='stop_area:sa1', to='stop_area:sa3', datetime='20180315T080000', timeframe_duration=87300)
        response = self.query_region(query)
        assert 20 == len(response['journeys'])

    def test_timeframe_duration_with_min_nb_journeys(self):
        """
        The data contains 20 journeys (every 10 min) + 1 journey 24H after the first.
        The first journeys is 20180315T080000.

        timeframe_duration and min_nb_journeys is active

        """

        # min_nb_journeys = 8 and timeframe_duration = 1H (60*60 = 3600)
        # The response have to contain 20 journeys because min_nb_journeys is verified.
        # The superior criteria is min_nb_journeys, so we continue until we have 8 journeys.
        query = (
            'journeys?from={_from}&'
            'to={to}&'
            'datetime={datetime}&'
            'min_nb_journeys={min_nb_journeys}&'
            'timeframe_duration={timeframe_duration}&'
        ).format(
            _from='stop_area:sa1',
            to='stop_area:sa3',
            datetime='20180315T080000',
            min_nb_journeys=8,
            timeframe_duration=3600,
        )
        response = self.query_region(query)
        assert 8 <= len(response['journeys'])

        # min_nb_journeys = 8 and timeframe_duration = 24H + 15 min
        # The response have to contain 20 journeys because min_nb_journeys is verified.
        # The superior criteria is timeframe_duration, so we continue until we have 20 journeys.
        query = (
            'journeys?from={_from}&'
            'to={to}&'
            'datetime={datetime}&'
            'min_nb_journeys={min_nb_journeys}&'
            'timeframe_duration={timeframe_duration}&'
        ).format(
            _from='stop_area:sa1',
            to='stop_area:sa3',
            datetime='20180315T080000',
            min_nb_journeys=8,
            timeframe_duration=87300,
        )
        response = self.query_region(query)
        assert 20 == len(response['journeys'])

        # min_nb_journeys = 2 and timeframe_duration = 1H (60*60 = 3600)
        # The response have to contains 6 journeys because min_nb_journeys is verified.
        # The superior criteria is timeframe_duration and we have 6 journeys in 1H.
        query = (
            'journeys?from={_from}&'
            'to={to}&'
            'datetime={datetime}&'
            'min_nb_journeys={min_nb_journeys}&'
            'timeframe_duration={timeframe_duration}&'
        ).format(
            _from='stop_area:sa1',
            to='stop_area:sa3',
            datetime='20180315T080000',
            min_nb_journeys=2,
            timeframe_duration=3600,
        )
        response = self.query_region(query)
        assert 6 <= len(response['journeys'])

        # min_nb_journeys = 11 and timeframe_duration = 1H 35min (60*95 = 5700)
        # The response have to contains 11 journeys because min_nb_journeys is the main criteria.
        # With timeframe_duration = 1h35min, we can find 10 journeys but min_nb_journeys=11.
        # So we continue until the eleventh.
        query = (
            'journeys?from={_from}&'
            'to={to}&'
            'datetime={datetime}&'
            'min_nb_journeys={min_nb_journeys}&'
            'timeframe_duration={timeframe_duration}&'
        ).format(
            _from='stop_area:sa1',
            to='stop_area:sa3',
            datetime='20180315T080000',
            min_nb_journeys=11,
            timeframe_duration=5700,
        )
        response = self.query_region(query)
        assert 11 == len(response['journeys'])

        # min_nb_journeys = 20 and timeframe_duration = 4H (60*60*4 = 14400)
        # The response have to contains 20 journeys because min_nb_journeys is verified.
        # In 4H, the data contains 20 journeys.
        query = (
            'journeys?from={_from}&'
            'to={to}&'
            'datetime={datetime}&'
            'min_nb_journeys={min_nb_journeys}&'
            'timeframe_duration={timeframe_duration}&'
        ).format(
            _from='stop_area:sa1',
            to='stop_area:sa3',
            datetime='20180315T080000',
            min_nb_journeys=20,
            timeframe_duration=14400,
        )
        response = self.query_region(query)
        assert 20 == len(response['journeys'])

        # min_nb_journeys = 21 and timeframe_duration = 4H (60*60*4 = 14400)
        # The response have to contains 20 journeys because min_nb_journeys is not verified.
        # Criteria is not verified, because we don't have 21 journeys in the time frame duration.
        # We continue to search out of the bound, but only during 24h if the min_nb_journeys is
        # always not verified (and we reach that 24h-max limit before finding 21st journey)
        query = (
            'journeys?from={_from}&'
            'to={to}&'
            'datetime={datetime}&'
            'min_nb_journeys={min_nb_journeys}&'
            'timeframe_duration={timeframe_duration}&'
        ).format(
            _from='stop_area:sa1',
            to='stop_area:sa3',
            datetime='20180315T080000',
            min_nb_journeys=21,
            timeframe_duration=14400,
        )
        response = self.query_region(query)
        assert 20 == len(response['journeys'])


@dataset({"main_routing_test": {}})
class JourneysRidesharing:
    def test_first_ridesharing_last_walking_section_mode(self):
        query = (
            "journeys?from=0.0000898312;0.0000898312&to=0.00188646;0.000449156&datetime=20120614T075500&"
            "first_section_mode[]={first}&last_section_mode[]={last}&debug=true".format(
                first='ridesharing', last='walking'
            )
        )
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
        assert 'to_delete' in rs_journey["tags"]  # no response provided for ridesharing: to_delete
        rs_section = rs_journey["sections"][0]
        assert rs_section["departure_date_time"] == rs_journey["departure_date_time"]
        assert rs_section["arrival_date_time"] == rs_journey["arrival_date_time"]
        assert rs_section["duration"] == rs_journey["duration"]
        assert rs_section["mode"] == "ridesharing"
        assert rs_section["type"] == "street_network"
        assert rs_section["id"]  # check that id is provided
        assert rs_section["geojson"]["properties"][0]["length"] == rs_journey["distances"]["ridesharing"]
        assert rs_section["geojson"]["coordinates"][0][0] == approx(
            float(rs_section["from"]["address"]["coord"]["lon"])
        )
        assert rs_section["geojson"]["coordinates"][0][1] == approx(
            float(rs_section["from"]["address"]["coord"]["lat"])
        )
        assert rs_section["geojson"]["coordinates"][-1][0] == approx(
            float(rs_section["to"]["address"]["coord"]["lon"])
        )
        assert rs_section["geojson"]["coordinates"][-1][1] == approx(
            float(rs_section["to"]["address"]["coord"]["lat"])
        )

    def test_asynchronous_ridesharing_mode(self):
        query = (
            "journeys?from=0.0000898312;0.0000898312&to=0.00188646;0.000449156&datetime=20120614T075500&"
            "first_section_mode[]={first}&last_section_mode[]={last}&debug=true&_asynchronous_ridesharing=True".format(
                first='ridesharing', last='walking'
            )
        )
        response = self.query_region(query)
        check_best(response)
        assert len(response["journeys"]) == 1
        assert response["journeys"][0]["type"] == "best"
        rs_journey = response["journeys"][0]
        assert "ridesharing" in rs_journey["tags"]
        rs_section = rs_journey["sections"][0]
        assert rs_section["mode"] == "ridesharing"
        assert rs_section["type"] == "street_network"
        # A link is added to call the new ridesharing API
        links = response["links"]
        link_is_present = False
        for link in links:
            if "ridesharing" in link["rel"]:
                link_is_present = True
                break
        assert link_is_present

    def test_first_ridesharing_section_mode_forbidden(self):
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


@dataset({"main_routing_test": {}})
class JourneysDirectPathMode:
    def test_direct_path_mode_bike(self):
        """
        test that the journey returns a direct_path_in bike and no direct path in walking
        because of direct_path_mode[]=bike and a pt itinerary with walking fallback
        because of first_section_mode[]=walking
        """
        query = (
            "journeys?from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&datetime=20120614T075500&"
            "first_section_mode[]=walking&last_section_mode[]=walking&_min_bike=0&direct_path_mode[]=bike"
        )
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response["journeys"]) == 2
        assert response["journeys"][0]["type"] == "best"
        assert "bike" in response["journeys"][0]["tags"]
        assert "non_pt" in response["journeys"][0]["tags"]

        assert "walking" in response["journeys"][1]["tags"]
        assert "non_pt" not in response["journeys"][1]["tags"]

    def test_direct_path_mode_bike_and_direct_path_mode_none(self):
        """
        test that the journey returns no direct_path because of direct_path=none
        even with direct_path_mode[]=bike and a pt itinerary with walking fallback
        because of first_section_mode[]=walking
        """
        query = (
            "journeys?from=0.0000898312;0.0000898312&to=0.00188646;0.00071865&datetime=20120614T075500&"
            "first_section_mode[]=walking&last_section_mode[]=walking&_min_bike=0&direct_path_mode[]=bike&direct_path=none"
        )
        response = self.query_region(query)
        check_best(response)
        self.is_valid_journey_response(response, query)
        assert len(response["journeys"]) == 1
        assert "walking" in response["journeys"][0]["tags"]
        assert "non_pt" not in response["journeys"][0]["tags"]


@dataset({"min_nb_journeys_test": {}})
class JourneysTickets:
    def test_journey_tickets(self):
        """
        2 solutions are found with 1 ticket for each journey
        """
        query = 'journeys?from=2.39592;48.84838&to=2.36381;48.86750&datetime=20180309T080000'
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)

        # Journeys
        assert len(response['journeys']) == 2
        assert 'fare' in response['journeys'][0]
        assert response['journeys'][0]['fare']['total']['value'] == '120.0'
        assert response['journeys'][0]['fare']['total']['currency'] == 'euro'
        assert len(response['journeys'][0]['fare']['links']) == 1
        assert 'fare' in response['journeys'][1]
        assert response['journeys'][1]['fare']['total']['value'] == '100.0'
        assert response['journeys'][1]['fare']['total']['currency'] == 'euro'
        assert len(response['journeys'][1]['fare']['links']) == 1

        # Tickets
        assert len(response['tickets']) == 2
        assert response['tickets'][0]['name'] == 'A-Ticket name'
        assert response['tickets'][0]['source_id'] == 'A-Ticket'
        assert response['tickets'][0]['comment'] == 'A-Ticket comment'
        assert response['tickets'][1]['name'] == 'B-Ticket name'
        assert response['tickets'][1]['source_id'] == 'B-Ticket'
        assert response['tickets'][1]['comment'] == 'B-Ticket comment'

        # Links between journeys.fare and journeys.tickets
        assert response['tickets'][0]['id'] == response['journeys'][1]['fare']['links'][0]['id']
        assert response['tickets'][1]['id'] == response['journeys'][0]['fare']['links'][0]['id']

    def test_journey_tickets_with_max_nb_journeys(self):
        """
        2 solutions found, but with max_nb_journeys option, only 1 journey remains.
        Check there is only one ticket.
        The max_nb_journeys option forces filtering of one journey and the associated ticket too
        """
        query = 'journeys?from=2.39592;48.84838&to=2.36381;48.86750&datetime=20180309T080000&max_nb_journeys=1'
        response = self.query_region(query)
        self.is_valid_journey_response(response, query)

        # Journeys
        assert len(response['journeys']) == 1
        assert 'fare' in response['journeys'][0]
        assert response['journeys'][0]['fare']['total']['value'] == '120.0'
        assert response['journeys'][0]['fare']['total']['currency'] == 'euro'
        assert len(response['journeys'][0]['fare']['links']) == 1

        # Tickets
        assert len(response['tickets']) == 1
        assert response['tickets'][0]['name'] == 'B-Ticket name'
        assert response['tickets'][0]['source_id'] == 'B-Ticket'
        assert response['tickets'][0]['comment'] == 'B-Ticket comment'

        # Links between journeys.fare and journeys.tickets
        assert response['tickets'][0]['id'] == response['journeys'][0]['fare']['links'][0]['id']


@dataset({"main_stif_test": {}})
class JourneysTicketsWithDebug:
    def test_journey_tickets_with_debug_true(self):
        """
              Line P         Line Q         Line R         Line S
              P-Ticket       Q-Ticket       R-Ticket       S-Ticket
        stopP -------> stopQ -------> stopR -------> stopS -------> stopT
        15:00          16:00          17:00          18:00          19:00

                                     Line T
                                     T-Ticket
        stopP ----------------------------------------------------> stopT
        15:00                                                       20:00
        Goal : Test if tickets work correctly with the debug option
        """
        # debug = false
        # Tickets have to be filtered like journeys
        query = "journeys?from=stopP&to=stopT&datetime=20140614T145500&" "_max_successive_physical_mode=3"
        response = self.query_region(query)

        # Journeys
        assert len(response['journeys']) == 1
        assert 'fare' in response['journeys'][0]
        assert response['journeys'][0]['fare']['total']['value'] == '99.0'
        assert response['journeys'][0]['fare']['total']['currency'] == 'euro'
        assert len(response['journeys'][0]['fare']['links']) == 1

        # Tickets
        assert len(response['tickets']) == 1
        assert response['tickets'][0]['source_id'] == 'T-Ticket'

        # Links between journeys.fare and journeys.tickets
        assert response['tickets'][0]['id'] == response['journeys'][0]['fare']['links'][0]['id']

        # debug = true
        # All solutions are retreived with their associated tickets
        query = (
            "journeys?from=stopP&to=stopT&datetime=20140614T145500&" "_max_successive_physical_mode=3&debug=true"
        )
        response = self.query_region(query)

        # Journeys
        assert len(response['journeys']) == 2
        assert response['journeys'][0]['fare']['total']['value'] == '400.0'
        assert response['journeys'][0]['fare']['total']['currency'] == 'euro'
        assert len(response['journeys'][0]['fare']['links']) == 4
        assert response['journeys'][1]['fare']['total']['value'] == '99.0'
        assert response['journeys'][1]['fare']['total']['currency'] == 'euro'
        assert len(response['journeys'][1]['fare']['links']) == 1

        # Tickets
        assert len(response['tickets']) == 5
        assert response['tickets'][0]['source_id'] == 'P-Ticket'
        assert response['tickets'][1]['source_id'] == 'Q-Ticket'
        assert response['tickets'][2]['source_id'] == 'R-Ticket'
        assert response['tickets'][3]['source_id'] == 'S-Ticket'
        assert response['tickets'][4]['source_id'] == 'T-Ticket'

        # Links between journeys.fare and journeys.tickets
        assert response['tickets'][0]['id'] == response['journeys'][0]['fare']['links'][0]['id']
        assert response['tickets'][1]['id'] == response['journeys'][0]['fare']['links'][1]['id']
        assert response['tickets'][2]['id'] == response['journeys'][0]['fare']['links'][2]['id']
        assert response['tickets'][3]['id'] == response['journeys'][0]['fare']['links'][3]['id']
        assert response['tickets'][4]['id'] == response['journeys'][1]['fare']['links'][0]['id']
