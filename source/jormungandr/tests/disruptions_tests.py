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
            assert len(stop_disrupt) == 1
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
            'coords/{s}/traffic_reports?{dt_filter}'.format(s=s_coord, dt_filter=default_date_filter))

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
        :return: True if stop_time is within disruption time, else None
        """
        if disruption_time['begin'] < stop_time['arrival_date_time'] < disruption_time['end'] or \
                disruption_time['begin'] < stop_time['departure_date_time'] < disruption_time['end']:
            return True

    def test_disruption_with_departures(self):
        """
        on a '/departure' call on stopB, we should get its disruptions
        """
        response = self.query_region('stop_points/stop_point:stopB/departures?'
                                     'from_datetime=20120614T080000&_current_datetime=20120614T080000')

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
        not_impacted_lines = set(departures_lines)-set(disruption_lines)
        for line in not_impacted_lines:
            not_impacted_departure = jmespath.search("[?route.line.name=='{}']".format(line), departures)
            departure_disruptions = get_all_element_disruptions(not_impacted_departure[0], response)
            assert not departure_disruptions

        impacted_lines = set(departures_lines) - set(not_impacted_lines)
        for l in impacted_lines:
            departure = jmespath.search("[?route.line.name=='{}']".format(l), departures)
            departure_disruptions = get_all_element_disruptions(departure, response)
            if self.is_in_disprution_time(departure[0]['stop_date_time'], disruptions[0]['application_periods'][0]):
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
        response = self.query_region('stop_points/stop_point:stopA/arrivals?'
                                     'from_datetime=20120614T070000&_current_datetime=20120614T080000')

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
            if self.is_in_disprution_time(departure[0]['stop_date_time'], disruptions[0]['application_periods'][0]):
                assert departure_disruptions
                assert len(departure_disruptions) == 1
                assert 'too_bad_all_lines' in departure_disruptions
                is_valid_disruption(departure_disruptions['too_bad_all_lines'][0].disruption)
            else:
                assert not departure_disruptions




        # arrival = arrivals[0]
        # disruptions = get_all_element_disruptions(arrival, response)
        # assert disruptions
        # assert len(disruptions) == 1
        # assert 'too_bad_all_lines' in disruptions
        # is_valid_disruption(disruptions['too_bad_all_lines'][0].disruption)
        #
        # # This line isn't in the impacted_objects list
        # arrival = arrivals[1]
        # disruptions = get_all_element_disruptions(arrival, response)
        # assert not disruptions
        #
        # # This line is in the impacted_objects list but arrival is after the time of disruption
        # arrival = arrivals[2]
        # disruptions = get_all_element_disruptions(arrival, response)
        # assert not disruptions


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
        assert len(impacts) == 3
        assert 'impact_published_later' not in impacts

        response = self.query_region('traffic_reports?_current_datetime=20120828T130000')

        impacts = get_impacts(response)
        assert len(impacts) == 4
        assert 'impact_published_later' in impacts

    def test_disruption_datefilter_limits(self):
        """
        the _current_datetime is by default in UTC, we test that this is correctly taken into account
        disruption_on_line_A is applied from 20120801T000000 to 20120901T115959
        """
        response = self.query_region('stop_areas/stopB/traffic_reports?_current_datetime=20120801T000000Z')

        #we should have disruptions at this date
        assert len(response['disruptions']) > 0

        response = self.query_region('stop_areas/stopB/traffic_reports?_current_datetime=20120731T235959Z')

        #before the start of the period => no disruptions
        assert len(response['disruptions']) == 0

        #%2B is the code for '+'
        response = self.query_region('stop_areas/stopB/traffic_reports?_current_datetime=20120801T005959%2B0100')

        #UTC + 1, so in UTC it's 23h59 the day before => no disruption
        assert len(response['disruptions']) == 0

        response = self.query_region('stop_areas/stopB/traffic_reports?_current_datetime=20120801T000000-0100')

        #UTC - 1, so in UTC it's 01h => disruptions
        assert len(response['disruptions']) > 0

        response = self.query_region('stop_areas/stopB/traffic_reports?_current_datetime=20120731T235959-0100')

        #UTC - 1, so in UTC it's 00h59 => disruptions
        assert len(response['disruptions']) > 0

    def test_disruption_on_route(self):
        """
        check that disruption on route are displayed on the corresponding line
        """
        response = self.query_region('traffic_reports?_current_datetime=20130225T090000')

        impacts = get_impacts(response)
        assert len(impacts) == 0

        response = self.query_region('traffic_reports?_current_datetime=20130227T090000')

        impacts = get_impacts(response)
        assert len(impacts) == 1
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

        #Check message, channel and types
        disruption_message = get_not_null(response, 'disruptions')
        assert len(disruption_message) == 1
        message = get_not_null(disruption_message[0], 'messages')
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
        assert len(impacts) == 0

        response = self.query_region('traffic_reports?_current_datetime=20130427T090000')

        impacts = get_impacts(response)
        assert len(impacts) == 3
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

        #Verify message and channel without any channel type
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

        #in the disruptoins the earliest one is called 'too_bad_all_lines'
        # and is active on [20120614T060000, 20120614T115959[
        too_bad_all_line = next(d for d in disruptions if d['id'] == 'too_bad_all_lines')
        assert too_bad_all_line['application_periods'][0]['begin'] == '20120614T060000'
        assert too_bad_all_line['application_periods'][0]['end'] == '20120614T115959'

        # if we ask the all the disruption valid until 20120615, we only got 'too_bad_all_lines'
        disrup = self.query_region('lines/A/disruptions?until=20120615T000000')['disruptions']
        assert {d['id'] for d in disrup} == {'too_bad_all_lines'}

        # same if we ask until a date in too_bad_all_line application_periods
        disrup = self.query_region('lines/A/disruptions?until=20120614T105959')['disruptions']
        assert {d['id'] for d in disrup} == {'too_bad_all_lines'}

        # same if we ask for the very beginning of the disruption
        disrup = self.query_region('lines/A/disruptions?until=20120614T060000')['disruptions']
        assert {d['id'] for d in disrup} == {'too_bad_all_lines'}

        # but the very second before, we got nothing
        _, code = self.query_region('lines/A/disruptions?until=20120614T055959', check=False)
        assert code == 404

        # if we ask for all disruption valid since 20120614T115959 (the end of too_bad_all_line),
        # we got all the disruptions
        disrup = self.query_region('lines/A/disruptions?since=20120614T115959')['disruptions']
        assert len(disrup) == 5

        # if we ask for all disruption valid since 20120614T120000 (the second after the end of
        # too_bad_all_line), we got all of them but 'too_bad_all_lines'
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
        disrup = self.query_region('lines/A/disruptions?since=20121016T000000&until=20121217T000000')['disruptions']
        assert {d['id'] for d in disrup} == {'later_impact'}

        # we query it with a period that doest not intersect one of 'later_impact periods, we got nothing
        _, code = self.query_region('lines/A/disruptions?since=20121016T000000&until=20121017T000000',
                                    check=False)
        assert code == 404

        # we query it with a period [d, d] that intersect one of 'later_impact periods, we got the impact
        disrup = self.query_region('lines/A/disruptions?since=20121001T000000&until=20121001T000000')['disruptions']
        assert {d['id'] for d in disrup} == {'later_impact'}

    def test_disruption_invalid_period_filtering(self):
        """if we query with a since or an until not in the production period, we got an error"""
        resp, code = self.query_region('disruptions?since=20201016T000000', check=False)
        assert code == 404
        assert resp['error']['message'] == 'ptref : invalid filtering period, not in production period'

        resp, code = self.query_region('disruptions?until=20001016T000000', check=False)
        assert code == 404
        assert resp['error']['message'] == 'ptref : invalid filtering period, not in production period'

    def test_forbidden_uris_on_disruptions(self):
        """test forbidden uri for disruptions"""
        response, code = self.query_no_assert("v1/coverage/main_routing_test/disruptions")
        assert code == 200
        disruptions = get_not_null(response, 'disruptions')
        assert len(disruptions) == 9

        #filtering disruptions on line A
        response, code = self.query_no_assert("v1/coverage/main_routing_test/disruptions?forbidden_uris[]=A")
        assert code == 200
        disruptions = get_not_null(response, 'disruptions')
        assert len(disruptions) == 4

        # for retrocompatibility purpose forbidden_id[] is the same
        response, code = self.query_no_assert("v1/coverage/main_routing_test/disruptions?forbidden_id[]=A")
        assert code == 200
        disruptions = get_not_null(response, 'disruptions')
        assert len(disruptions) == 4

        # when we forbid another id, we find again all our disruptions
        response, code = self.query_no_assert("v1/coverage/main_routing_test/disruptions?forbidden_id[]=bloubliblou")
        assert code == 200
        disruptions = get_not_null(response, 'disruptions')
        assert len(disruptions) == 9

    def test_line_reports(self):
        response = self.query_region("line_reports?_current_datetime=20120801T000000")
        warnings = get_not_null(response, 'warnings')
        assert len(warnings) == 1
        assert warnings[0]['id'] == 'beta_endpoint'
        assert len(get_not_null(response, 'disruptions')) == 3
        line_reports = get_not_null(response, 'line_reports')
        for line_report in line_reports:
            is_valid_line_report(line_report)
        assert len(line_reports) == 4
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

        for line_report in line_reports[1:]:
            assert len(line_report['pt_objects']) == 2
            assert line_report['pt_objects'][0]['id'] == 'base_network'
            assert line_report['pt_objects'][1]['id'] == 'stopA'

    def test_line_reports_with_since_and_until(self):
        response = self.query_region("line_reports?_current_datetime=20120801T000000"
                                     "&since=20120801T000000&until=20120803T000000")
        warnings = get_not_null(response, 'warnings')
        assert len(warnings) == 1
        assert warnings[0]['id'] == 'beta_endpoint'
        assert len(get_not_null(response, 'disruptions')) == 2
        line_reports = get_not_null(response, 'line_reports')
        for line_report in line_reports:
            is_valid_line_report(line_report)
        assert len(line_reports) == 4
        assert line_reports[0]['line']['id'] == 'A'
        assert len(line_reports[0]['pt_objects']) == 3
        assert line_reports[0]['pt_objects'][0]['id'] == 'A'
        assert len(line_reports[0]['pt_objects'][0]['line']['links']) == 1
        assert line_reports[0]['pt_objects'][0]['line']['links'][0]['id'] == 'too_bad_again'
        assert line_reports[0]['pt_objects'][1]['id'] == 'base_network'
        assert len(line_reports[0]['pt_objects'][1]['network']['links']) == 1
        assert line_reports[0]['pt_objects'][1]['network']['links'][0]['id'] == 'too_bad_again'
        assert line_reports[0]['pt_objects'][2]['id'] == 'stopA'

        for line_report in line_reports[1:]:
            assert len(line_report['pt_objects']) == 2
            assert line_report['pt_objects'][0]['id'] == 'base_network'
            assert line_report['pt_objects'][1]['id'] == 'stopA'

        for line_report in line_reports[1:]:
            assert len(line_report['pt_objects']) == 2
            assert line_report['pt_objects'][0]['id'] == 'base_network'
            assert line_report['pt_objects'][1]['id'] == 'stopA'


@dataset({"line_sections_test": {}})
class TestDisruptionsLineSections(AbstractTestFixture):
    #Information about the data in line_section_test
    #The data is valid from "20170101T000000" to "20170131T000000"
    #The disruption has publication_period from "20170101T000000" to "20170110T000000"
    #and one application_period from "20170102T000000" to "20170105T000000"
    def test_line_reports(self):
        response = self.query_region("line_reports?_current_datetime=20170103T120000")
        disruptions = get_not_null(response, 'disruptions')
        assert len(disruptions) == 2
        line_reports = get_not_null(response, 'line_reports')
        assert len(line_reports) == 1
        is_valid_line_report(line_reports[0])
        assert line_reports[0]['line']['id'] == 'line:1'
        assert len(line_reports[0]['pt_objects']) == 5
        assert line_reports[0]['pt_objects'][0]['id'] == 'C_1'
        assert line_reports[0]['pt_objects'][1]['id'] == 'D_1'
        assert line_reports[0]['pt_objects'][2]['id'] == 'D_3'
        assert line_reports[0]['pt_objects'][3]['id'] == 'E_1'
        assert line_reports[0]['pt_objects'][4]['id'] == 'F_1'
        for pt_object in line_reports[0]['pt_objects']:
            assert pt_object['embedded_type'] == 'stop_point'
            assert len(pt_object['stop_point']['links']) == (2 if pt_object['id'] == 'E_1' else 1)
            if pt_object['id'] != 'F_1':
                assert pt_object['stop_point']['links'][0]['id'] == 'line_section_on_line_1'
        assert line_reports[0]['pt_objects'][3]['stop_point']['links'][1]['id'] == 'line_section_on_line_1_other_effect'
        assert line_reports[0]['pt_objects'][4]['stop_point']['links'][0]['id'] == 'line_section_on_line_1_other_effect'

    def test_line_reports_with_current_datetime_outof_application_period(self):
        #without since/until we use since=production_date.begin and until = production_date.end
        #application period intersects with active period of the disruption but not publication period
        response = self.query_region("line_reports?_current_datetime=20170111T130000")
        assert len(response['disruptions']) == 0

    def test_line_reports_with_since_until_intersects_application_period(self):
        response = self.query_region("line_reports?_current_datetime=20170101T120000"
                                     "&since=20170104T130000&until=20170106T000000")
        disruptions = get_not_null(response, 'disruptions')
        assert len(disruptions) == 2
        line_reports = get_not_null(response, 'line_reports')
        assert len(line_reports) == 1
        is_valid_line_report(line_reports[0])
        assert line_reports[0]['line']['id'] == 'line:1'
        assert len(line_reports[0]['pt_objects']) == 5
        assert line_reports[0]['pt_objects'][0]['id'] == 'C_1'
        assert line_reports[0]['pt_objects'][1]['id'] == 'D_1'
        assert line_reports[0]['pt_objects'][2]['id'] == 'D_3'
        assert line_reports[0]['pt_objects'][3]['id'] == 'E_1'
        assert line_reports[0]['pt_objects'][4]['id'] == 'F_1'
        for pt_object in line_reports[0]['pt_objects']:
            assert pt_object['embedded_type'] == 'stop_point'
            assert len(pt_object['stop_point']['links']) == (2 if pt_object['id'] == 'E_1' else 1)
            if pt_object['id'] != 'F_1':
                assert pt_object['stop_point']['links'][0]['id'] == 'line_section_on_line_1'
        assert line_reports[0]['pt_objects'][3]['stop_point']['links'][1]['id'] == 'line_section_on_line_1_other_effect'
        assert line_reports[0]['pt_objects'][4]['stop_point']['links'][0]['id'] == 'line_section_on_line_1_other_effect'

    def test_line_reports_with_since_intersects_application_period(self):
        response = self.query_region("line_reports?_current_datetime=20170101T120000&since=20170104T130000")
        disruptions = get_not_null(response, 'disruptions')
        assert len(disruptions) == 2
        line_reports = get_not_null(response, 'line_reports')
        assert len(line_reports) == 1
        is_valid_line_report(line_reports[0])
        assert line_reports[0]['line']['id'] == 'line:1'
        assert len(line_reports[0]['pt_objects']) == 5
        assert line_reports[0]['pt_objects'][0]['id'] == 'C_1'
        assert line_reports[0]['pt_objects'][1]['id'] == 'D_1'
        assert line_reports[0]['pt_objects'][2]['id'] == 'D_3'
        assert line_reports[0]['pt_objects'][3]['id'] == 'E_1'
        assert line_reports[0]['pt_objects'][4]['id'] == 'F_1'
        for pt_object in line_reports[0]['pt_objects']:
            assert pt_object['embedded_type'] == 'stop_point'
            assert len(pt_object['stop_point']['links']) == (2 if pt_object['id'] == 'E_1' else 1)
            if pt_object['id'] != 'F_1':
                assert pt_object['stop_point']['links'][0]['id'] == 'line_section_on_line_1'
        assert line_reports[0]['pt_objects'][3]['stop_point']['links'][1]['id'] == 'line_section_on_line_1_other_effect'
        assert line_reports[0]['pt_objects'][4]['stop_point']['links'][0]['id'] == 'line_section_on_line_1_other_effect'

    def test_line_reports_with_since_until_outof_application_period(self):
        response = self.query_region("line_reports?_current_datetime=20170101T120000"
                                     "&since=20170105T130000&until=20170108T000000")
        assert len(response['disruptions']) == 0

    def test_line_reports_with_since_until_value_not_valid(self):
        response, code = self.query_region("line_reports?_current_datetime=20170101T120000"
                                           "&since=20170108T130000&until=20170105T000000", check=False)
        assert code == 404
        assert response['error']['message'] == 'invalid filtering period (since > until)'
