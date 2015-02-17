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
from tests_mechanism import AbstractTestFixture, dataset
from check_utils import *


def get_impacts(response):
    return {d['impact_id']: d for d in response['disruptions']}

# for the tests we need custom datetime to display the disruptions
default_date_filter = '_current_datetime=20140101T000000'


@dataset(["main_routing_test"])
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

        response = self.query_region('traffic_reports?' + default_date_filter, display=False)

        traffic_report = get_not_null(response, 'traffic_reports')

        # the disruptions are grouped by network and we have only one network
        eq_(len(traffic_report), 1)

        impacted_lines = get_not_null(traffic_report[0], 'lines')
        eq_(len(impacted_lines), 1)
        is_valid_line(impacted_lines[0], depth_check=0)
        eq_(impacted_lines[0]['id'], 'A')

        lines_disrupt = get_disruptions(impacted_lines[0], response)
        eq_(len(lines_disrupt), 2)
        for d in lines_disrupt:
            is_valid_disruption(d)
        eq_(lines_disrupt[0]['uri'], 'disruption_on_line_A')
        eq_(lines_disrupt[0]['impact_id'], 'too_bad_again')
        eq_(lines_disrupt[0]['severity']['name'], 'bad severity')

        eq_(lines_disrupt[1]['uri'], 'disruption_on_line_A_but_later')
        eq_(lines_disrupt[1]['impact_id'], 'later_impact')
        eq_(lines_disrupt[1]['severity']['name'], 'information severity')

        impacted_network = get_not_null(traffic_report[0], 'network')
        is_valid_network(impacted_network, depth_check=0)
        eq_(impacted_network['id'], 'base_network')
        network_disrupt = get_disruptions(impacted_network, response)
        eq_(len(network_disrupt), 2)
        for d in network_disrupt:
            is_valid_disruption(d)
        eq_(network_disrupt[0]['uri'], 'disruption_on_line_A')
        eq_(network_disrupt[0]['impact_id'], 'too_bad_again')

        eq_(network_disrupt[1]['uri'], 'disruption_on_line_A_but_later')
        eq_(network_disrupt[1]['impact_id'], 'later_impact')

        impacted_stop_areas = get_not_null(traffic_report[0], 'stop_areas')
        eq_(len(impacted_stop_areas), 1)
        is_valid_stop_area(impacted_stop_areas[0], depth_check=0)
        eq_(impacted_stop_areas[0]['id'], 'stopA')
        stop_disrupt = get_disruptions(impacted_stop_areas[0], response)
        eq_(len(stop_disrupt), 1)
        for d in stop_disrupt:
            is_valid_disruption(d)
        eq_(stop_disrupt[0]['uri'], 'disruption_on_stop_A')
        eq_(stop_disrupt[0]['impact_id'], 'too_bad')

        """
        by querying directly the impacted object, we find the same results
        """
        # TODO: we can't make those test for the moment since we need to add a datetime param to those APIs
        # networks = self.query_region('networks/base_network?' + default_date_filter)
        # network = get_not_null(networks, 'networks')[0]
        # is_valid_network(network)
        # eq_(get_not_null(network, 'disruptions'), network_disrupt)
        # eq_(get_not_null(network, 'messages'), get_not_null(impacted_network, 'messages'))
        #
        # lines = self.query_region('lines/A?' + default_date_filter)
        # line = get_not_null(lines, 'lines')[0]
        # is_valid_line(line)
        # eq_(get_not_null(line, 'disruptions'), lines_disrupt)
        # eq_(get_not_null(line, 'messages'), get_not_null(impacted_lines[0], 'messages'))
        #
        # stops = self.query_region('stop_areas/stopA?' + default_date_filter)
        # stop = get_not_null(stops, 'stop_areas')[0]
        # is_valid_stop_area(stop)
        # eq_(get_not_null(stop, 'disruptions'), stop_disrupt)
        # eq_(get_not_null(stop, 'messages'), get_not_null(impacted_stop_areas[0], 'messages'))

    def test_disruption_with_stop_areas(self):
        """
        when calling the pt object stopA, we should get its disruptions
        """

        response = self.query_region('stop_areas/stopA')

        stops = get_not_null(response, 'stop_areas')
        eq_(len(stops), 1)
        stop = stops[0]

        #TODO: we can't make those test for the moment since we need to add a datetime param to those APIs
        #get_not_null(stop, 'disruptions')

    def test_direct_disruption_call(self):
        """
        when calling the disruptions on the pt object stopB,
        we should get the disruption summary filtered on the stopB

        so
        we get the impact on the line A (it goes through stopB),
        and the impact on the network
        """

        """
        TODO: we can't make those test for the moment since we need to add a datetime param to those APIs
        response = self.query_region('stop_areas/stopB/disruptions?' + default_date_filter, display=True)

        disruptions = get_not_null(response, 'disruptions')
        eq_(len(disruptions), 1)

        impacted_lines = get_not_null(disruptions[0], 'lines')
        eq_(len(impacted_lines), 1)
        is_valid_line(impacted_lines[0], depth_check=0)
        eq_(impacted_lines[0]['id'], 'A')

        lines_disrupt = get_not_null(impacted_lines[0], 'disruptions')
        eq_(len(lines_disrupt), 1)
        eq_(lines_disrupt[0]['uri'], 'disruption_on_line_A')
        eq_(lines_disrupt[0]['impact_id'], 'too_bad_again')

        impacted_network = get_not_null(disruptions[0], 'network')

        is_valid_network(impacted_network, depth_check=0)
        eq_(impacted_network['id'], 'base_network')
        network_disrupt = get_not_null(impacted_network, 'disruptions')
        eq_(len(network_disrupt), 1)
        eq_(network_disrupt[0]['uri'], 'disruption_on_line_A')
        eq_(network_disrupt[0]['impact_id'], 'too_bad_again')

        # but we should not have disruption on stop area, B is not disrupted
        assert 'stop_areas' not in disruptions[0]
        """

    def test_no_disruption(self):
        """
        when calling the pt object stopB, we should get no disruptions
        """

        response = self.query_region('stop_areas/stopB', display=False)

        stops = get_not_null(response, 'stop_areas')
        eq_(len(stops), 1)
        stop = stops[0]

        assert 'disruptions' not in stop

    def test_disruption_publication_date_filter(self):
        """
        test the publication date filter

        'disruption_on_line_A_but_publish_later' is published from the 28th of january at 10

        so at 9 it is not in the list, at 11, we get it
        """
        response = self.query_region('traffic_reports?_current_datetime=20140128T090000', display=False)

        impacts = get_impacts(response)
        eq_(len(impacts), 3)
        assert 'impact_published_later' not in impacts

        response = self.query_region('traffic_reports?_current_datetime=20140128T130000', display=False)

        impacts = get_impacts(response)
        eq_(len(impacts), 4)
        assert 'impact_published_later' in impacts
