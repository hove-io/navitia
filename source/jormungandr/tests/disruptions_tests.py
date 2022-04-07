# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
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
from .tests_mechanism import AbstractTestFixture, dataset
from .check_utils import (
    get_not_null,
    get_all_element_disruptions,
    get_disruptions,
    is_valid_line,
    is_valid_disruption,
    is_valid_network,
    is_valid_stop_area,
    is_valid_line_report,
    s_coord,
    get_disruption,
)
import jmespath


def get_impacts(response):
    return {d['id']: d for d in response['disruptions']}


# for the tests we need custom datetime to display the disruptions
default_date_filter = '_current_datetime=20120801T000000'


@dataset({"main_routing_test": {}})
class TestDisruptions(AbstractTestFixture):
    """
    Test the structure of the disruption api
    """

    def test_disruption_end_point(self):
        """
        simple disruption call

        we'll get everything, so:
        one impacted line
        one impacted network
        one impacted stop_area
        """

        response = self.query_region('traffic_reports?' + default_date_filter)

        global_traffic_report = get_not_null(response, 'traffic_reports')

        def check_response(traffic_report):
            # the disruptions are grouped by network and we have only one network
            assert len(traffic_report) == 1

            impacted_lines = get_not_null(traffic_report[0], 'lines')
            assert len(impacted_lines) == 1
            is_valid_line(impacted_lines[0], depth_check=0)
            assert impacted_lines[0]['id'] == 'A'

            lines_disrupt = get_disruptions(impacted_lines[0], response)
            assert len(lines_disrupt) == 2
            for d in lines_disrupt:
                is_valid_disruption(d)
            assert lines_disrupt[0]['disruption_id'] == 'disruption_on_line_A'
            assert lines_disrupt[0]['contributor'] == 'contrib'
            assert lines_disrupt[0]['uri'] == 'too_bad_again'
            assert lines_disrupt[0]['severity']['name'] == 'bad severity'

            application_patterns = lines_disrupt[0]["application_patterns"]
            assert len(application_patterns) == 1
            assert application_patterns[0]["application_period"]["begin"] == "20120801"
            assert application_patterns[0]["application_period"]["end"] == "20120901"
            week_days = ['monday', 'tuesday', 'friday', 'wednesday', 'thursday', 'sunday', 'saturday']
            for day in week_days:
                assert application_patterns[0]["week_pattern"][day]

            application_patterns = lines_disrupt[1]["application_patterns"]
            assert len(application_patterns) == 2
            assert application_patterns[0]["application_period"]["begin"] == "20121001"
            assert application_patterns[0]["application_period"]["end"] == "20121015"

            for day in week_days:
                assert application_patterns[0]["week_pattern"][day]
            time_slots = application_patterns[0]["time_slots"]
            assert len(time_slots) == 1
            assert time_slots[0]["begin"] == '000000'
            assert time_slots[0]["end"] == '120000'

            assert application_patterns[1]["application_period"]["begin"] == "20121201"
            assert application_patterns[1]["application_period"]["end"] == "20121215"
            for day in week_days:
                assert application_patterns[0]["week_pattern"][day]

            time_slots = application_patterns[0]["time_slots"]
            assert len(time_slots) == 1
            assert time_slots[0]["begin"] == '000000'
            assert time_slots[0]["end"] == '120000'

            # we should be able to find the line in the disruption impacted objects
            impacted_objs = lines_disrupt[0]['impacted_objects']
            # this disruption impacts a line and a network
            assert len(impacted_objs) == 2
            impacted_lines = [o for o in impacted_objs if o.get('pt_object').get('embedded_type') == 'line']
            assert len(impacted_lines) == 1
            # it's a line impact, there should not be a 'impacted_section' section
            # (it's only for linesections impacts)
            assert 'impacted_section' not in impacted_lines[0]
            assert impacted_lines[0]['pt_object']['id'] == 'A'

            assert lines_disrupt[1]['uri'] == 'later_impact'

            impacted_network = get_not_null(traffic_report[0], 'network')
            is_valid_network(impacted_network, depth_check=0)
            assert impacted_network['id'] == 'base_network'
            network_disrupt = get_disruptions(impacted_network, response)
            assert len(network_disrupt) == 2
            for d in network_disrupt:
                is_valid_disruption(d)
            assert network_disrupt[0]['disruption_id'] == 'disruption_on_line_A'
            assert network_disrupt[0]['uri'] == 'too_bad_again'

            assert network_disrupt[1]['contributor'] == ''

            assert network_disrupt[1]['disruption_id'] == 'disruption_on_line_A_but_later'
            assert network_disrupt[1]['uri'] == 'later_impact'

            impacted_stop_areas = get_not_null(traffic_report[0], 'stop_areas')
            assert len(impacted_stop_areas) == 1
            is_valid_stop_area(impacted_stop_areas[0], depth_check=0)
            assert impacted_stop_areas[0]['id'] == 'stopA'
            stop_disrupt = get_disruptions(impacted_stop_areas[0], response)
            assert len(stop_disrupt) == 2
            for d in stop_disrupt:
                is_valid_disruption(d)
            assert stop_disrupt[0]['disruption_id'] == 'disruption_on_stop_A'
            assert stop_disrupt[0]['uri'] == 'too_bad'

            properties = get_not_null(stop_disrupt[0], 'properties')
            assert len(properties) == 2
            assert properties[0]['key'] == 'fo'
            assert properties[0]['type'] == 'obar'
            assert properties[0]['value'] == '42'

            assert properties[1]['key'] == 'foo'
            assert properties[1]['type'] == 'bar'
            assert properties[1]['value'] == '42'

        check_response(global_traffic_report)

        # we should also we able to find the disruption with a place_nerby
        # (since the coords in the # datasets are very close, we find all)

        coord_traffic_report = self.query_region(
            'coords/{s}/traffic_reports?{dt_filter}'.format(s=s_coord, dt_filter=default_date_filter)
        )

        check_response(get_not_null(coord_traffic_report, 'traffic_reports'))

        """
        by querying directly the impacted object, we only find publishable disruptions
        """
        networks = self.query_region('networks/base_network?' + default_date_filter)
        network = get_not_null(networks, 'networks')[0]
        is_valid_network(network)
        network_disrupt = get_disruptions(network, response)
        assert len(network_disrupt) == 2
        for d in network_disrupt:
            is_valid_disruption(d)
        assert network_disrupt[0]['disruption_id'] == 'disruption_on_line_A'
        assert network_disrupt[0]['uri'] == 'too_bad_again'
        assert network_disrupt[1]['disruption_id'] == 'disruption_on_line_A_but_later'
        assert network_disrupt[1]['uri'] == 'later_impact'

        lines = self.query_region('lines/A?' + default_date_filter)
        line = get_not_null(lines, 'lines')[0]
        is_valid_line(line)
        lines_disrupt = get_disruptions(line, response)
        assert len(lines_disrupt) == 2
        for d in lines_disrupt:
            is_valid_disruption(d)
        assert lines_disrupt[0]['disruption_id'] == 'disruption_on_line_A'
        assert lines_disrupt[0]['uri'] == 'too_bad_again'
        assert lines_disrupt[0]['severity']['name'] == 'bad severity'

        assert lines_disrupt[1]['disruption_id'] == 'disruption_on_line_A_but_later'
        assert lines_disrupt[1]['uri'] == 'later_impact'
        assert lines_disrupt[1]['severity']['name'] == 'information severity'

        stops = self.query_region('stop_areas/stopA?' + default_date_filter)
        stop = get_not_null(stops, 'stop_areas')[0]
        is_valid_stop_area(stop)
        stop_disrupt = get_disruptions(stop, response)
        assert len(stop_disrupt) == 1
        for d in stop_disrupt:
            is_valid_disruption(d)
        assert stop_disrupt[0]['disruption_id'] == 'disruption_on_stop_A'
        assert stop_disrupt[0]['uri'] == 'too_bad'

    def test_disruption_with_stop_areas(self):
        """
        when calling the pt object stopA, we should get its disruptions
        """

        response = self.query_region('stop_areas/stopA?' + default_date_filter)

        stops = get_not_null(response, 'stop_areas')
        assert len(stops) == 1
        stop = stops[0]

        disruptions = get_disruptions(stop, response)
        assert len(disruptions) == 1
        is_valid_disruption(disruptions[0])

    def is_in_disprution_time(self, stop_time, disruption_time):
        """
        :param stop_time: dict of departure/arrival stop_date_time
        :param disruption_time: dict of disruption application_periods
        :return: True if stop_time is within disruption time, else False
        """
        return (
            disruption_time['begin'] < stop_time['arrival_date_time'] < disruption_time['end']
            or disruption_time['begin'] < stop_time['departure_date_time'] < disruption_time['end']
        )

    def test_disruption_with_departures(self):
        """
        on a '/departure' call on stopB, we should get its disruptions
        """
        response = self.query_region(
            'stop_points/stop_point:stopB/departures?'
            'from_datetime=20120614T080000&_current_datetime=20120614T080000'
        )

        departures = get_not_null(response, 'departures')
        assert len(departures) >= 2

        disruptions = get_not_null(response, 'disruptions')
        assert disruptions
        assert len(disruptions) >= 1
        for d in disruptions:
            is_valid_disruption(d)

        # Every lines impacted by the disruption
        disruption_lines = jmespath.search('impacted_objects[].pt_object.line.name', disruptions[0])
        # Every lines departing from the stop point
        departures_lines = jmespath.search('[].route.line.name', departures)

        # Line not impacted shouldn't return a disruption
        not_impacted_lines = set(departures_lines) - set(disruption_lines)
        for line in not_impacted_lines:
            not_impacted_departure = jmespath.search("[?route.line.name=='{}']".format(line), departures)
            departure_disruptions = get_all_element_disruptions(not_impacted_departure[0], response)
            assert not departure_disruptions

        impacted_lines = set(departures_lines) - set(not_impacted_lines)
        for l in impacted_lines:
            departure = jmespath.search("[?route.line.name=='{}']".format(l), departures)
            departure_disruptions = get_all_element_disruptions(departure, response)
            if self.is_in_disprution_time(
                departure[0]['stop_date_time'], disruptions[0]['application_periods'][0]
            ):
                assert departure_disruptions
                assert len(departure_disruptions) == 1
                assert 'too_bad_all_lines' in departure_disruptions
                is_valid_disruption(departure_disruptions['too_bad_all_lines'][0].disruption)
            else:
                assert not departure_disruptions

    def test_disruption_with_arrival(self):
        """
        on a '/arrivals' call on stopA, we should get its disruptions
        """
        response = self.query_region(
            'stop_points/stop_point:stopA/arrivals?'
            'from_datetime=20120614T070000&_current_datetime=20120614T080000'
        )

        arrivals = get_not_null(response, 'arrivals')
        assert len(arrivals) >= 2

        disruptions = get_not_null(response, 'disruptions')
        assert disruptions
        assert len(disruptions) >= 1
        for d in disruptions:
            is_valid_disruption(d)

        # Every lines impacted by the disruption
        disruption_lines = jmespath.search('impacted_objects[].pt_object.line.name', disruptions[0])
        # Every lines arriving to the stop point
        arrivals_lines = jmespath.search('[].route.line.name', arrivals)

        # Line not impacted shouldn't return a disruption
        not_impacted_lines = set(arrivals_lines) - set(disruption_lines)
        for line in not_impacted_lines:
            not_impacted_departure = jmespath.search("[?route.line.name=='{}']".format(line), arrivals)
            departure_disruptions = get_all_element_disruptions(not_impacted_departure[0], response)
            assert not departure_disruptions

        impacted_lines = set(arrivals_lines) - set(not_impacted_lines)
        for l in impacted_lines:
            departure = jmespath.search("[?route.line.name=='{}']".format(l), arrivals)
            departure_disruptions = get_all_element_disruptions(departure, response)
            if self.is_in_disprution_time(
                departure[0]['stop_date_time'], disruptions[0]['application_periods'][0]
            ):
                assert departure_disruptions
                assert len(departure_disruptions) == 1
                assert 'too_bad_all_lines' in departure_disruptions
                is_valid_disruption(departure_disruptions['too_bad_all_lines'][0].disruption)
            else:
                assert not departure_disruptions

    def test_direct_disruption_call(self):
        """
        when calling the disruptions on the pt object stopB,
        we should get the disruption summary filtered on the stopB

        so
        we get the impacts on the line A (it goes through stopB),
        and the impacts on the network
        """

        response = self.query_region('stop_areas/stopB/traffic_reports?' + default_date_filter)

        traffic_report = get_not_null(response, 'traffic_reports')
        assert len(traffic_report) == 1

        impacted_lines = get_not_null(traffic_report[0], 'lines')
        assert len(impacted_lines) == 1
        is_valid_line(impacted_lines[0], depth_check=0)
        assert impacted_lines[0]['id'] == 'A'

        lines_disrupt = get_disruptions(impacted_lines[0], response)
        assert len(lines_disrupt) == 2
        assert lines_disrupt[0]['disruption_id'] == 'disruption_on_line_A'
        assert lines_disrupt[0]['uri'] == 'too_bad_again'

        impacted_network = get_not_null(traffic_report[0], 'network')

        is_valid_network(impacted_network, depth_check=0)
        assert impacted_network['id'] == 'base_network'
        network_disrupt = get_disruptions(impacted_network, response)
        assert len(network_disrupt) == 2
        assert network_disrupt[0]['disruption_id'] == 'disruption_on_line_A'
        assert network_disrupt[0]['uri'] == 'too_bad_again'

        # but we should not have disruption on stop area, B is not disrupted
        assert 'stop_areas' not in traffic_report[0]

    def test_no_disruption(self):
        """
        when calling the pt object stopB, we should get no disruptions
        """

        response = self.query_region('stop_areas/stopB')

        stops = get_not_null(response, 'stop_areas')
        assert len(stops) == 1
        stop = stops[0]

        assert 'disruptions' not in stop

    def test_disruption_publication_date_filter(self):
        """
        test the publication date filter

        'disruption_on_line_A_but_publish_later' is published from 2012-08-28 10:00

        so at 9 it is not in the list, at 11, we get it
        """
        response = self.query_region('traffic_reports?_current_datetime=20120828T090000')

        impacts = get_impacts(response)
        assert len(impacts) == 4
        assert 'impact_published_later' not in impacts

        response = self.query_region('traffic_reports?_current_datetime=20120828T130000')

        impacts = get_impacts(response)
        assert len(impacts) == 5
        assert 'impact_published_later' in impacts

    def test_disruption_publication_date_filter_many_application_patterns(self):
        """
        test the publication date filter

        'disruption_on_line_A_but_publish_later' is published from 2012-08-28 10:00

        so at 9 it is not in the list, at 11, we get it
        """
        response = self.query_region('traffic_reports?_current_datetime=20120810T090000')

        impacts = get_impacts(response)
        assert len(impacts) == 5
        assert 'impact_k' in impacts
        impact_k = impacts["impact_k"]

        assert impact_k['disruption_id'] == 'disruption_on_line_K'
        is_valid_disruption(impact_k)
        assert len(impact_k["application_periods"]) == 14
        application_patterns = impact_k["application_patterns"]
        assert len(application_patterns) == 2
        time_slots = application_patterns[0]["time_slots"]
        assert len(time_slots) == 2
        assert time_slots[0]["begin"] == '081500'
        assert time_slots[0]["end"] == '093000'
        assert time_slots[1]["begin"] == '121000'
        assert time_slots[1]["end"] == '133000'
        assert application_patterns[0]["application_period"]["begin"] == "20120806"
        assert application_patterns[0]["application_period"]["end"] == '20120812'
        assert application_patterns[0]["week_pattern"]['monday']
        assert application_patterns[0]["week_pattern"]['tuesday']
        assert application_patterns[0]["week_pattern"]['friday']
        assert not application_patterns[0]["week_pattern"]['wednesday']
        assert not application_patterns[0]["week_pattern"]['thursday']
        assert not application_patterns[0]["week_pattern"]['sunday']
        assert application_patterns[0]["week_pattern"]['saturday']

        time_slots = application_patterns[1]["time_slots"]
        assert len(time_slots) == 2
        assert time_slots[0]["begin"] == '111500'
        assert time_slots[0]["end"] == '133500'
        assert time_slots[1]["begin"] == '171000'
        assert time_slots[1]["end"] == '184500'

        assert application_patterns[1]["application_period"]["begin"] == '20120820'
        assert application_patterns[1]["application_period"]["end"] == '20120826'
        assert not application_patterns[1]["week_pattern"]['monday']
        assert application_patterns[1]["week_pattern"]['tuesday']
        assert not application_patterns[1]["week_pattern"]['friday']
        assert application_patterns[1]["week_pattern"]['wednesday']
        assert application_patterns[1]["week_pattern"]['thursday']
        assert not application_patterns[1]["week_pattern"]['sunday']
        assert not application_patterns[1]["week_pattern"]['saturday']

    def test_disruption_datefilter_limits(self):
        """
        the _current_datetime is by default in UTC, we test that this is correctly taken into account
        disruption_on_line_A is applied from 20120801T000000 to 20120901T115959
        """
        response = self.query_region('stop_areas/stopB/traffic_reports?_current_datetime=20120801T000000Z')

        # we should have disruptions at this date
        assert len(response['disruptions']) > 0

        response = self.query_region('stop_areas/stopB/traffic_reports?_current_datetime=20120731T235959Z')

        # before the start of the period => no disruptions
        assert len(response['disruptions']) == 0

        # %2B is the code for '+'
        response = self.query_region('stop_areas/stopB/traffic_reports?_current_datetime=20120801T005959%2B0100')

        # UTC + 1, so in UTC it's 23h59 the day before => no disruption
        assert len(response['disruptions']) == 0

        response = self.query_region('stop_areas/stopB/traffic_reports?_current_datetime=20120801T000000-0100')

        # UTC - 1, so in UTC it's 01h => disruptions
        assert len(response['disruptions']) > 0

        response = self.query_region('stop_areas/stopB/traffic_reports?_current_datetime=20120731T235959-0100')

        # UTC - 1, so in UTC it's 00h59 => disruptions
        assert len(response['disruptions']) > 0

    def test_disruption_on_route(self):
        """
        check that disruption on route are displayed on the corresponding line
        """
        response = self.query_region('traffic_reports?_current_datetime=20130225T090000')

        impacts = get_impacts(response)
        assert len(impacts) == 1

        response = self.query_region('traffic_reports?_current_datetime=20130227T090000')

        impacts = get_impacts(response)
        assert len(impacts) == 2
        assert 'too_bad_route_A:0' in impacts

        traffic_report = get_not_null(response, 'traffic_reports')
        assert len(traffic_report) == 1

        impacted_lines = get_not_null(traffic_report[0], 'lines')
        assert len(impacted_lines) == 1
        is_valid_line(impacted_lines[0], depth_check=0)
        assert impacted_lines[0]['id'] == 'A'

        lines_disrupt = get_disruptions(impacted_lines[0], response)
        assert len(lines_disrupt) == 1
        for d in lines_disrupt:
            is_valid_disruption(d)
        assert lines_disrupt[0]['disruption_id'] == 'disruption_route_A:0'
        assert lines_disrupt[0]['uri'] == 'too_bad_route_A:0'

        # Check message, channel and types
        disruption_message = get_not_null(response, 'disruptions')
        assert len(disruption_message) == 2
        disruption = get_disruption(disruption_message, 'too_bad_route_A:0')
        message = get_not_null(disruption, 'messages')
        assert message[0]['text'] == 'no luck'
        channel = get_not_null(message[0], 'channel')
        assert channel['id'] == 'sms'
        assert channel['name'] == 'sms'
        channel_types = channel['types']
        assert len(channel_types) == 2
        assert channel_types[0] == 'web'
        assert channel_types[1] == 'sms'

        assert message[1]['text'] == 'try again'
        channel = get_not_null(message[1], 'channel')
        assert channel['id'] == 'email'
        assert channel['name'] == 'email'
        channel_types = channel['types']
        assert len(channel_types) == 2
        assert channel_types[0] == 'web'
        assert channel_types[1] == 'email'

    def test_disruption_on_route_and_line(self):
        """
        and we check the sort order of the lines
        """
        response = self.query_region('traffic_reports?_current_datetime=20130425T090000')

        impacts = get_impacts(response)
        assert len(impacts) == 1

        response = self.query_region('traffic_reports?_current_datetime=20130427T090000')

        impacts = get_impacts(response)
        assert len(impacts) == 4
        assert 'too_bad_route_A:0_and_line' in impacts

        traffic_report = get_not_null(response, 'traffic_reports')
        assert len(traffic_report) == 1

        impacted_lines = get_not_null(traffic_report[0], 'lines')
        assert len(impacted_lines) == 3
        is_valid_line(impacted_lines[0], depth_check=0)
        assert impacted_lines[0]['id'] == 'B'
        assert impacted_lines[1]['id'] == 'A'
        assert impacted_lines[2]['id'] == 'C'

        lines_disrupt = get_disruptions(impacted_lines[1], response)
        assert len(lines_disrupt) == 1
        for d in lines_disrupt:
            is_valid_disruption(d)
        assert lines_disrupt[0]['disruption_id'] == 'disruption_route_A:0_and_line'
        assert lines_disrupt[0]['uri'] == 'too_bad_route_A:0_and_line'

        # Verify message and channel without any channel type
        message = get_not_null(lines_disrupt[0], 'messages')
        assert len(message) == 3
        assert message[0]['text'] == 'no luck'
        channel = get_not_null(message[0], 'channel')
        assert channel['id'] == 'sms'
        assert channel['name'] == 'sms channel'
        assert len(channel['types']) == 1

        assert message[1]['text'] == 'try again'
        channel = get_not_null(message[1], 'channel')
        assert channel['id'] == 'sms'
        assert channel['name'] == 'sms channel'
        assert len(channel['types']) == 1

        assert message[2]['text'] == 'beacon in channel'
        channel = get_not_null(message[2], 'channel')
        assert channel['id'] == 'beacon'
        assert channel['name'] == 'beacon channel'
        assert len(channel['types']) == 3

    def test_disruption_date_filtering(self):
        """
        test the filtering of the /disruptions API with since/until

        The ease the test we consider only the A line disruptions
        """
        response = self.query_region('lines/A/disruptions')

        disruptions = response['disruptions']
        for d in disruptions:
            is_valid_disruption(d)

        # No filtering results in the whole 5 disruptions
        assert len(disruptions) == 5

        # in the disruptoins the earliest one is called 'too_bad_all_lines'
        # and is active on [20120614T060000, 20120614T120000] in 'main_routing_test'
        too_bad_all_line = next(d for d in disruptions if d['id'] == 'too_bad_all_lines')
        assert too_bad_all_line['application_periods'][0]['begin'] == '20120614T060000'
        assert too_bad_all_line['application_periods'][0]['end'] == '20120614T120000'

        # if we ask the all the disruption valid until 20120615, we only got 'too_bad_all_lines'
        disrup = self.query_region('lines/A/disruptions?until=20120615T000000')['disruptions']
        assert {d['id'] for d in disrup} == {'too_bad_all_lines'}

        # same if we ask until a date in too_bad_all_line application_periods
        # filter condition: period.begin < until
        disrup = self.query_region('lines/A/disruptions?until=20120614T105959')['disruptions']
        assert {d['id'] for d in disrup} == {'too_bad_all_lines'}

        # same if we ask for the very beginning of the disruption
        # filter condition: period.begin <= until
        disrup = self.query_region('lines/A/disruptions?until=20120614T060000')['disruptions']
        assert {d['id'] for d in disrup} == {'too_bad_all_lines'}

        # but the very second before, we got nothing
        # filter condition: period.begin <= until
        _, code = self.query_region('lines/A/disruptions?until=20120614T055959', check=False)
        assert code == 404

        # if we ask for all disruption valid since 20120614T115959 (the end of too_bad_all_line),
        # we got all the disruptions (filter condition: period.end > since)
        disrup = self.query_region('lines/A/disruptions?since=20120614T115959')['disruptions']
        assert len(disrup) == 5

        # if we ask for all disruption valid since 20120614T120000, we got all of them but 'too_bad_all_lines'
        # filter condition: period.end > since
        disrup = self.query_region('lines/A/disruptions?since=20120614T120000')['disruptions']
        assert len(disrup) == 4 and not any(d for d in disrup if d['id'] == 'too_bad_all_lines')

    def test_disruption_period_filtering(self):
        """
        there are 2 late disruptions 'later_impact' and 'too_bad_route_A:0_and_line'
        'later_impact' is interesting because it has several application periods:
         * [20121001T000000, 20121015T115959]
         * [20121201T000000, 20121215T115959]
        """
        # we query it with a period that intersect one of 'later_impact periods, we got the impact
        disrup = self.query_region('lines/A/disruptions?since=20121016T000000&until=20121217T000000')[
            'disruptions'
        ]
        assert {d['id'] for d in disrup} == {'later_impact'}

        # we query it with a period that doest not intersect one of 'later_impact periods, we got nothing
        _, code = self.query_region(
            'lines/A/disruptions?since=20121016T000000&until=20121017T000000', check=False
        )
        assert code == 404

        # we query it with a period [d, d] that intersect one of 'later_impact periods, we got the impact
        disrup = self.query_region('lines/A/disruptions?since=20121001T000000&until=20121001T000000')[
            'disruptions'
        ]
        assert {d['id'] for d in disrup} == {'later_impact'}

    def test_forbidden_uris_on_disruptions(self):
        """test forbidden uri for disruptions"""
        response, code = self.query_no_assert("v1/coverage/main_routing_test/disruptions")
        assert code == 200
        disruptions = get_not_null(response, 'disruptions')
        assert len(disruptions) == 12

        # filtering disruptions on line A
        response, code = self.query_no_assert("v1/coverage/main_routing_test/disruptions?forbidden_uris[]=A")
        assert code == 200
        disruptions = get_not_null(response, 'disruptions')
        assert len(disruptions) == 7

        # for retrocompatibility purpose forbidden_id[] is the same
        response, code = self.query_no_assert("v1/coverage/main_routing_test/disruptions?forbidden_id[]=A")
        assert code == 200
        disruptions = get_not_null(response, 'disruptions')
        assert len(disruptions) == 7

        # when we forbid another id, we find again all our disruptions
        response, code = self.query_no_assert(
            "v1/coverage/main_routing_test/disruptions?forbidden_id[]=bloubliblou"
        )
        assert code == 200
        disruptions = get_not_null(response, 'disruptions')
        assert len(disruptions) == 12

    def test_line_reports(self):
        response = self.query_region("line_reports?_current_datetime=20120801T000000")
        warnings = get_not_null(response, 'warnings')
        assert len(warnings) == 1
        assert warnings[0]['id'] == 'beta_endpoint'
        assert len(get_not_null(response, 'disruptions')) == 4
        line_reports = get_not_null(response, 'line_reports')
        for line_report in line_reports:
            is_valid_line_report(line_report)
        assert len(line_reports) == 7
        assert line_reports[0]['line']['id'] == 'A'
        assert len(line_reports[0]['pt_objects']) == 3
        assert line_reports[0]['pt_objects'][0]['id'] == 'A'
        assert len(line_reports[0]['pt_objects'][0]['line']['links']) == 2
        assert line_reports[0]['pt_objects'][0]['line']['links'][0]['id'] == 'too_bad_again'
        assert line_reports[0]['pt_objects'][0]['line']['links'][1]['id'] == 'later_impact'
        assert line_reports[0]['pt_objects'][1]['id'] == 'base_network'
        assert len(line_reports[0]['pt_objects'][1]['network']['links']) == 2
        assert line_reports[0]['pt_objects'][1]['network']['links'][0]['id'] == 'too_bad_again'
        assert line_reports[0]['pt_objects'][1]['network']['links'][1]['id'] == 'later_impact'
        assert line_reports[0]['pt_objects'][2]['id'] == 'stopA'

        for line_report in line_reports[2:]:
            assert len(line_report['pt_objects']) == 2
            assert line_report['pt_objects'][0]['id'] == 'base_network'
            assert line_report['pt_objects'][1]['id'] == 'stopA'

    def test_line_reports_with_since_and_until(self):
        response = self.query_region(
            "line_reports?_current_datetime=20120801T000000" "&since=20120801T000000&until=20120803T000000"
        )
        warnings = get_not_null(response, 'warnings')
        assert len(warnings) == 1
        assert warnings[0]['id'] == 'beta_endpoint'
        assert len(get_not_null(response, 'disruptions')) == 2
        line_reports = get_not_null(response, 'line_reports')
        for line_report in line_reports:
            is_valid_line_report(line_report)
        assert len(line_reports) == 7
        assert line_reports[0]['line']['id'] == 'A'
        assert len(line_reports[0]['pt_objects']) == 3
        assert line_reports[0]['pt_objects'][0]['id'] == 'A'
        assert len(line_reports[0]['pt_objects'][0]['line']['links']) == 1
        assert line_reports[0]['pt_objects'][0]['line']['links'][0]['id'] == 'too_bad_again'
        assert line_reports[0]['pt_objects'][1]['id'] == 'base_network'
        assert len(line_reports[0]['pt_objects'][1]['network']['links']) == 1
        assert line_reports[0]['pt_objects'][1]['network']['links'][0]['id'] == 'too_bad_again'
        assert line_reports[0]['pt_objects'][2]['id'] == 'stopA'

        for line_report in line_reports[2:]:
            assert len(line_report['pt_objects']) == 2
            assert line_report['pt_objects'][0]['id'] == 'base_network'
            assert line_report['pt_objects'][1]['id'] == 'stopA'

        for line_report in line_reports[2:]:
            assert len(line_report['pt_objects']) == 2
            assert line_report['pt_objects'][0]['id'] == 'base_network'
            assert line_report['pt_objects'][1]['id'] == 'stopA'

    def test_line_reports_with_forbidden_uris(self):
        response = self.query_region("line_reports?_current_datetime=20120801T000000")
        line_reports = get_not_null(response, 'line_reports')
        for line_report in line_reports:
            is_valid_line_report(line_report)
        # 6 lines affected by disruptions in this response: A,B,C,D,M,PM
        assert len(line_reports) == 7

        response = self.query_region("line_reports?_current_datetime=20120801T000000&forbidden_uris[]=M")
        line_reports = get_not_null(response, 'line_reports')
        for line_report in line_reports:
            is_valid_line_report(line_report)
        # Line M is now forbidden
        # TODO: investigate behavior -> https://jira.kisio.org/browse/NAVP-1365

    def test_disruption_with_stop_areas_and_different_parameters(self):
        """
        Data production period: 2012/06/14 - 2013/06/14
        One disruption on 'stopA' with publication and application period : 20120801T000000 - 20120901T120000
        One disruption on 'stopA' with publication period : 20120801T000000 - 20130801T120000 and
        application period : 20130701T120000 - 20130801T120000

        All the disruptions with publication_period intersecting with data production_period are valid
        and charged by kraken.

        Queries possible to return disruptions are as followings (_current_datetime used to go to the past.)
        1. stop_areas/<stop_area_id>?_current_datetime=20120801T000000
        2. stop_areas/<stop_area_id>/disruptions?_current_datetime=20120801T000000
        3. stop_areas/<stop_area_id>/disruptions?_current_datetime=20120801T000000&since=20120801T000000
        4. stop_areas/<stop_area_id>/disruptions?_current_datetime=20120801T000000&since=20120801T000000
        &until=20120901T000000
        """
        curr_date_filter = '_current_datetime=20120801T000000'

        response = self.query_region('stop_areas/stopA')

        stops = get_not_null(response, 'stop_areas')
        assert len(stops) == 1
        stop = stops[0]

        disruptions = get_disruptions(stop, response)
        assert len(disruptions) == 0

        # api pt_ref sends all the disruptions on pt_object with data production_period intersecting application_period
        # and active for the date passed in parameter(&_current_datetime=20120801T000000)
        response = self.query_region('stop_areas/stopA?' + curr_date_filter)
        stops = get_not_null(response, 'stop_areas')
        disruptions = get_disruptions(stops[0], response)
        assert len(disruptions) == 1
        is_valid_disruption(disruptions[0])

        # api /disruptions on pt_ref sends all the disruptions on pt_objects
        response = self.query_region('stop_areas/stopA/disruptions?' + curr_date_filter)
        disruptions = response['disruptions']
        for d in disruptions:
            is_valid_disruption(d)
        assert len(disruptions) == 2

        # api /disruptions on pt_ref with parameters &since sends all the disruptions on pt_objects with filter
        # on since and until(=positive infinite value) with search period intersecting application_period
        response = self.query_region('stop_areas/stopA/disruptions?since=20120801T000000&' + curr_date_filter)
        disruptions = response['disruptions']
        for d in disruptions:
            is_valid_disruption(d)
        assert len(disruptions) == 2

        # api /disruptions on pt_ref with parameters &since and &until sends all the disruptions on pt_objects
        # with filter on since and until with search period intersecting application_period
        response = self.query_region(
            'stop_areas/stopA/disruptions?since=20120801T000000&until=20120902T000000&' + curr_date_filter
        )
        disruptions = response['disruptions']
        assert len(disruptions) == 1

        # gives the disruption in future out of data poduction period
        response = self.query_region(
            'stop_areas/stopA/disruptions?since=20130703T000000&until=20130801T000000&' + curr_date_filter
        )
        disruptions = response['disruptions']
        assert len(disruptions) == 1

        response = self.query_region(
            'stop_areas/stopA/disruptions?since=20120801T000000&until=20130801T000000&' + curr_date_filter
        )
        disruptions = response['disruptions']
        assert len(disruptions) == 2

    def test_traffic_report_with_since_until(self):
        response = self.query_region(
            'traffic_reports?since=20120801T000000&until=20121002T000000&_current_datetime=20120801T000000'
        )

        impacts = get_impacts(response)
        assert len(impacts) == 3
        for impact_id in list(impacts.keys()):
            assert impact_id in ['too_bad', 'later_impact', 'too_bad_again']

    def test_traffic_report_without_since(self):
        response = self.query_region('traffic_reports?until=20121002T000000&_current_datetime=20120801T000000')

        impacts = get_impacts(response)
        assert len(impacts) == 3
        for impact_id in list(impacts.keys()):
            assert impact_id in ['too_bad', 'later_impact', 'too_bad_again']

    def test_traffic_report_without_until(self):
        response = self.query_region('traffic_reports?since=20120801T000000&_current_datetime=20120801T000000')

        impacts = get_impacts(response)
        assert len(impacts) == 4
        for impact_id in list(impacts.keys()):
            assert impact_id in ['too_bad_future', 'too_bad', 'later_impact', 'too_bad_again']

    def test_traffic_report_since_gt_until(self):
        response, code = self.query_region(
            'traffic_reports?since=20121002T000000&until=20120801T000000&_current_datetime=20120801T000000',
            check=False,
        )

        assert code == 400
        assert response["message"] == u'until must be >= since'


