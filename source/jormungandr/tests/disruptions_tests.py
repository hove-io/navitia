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

        response = self.query_region('disruptions', display=True)

        disruptions = get_not_null(response, 'disruptions')

        # the disruptions are grouped by network and we have only one network
        assert len(disruptions) == 1

        impacted_lines = get_not_null(disruptions[0], 'lines')
        assert len(impacted_lines) == 1
        is_valid_line(impacted_lines[0], depth_check=0)
        assert impacted_lines[0]['id'] == 'A'

        lines_disrupt = get_not_null(impacted_lines[0], 'disruptions')
        assert len(lines_disrupt) == 1
        assert lines_disrupt[0]['uri'] == 'disruption_on_line_A'
        assert lines_disrupt[0]['impact_uri'] == 'too_bad_again'

        impacted_network = get_not_null(disruptions[0], 'network')
        is_valid_network(impacted_network, depth_check=0)
        assert impacted_network['id'] == 'base_network'
        network_disrupt = get_not_null(impacted_network, 'disruptions')
        assert len(network_disrupt) == 1
        assert network_disrupt[0]['uri'] == 'disruption_on_line_A'
        assert network_disrupt[0]['impact_uri'] == 'too_bad_again'

        impacted_stop_areas = get_not_null(disruptions[0], 'stop_areas')
        assert len(impacted_stop_areas) == 1
        is_valid_stop_area(impacted_stop_areas[0], depth_check=0)
        assert impacted_stop_areas[0]['id'] == 'stopA'
        stop_disrupt = get_not_null(impacted_stop_areas[0], 'disruptions')
        assert len(stop_disrupt) == 1
        assert stop_disrupt[0]['uri'] == 'disruption_on_stop_A'
        assert stop_disrupt[0]['impact_uri'] == 'too_bad'

        """
        by querying directly the impacted object, we find the same results
        """
        networks = self.query_region('networks/base_network', display=True)
        network = get_not_null(networks, 'networks')[0]
        is_valid_network(network)
        assert get_not_null(network, 'disruptions') == network_disrupt
        assert get_not_null(network, 'messages') == get_not_null(impacted_network, 'messages')

        lines = self.query_region('lines/A')
        line = get_not_null(lines, 'lines')[0]
        is_valid_line(line)
        assert get_not_null(line, 'disruptions') == lines_disrupt
        assert get_not_null(line, 'messages') == get_not_null(impacted_lines[0], 'messages')

        stops = self.query_region('stop_areas/stopA')
        stop = get_not_null(stops, 'stop_areas')[0]
        is_valid_stop_area(stop)
        assert get_not_null(stop, 'disruptions') == stop_disrupt
        assert get_not_null(stop, 'messages') == get_not_null(impacted_stop_areas[0], 'messages')

    def test_disruption_with_stop_areas(self):
        """
        when calling the pt object stopA, we should get its disruptions
        """

        response = self.query_region('stop_areas/stopA', display=True)

        stops = get_not_null(response, 'stop_areas')
        assert len(stops) == 1
        stop = stops[0]

        get_not_null(stop, 'disruptions')

    def test_no_disruption(self):
        """
        when calling the pt object stopB, we should get no disruptions
        """

        response = self.query_region('stop_areas/stopB', display=True)

        stops = get_not_null(response, 'stop_areas')
        assert len(stops) == 1
        stop = stops[0]

        assert 'disruptions' not in stop

#TODO, test on != dates