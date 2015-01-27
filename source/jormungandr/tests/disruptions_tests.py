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
from jormungandr import utils
from check_utils import *


def get_impacts(response):
    return {d['impact_id']: d for d in response['disruptions']}

# for the tests we need custom datetime to display the disruptions
default_date_filter = 'datetime=20140101T000000&_current_datetime=20140101T000000'


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

        response = self.query_region('traffic_reports?' + default_date_filter, display=True)

        traffic_report = get_not_null(response, 'traffic_reports')

        # the disruptions are grouped by network and we have only one network
        assert len(traffic_report) == 1

        impacted_lines = get_not_null(traffic_report[0], 'lines')
        assert len(impacted_lines) == 1
        is_valid_line(impacted_lines[0], depth_check=0)
        assert impacted_lines[0]['id'] == 'A'

        lines_disrupt = get_disruptions(impacted_lines[0], response)
        assert len(lines_disrupt) == 1
        for d in lines_disrupt:
            is_valid_disruption(d)
        assert lines_disrupt[0]['uri'] == 'disruption_on_line_A'
        assert lines_disrupt[0]['impact_id'] == 'too_bad_again'
        assert lines_disrupt[0]['severity']['name'] == 'bad severity'

        impacted_network = get_not_null(traffic_report[0], 'network')
        is_valid_network(impacted_network, depth_check=0)
        assert impacted_network['id'] == 'base_network'
        network_disrupt = get_disruptions(impacted_network, response)
        assert len(network_disrupt) == 1
        for d in network_disrupt:
            is_valid_disruption(d)
        assert network_disrupt[0]['uri'] == 'disruption_on_line_A'
        assert network_disrupt[0]['impact_id'] == 'too_bad_again'

        impacted_stop_areas = get_not_null(traffic_report[0], 'stop_areas')
        assert len(impacted_stop_areas) == 1
        is_valid_stop_area(impacted_stop_areas[0], depth_check=0)
        assert impacted_stop_areas[0]['id'] == 'stopA'
        stop_disrupt = get_disruptions(impacted_stop_areas[0], response)
        assert len(stop_disrupt) == 1
        for d in stop_disrupt:
            is_valid_disruption(d)
        assert stop_disrupt[0]['uri'] == 'disruption_on_stop_A'
        assert stop_disrupt[0]['impact_id'] == 'too_bad'

        """
        by querying directly the impacted object, we find the same results
        """
        # TODO: we can't make those test for the moment since we need to add a datetime param to those APIs
        # networks = self.query_region('networks/base_network?' + default_date_filter)
        # network = get_not_null(networks, 'networks')[0]
        # is_valid_network(network)
        # assert get_not_null(network, 'disruptions') == network_disrupt
        # assert get_not_null(network, 'messages') == get_not_null(impacted_network, 'messages')
        #
        # lines = self.query_region('lines/A?' + default_date_filter)
        # line = get_not_null(lines, 'lines')[0]
        # is_valid_line(line)
        # assert get_not_null(line, 'disruptions') == lines_disrupt
        # assert get_not_null(line, 'messages') == get_not_null(impacted_lines[0], 'messages')
        #
        # stops = self.query_region('stop_areas/stopA?' + default_date_filter)
        # stop = get_not_null(stops, 'stop_areas')[0]
        # is_valid_stop_area(stop)
        # assert get_not_null(stop, 'disruptions') == stop_disrupt
        # assert get_not_null(stop, 'messages') == get_not_null(impacted_stop_areas[0], 'messages')

    def test_disruption_with_stop_areas(self):
        """
        when calling the pt object stopA, we should get its disruptions
        """

        response = self.query_region('stop_areas/stopA')

        stops = get_not_null(response, 'stop_areas')
        assert len(stops) == 1
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
        assert len(disruptions) == 1

        impacted_lines = get_not_null(disruptions[0], 'lines')
        assert len(impacted_lines) == 1
        is_valid_line(impacted_lines[0], depth_check=0)
        assert impacted_lines[0]['id'] == 'A'

        lines_disrupt = get_not_null(impacted_lines[0], 'disruptions')
        assert len(lines_disrupt) == 1
        assert lines_disrupt[0]['uri'] == 'disruption_on_line_A'
        assert lines_disrupt[0]['impact_id'] == 'too_bad_again'

        impacted_network = get_not_null(disruptions[0], 'network')

        is_valid_network(impacted_network, depth_check=0)
        assert impacted_network['id'] == 'base_network'
        network_disrupt = get_not_null(impacted_network, 'disruptions')
        assert len(network_disrupt) == 1
        assert network_disrupt[0]['uri'] == 'disruption_on_line_A'
        assert network_disrupt[0]['impact_id'] == 'too_bad_again'

        # but we should not have disruption on stop area, B is not disrupted
        assert 'stop_areas' not in disruptions[0]
        """

    def test_no_disruption(self):
        """
        when calling the pt object stopB, we should get no disruptions
        """

        response = self.query_region('stop_areas/stopB', display=True)

        stops = get_not_null(response, 'stop_areas')
        assert len(stops) == 1
        stop = stops[0]

        assert 'disruptions' not in stop

    def test_disruption_period_filter(self):
        """
        period filter

        we ask for the disruption on the 3 next days
        => we get 2 impacts (too_bad and too_bad_again)

        we ask for the disruption on the 3 next yers
        => we get 3 impacts (we get later_impact too)

        """

        response = self.query_region('traffic_reports?duration=P3D&' + default_date_filter)

        impacts = get_impacts(response)
        assert len(impacts) == 2
        assert 'later_impact' not in impacts

        response = self.query_region('traffic_reports?duration=P3Y&' + default_date_filter)

        impacts = get_impacts(response)
        assert len(impacts) == 3
        assert 'later_impact' in impacts

    def test_disruption_publication_date_filter(self):
        """
        test the publication date filter

        'disruption_on_line_A_but_publish_later' is published from the 28th of january at 10

        so at 9 it is not in the list, at 11, we get it
        """
        response = self.query_region('traffic_reports?datetime=20140101T000000'
                                     '&_current_datetime=20140128T090000', display=True)

        impacts = get_impacts(response)
        assert len(impacts) == 2
        assert 'impact_published_later' not in impacts

        response = self.query_region('traffic_reports?datetime=20140101T000000'
                                     '&_current_datetime=20140128T130000', display=True)

        impacts = get_impacts(response)
        assert len(impacts) == 3
        assert 'impact_published_later' in impacts