@dataset({"line_sections_test": {}})
class TestDisruptionsLineSections(AbstractTestFixture):
    def _check_line_reports_response(self, response):
        line_reports = get_not_null(response, 'line_reports')
        assert len(line_reports) == 2
        # line:1
        line_report = line_reports[0]
        is_valid_line_report(line_report)
        line = line_report['line']
        assert line['id'] == 'line:1'
        assert len(line['links']) == 2
        assert line['links'][0]['id'] == 'line_section_on_line_1'
        assert line['links'][1]['id'] == 'line_section_on_line_1_other_effect'
        # line:1 associated pt_objects
        pt_objects = line_report['pt_objects']
        assert len(pt_objects) == 5
        # C_1
        assert pt_objects[0]['id'] == 'C_1'
        assert [l['id'] for l in pt_objects[0][pt_objects[0]['embedded_type']]['links']] == [
            'line_section_on_line_1'
        ]
        # D_1
        assert pt_objects[1]['id'] == 'D_1'
        assert [l['id'] for l in pt_objects[1][pt_objects[1]['embedded_type']]['links']] == [
            'line_section_on_line_1'
        ]
        # D_3
        assert pt_objects[2]['id'] == 'D_3'
        assert [l['id'] for l in pt_objects[2][pt_objects[2]['embedded_type']]['links']] == [
            'line_section_on_line_1'
        ]
        # E_1
        assert pt_objects[3]['id'] == 'E_1'
        assert [l['id'] for l in pt_objects[3][pt_objects[3]['embedded_type']]['links']] == [
            'line_section_on_line_1',
            'line_section_on_line_1_other_effect',
        ]
        # F_1
        assert pt_objects[4]['id'] == 'F_1'
        assert [l['id'] for l in pt_objects[4][pt_objects[4]['embedded_type']]['links']] == [
            'line_section_on_line_1_other_effect'
        ]

        # line:2
        line_report = line_reports[1]
        is_valid_line_report(line_report)
        line = line_report['line']
        assert line['id'] == 'line:2'
        assert len(line['links']) == 1
        assert line['links'][0]['id'] == 'line_section_on_line_2'
        # line:2 associated pt_objects
        pt_objects = line_report['pt_objects']
        assert len(pt_objects) == 2
        # B_1
        assert pt_objects[0]['id'] == 'B_1'
        assert [l['id'] for l in pt_objects[0][pt_objects[0]['embedded_type']]['links']] == [
            'line_section_on_line_2'
        ]
        # F_1
        assert pt_objects[1]['id'] == 'F_1'
        assert [l['id'] for l in pt_objects[1][pt_objects[1]['embedded_type']]['links']] == [
            'line_section_on_line_2'
        ]

        # Associated disruptions
        disruptions = get_not_null(response, 'disruptions')
        assert len(disruptions) == 3
        # disruption : line_section_on_line_1
        disruption = disruptions[0]
        assert disruption['id'] == 'line_section_on_line_1'
        assert len(disruption['impacted_objects']) == 1
        impacted_object = disruption['impacted_objects']
        assert impacted_object[0]['pt_object']['id'] == 'line:1'
        assert impacted_object[0]['impacted_section']['from']['id'] == 'C'
        assert impacted_object[0]['impacted_section']['to']['id'] == 'E'
        assert [r['id'] for r in impacted_object[0]['impacted_section']['routes']] == [
            'route:line:1:1',
            'route:line:1:3',
        ]
        # disruption : line_section_on_line_1_other_effect
        disruption = disruptions[1]
        assert disruption['id'] == 'line_section_on_line_1_other_effect'
        assert len(disruption['impacted_objects']) == 2
        impacted_object = disruption['impacted_objects']
        # E -> F
        assert impacted_object[0]['pt_object']['id'] == 'line:1'
        assert impacted_object[0]['impacted_section']['from']['id'] == 'E'
        assert impacted_object[0]['impacted_section']['to']['id'] == 'F'
        assert [r['id'] for r in impacted_object[0]['impacted_section']['routes']] == [
            'route:line:1:1',
            'route:line:1:3',
        ]
        # F -> E
        assert impacted_object[1]['pt_object']['id'] == 'line:1'
        assert impacted_object[1]['impacted_section']['from']['id'] == 'F'
        assert impacted_object[1]['impacted_section']['to']['id'] == 'E'
        assert [r['id'] for r in impacted_object[0]['impacted_section']['routes']] == [
            'route:line:1:1',
            'route:line:1:3',
        ]
        # disruption : line_section_on_line_2
        disruption = disruptions[2]
        assert disruption['id'] == 'line_section_on_line_2'
        assert len(disruption['impacted_objects']) == 1
        impacted_object = disruption['impacted_objects']
        assert impacted_object[0]['pt_object']['id'] == 'line:2'
        assert impacted_object[0]['impacted_section']['from']['id'] == 'B'
        assert impacted_object[0]['impacted_section']['to']['id'] == 'F'
        assert [r['id'] for r in impacted_object[0]['impacted_section']['routes']] == ['route:line:2:1']

    # Information about the data in line_section_test
    # The data is valid from "20170101T000000" to "20170131T000000"
    # The disruption has publication_period from "20170101T000000" to "20170110T000000"
    # and one application_period from "20170102T000000" to "20170105T000000"
    def test_line_reports(self):
        response = self.query_region("line_reports?_current_datetime=20170103T120000")
        self._check_line_reports_response(response)

    def test_line_reports_with_current_datetime_outof_application_period(self):
        # without since/until we use since=production_date.begin and until = production_date.end
        # application period intersects with active period of the disruption but not publication period
        response = self.query_region("line_reports?_current_datetime=20170111T130000")
        assert len(response['disruptions']) == 0

    def test_line_reports_with_since_until_intersects_application_period(self):
        response = self.query_region(
            "line_reports?_current_datetime=20170101T120000" "&since=20170104T130000&until=20170106T000000"
        )
        self._check_line_reports_response(response)

    def test_line_reports_with_since_intersects_application_period(self):
        response = self.query_region("line_reports?_current_datetime=20170101T120000&since=20170104T130000")
        self._check_line_reports_response(response)

    def test_line_reports_with_since_until_outof_application_period(self):
        response = self.query_region(
            "line_reports?_current_datetime=20170101T120000" "&since=20170105T130000&until=20170108T000000"
        )
        assert len(response['disruptions']) == 0

    def test_line_reports_with_since_until_value_not_valid(self):
        response, code = self.query_region(
            "line_reports?_current_datetime=20170101T120000" "&since=20170108T130000&until=20170105T000000",
            check=False,
        )
        assert code == 404
        assert response['error']['message'] == 'invalid filtering period (since > until)'

    def test_line_reports_with_uri_filter(self):
        # Without filter
        response = self.query_region("line_reports?_current_datetime=20170101T120000")
        self._check_line_reports_response(response)

        # With lines=line:1 filter
        response = self.query_region("lines/line:1/line_reports?_current_datetime=20170101T120000")
        line_reports = get_not_null(response, 'line_reports')
        assert len(line_reports) == 1
        assert line_reports[0]['line']['id'] == 'line:1'
        assert len(line_reports[0]['line']['links']) == 2
        assert line_reports[0]['line']['links'][0]['id'] == 'line_section_on_line_1'
        assert line_reports[0]['line']['links'][1]['id'] == 'line_section_on_line_1_other_effect'
        for o in line_reports[0]['pt_objects']:
            links = o[o['embedded_type']]['links']
            assert links
            for l in links:
                assert l['id'] != 'line_section_on_line_2'
        # Associated disruptions
        disruptions = get_not_null(response, 'disruptions')
        assert len(disruptions) == 2
        assert disruptions[0]['id'] == 'line_section_on_line_1'
        assert disruptions[1]['id'] == 'line_section_on_line_1_other_effect'

        # With routes=route:line:2:1 filter
        response = self.query_region("routes/route:line:2:1/line_reports?_current_datetime=20170101T120000")
        line_reports = get_not_null(response, 'line_reports')
        assert len(line_reports) == 1
        assert line_reports[0]['line']['id'] == 'line:2'
        assert len(line_reports[0]['line']['links']) == 1
        assert line_reports[0]['line']['links'][0]['id'] == 'line_section_on_line_2'
        # The associated disruption
        disruptions = get_not_null(response, 'disruptions')
        assert len(disruptions) == 1
        assert disruptions[0]['id'] == 'line_section_on_line_2'

        # With wrong filter
        response, code = self.query_region(
            "lines/wrong_uri/line_reports?_current_datetime=20170101T120000", check=False
        )
        assert code == 404
        assert response['error']['message'] == 'ptref : Filters: Unable to find object'
