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

from .tests_mechanism import AbstractTestFixture, dataset
from .check_utils import *
import pytest
from jormungandr import app


def get_duration(coord, response):
    lon, lat = coord.split(';')
    lat = float(lat)
    lon = float(lon)
    heat_map = response['heat_maps'][0]
    i = next(
        (
            i
            for i, h in enumerate(heat_map['heat_matrix']['line_headers'])
            if h['cell_lat']['min_lat'] <= lat and lat <= h['cell_lat']['max_lat']
        )
    )
    duration = next(
        (
            l['duration'][i]
            for l in heat_map['heat_matrix']['lines']
            if l['cell_lon']['min_lon'] <= lon and lon <= l['cell_lon']['max_lon']
        )
    )
    return duration


@dataset({"main_routing_test": {}})
class TestHeatMap(AbstractTestFixture):
    """
    Test the structure of the heat_maps response
    Only the format of the heat_map is check not the value. The value returned is an approximation, as explained below.

    Heatmap duration explanation:
    -----------------------------
    The heatmap endpoint returns a table of cells, each with a min/max lon and min/max lat
    The coordinates of the point of departure given are included in one of these cells.
    (the point longitude is within min/max lon of the cell and the point latitude is within min/max lat of the cell)
    The duration is then calculated from the coordinates of the center of the cell to the required stop point.
    Hence the approximation and the comments "about..."

    Ex: if the departure coord are (0.12;0.26), see '*' below, the duration will be calculated from '+' (0.15;0.25)

           \lat |       |       |       |
        lon \  0.0     0.1     0.2     0.3
         0.0 ---|-------|-------|-------|
                |       |       |       |
                |       |       |       |
                |       |       |       |
         0.1 ---|-------|-------|-------|
                |       |       |    *  |
                |       |       |   +   |
                |       |       |       |
         0.2 ---|-------|-------|-------|
                |       |       |       |
                |       |       |       |
                |       |       |       |
         0.3 ---|-------|-------|-------|
                |       |       |       |

    """

    """
    For the purpose of tests' stability, a tolerance is used when comparing with the expected value. Since when the test
    dataset is changed, the position of heatmap's cells will also move around a little bit, so do the projections.
    """
    HEATMAP_TOLERANCE = 10

    def test_from_heat_map_coord(self):
        query = "v1/coverage/main_routing_test/" + heat_map_basic_query
        response = self.query(query)
        requested_datetime = response['heat_maps'][0]['requested_date_time']

        is_valid_heat_maps(response, self.tester, query)
        assert requested_datetime == '20120614T080000'
        self.check_context(response)

    def test_to_heat_map_coord(self):
        query = "v1/coverage/main_routing_test/heat_maps?to={}&datetime={}&max_duration={}"
        query = query.format(r_coord, "20120614T080200", "3600")
        response = self.query(query)

        is_valid_heat_maps(response, self.tester, query)
        self.check_context(response)

    def test_heat_maps_from_stop_point(self):
        q = "v1/coverage/main_routing_test/heat_maps?datetime={}&from={}&max_duration={}"
        q = q.format('20120614T080100', 'stopB', '3600')
        response = self.query(q)

        is_valid_heat_maps(response, self.tester, q)

        assert get_duration(stopB_coord, response) == pytest.approx(0, abs=self.HEATMAP_TOLERANCE)  # about 0
        assert get_duration(s_coord, response) == pytest.approx(18, abs=self.HEATMAP_TOLERANCE)  # about 18
        assert get_duration(stopA_coord, response) == pytest.approx(2, abs=self.HEATMAP_TOLERANCE)  # about 2
        assert get_duration(r_coord, response) == pytest.approx(83, abs=self.HEATMAP_TOLERANCE)  # about 2 + 81
        self.check_context(response)

    def test_heat_maps_to_stop_point(self):
        # In order to not disturb the test, line M which was added afterwards for shared section tests, is forbidden here
        q = "v1/coverage/main_routing_test/heat_maps?datetime={}&to={}&max_duration={}&forbidden_uris[]=M&"
        q = q.format('20120614T080200', 'stopA', '3600')
        response = self.query(q)

        tolerance = 10
        is_valid_heat_maps(response, self.tester, q)
        assert get_duration(stopA_coord, response) == pytest.approx(0, abs=self.HEATMAP_TOLERANCE)  # about 0
        assert get_duration(r_coord, response) == pytest.approx(81, abs=self.HEATMAP_TOLERANCE)  # about 81
        assert get_duration(stopB_coord, response) == pytest.approx(60, abs=self.HEATMAP_TOLERANCE)  # about 60
        assert get_duration(s_coord, response) == pytest.approx(78, abs=self.HEATMAP_TOLERANCE)  # about 60 + 18
        self.check_context(response)

    def test_heat_maps_no_datetime(self):
        current_datetime = '20120614T080000'
        q_no_dt = "v1/coverage/main_routing_test/heat_maps?from={}&max_duration={}&_current_datetime={}"
        q_no_dt = q_no_dt.format(s_coord, '3600', current_datetime)
        response_no_dt = self.query(q_no_dt)

        is_valid_heat_maps(response_no_dt, self.tester, q_no_dt)
        excepted_context = {'current_datetime': current_datetime, 'timezone': 'UTC'}
        self.check_context(response_no_dt)

        for key, value in excepted_context.items():
            assert response_no_dt['context'][key] == value

    def test_heat_maps_no_seconds_in_datetime(self):
        q_no_s = "v1/coverage/main_routing_test/heat_maps?from={}&max_duration={}&datetime={}"
        q_no_s = q_no_s.format(s_coord, '3600', '20120614T0800')
        response_no_s = self.query(q_no_s)

        is_valid_heat_maps(response_no_s, self.tester, q_no_s)
        self.check_context(response_no_s)

    def test_heat_maps_no_arguments(self):
        q = "v1/coverage/main_routing_test/heat_maps"
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 400
        assert normal_response['message'] == 'you should provide a \'from\' or a \'to\' argument'

    def test_heat_maps_no_region(self):
        q = "v1/coverage/heat_maps"
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 404
        assert normal_response['error']['message'] == 'The region heat_maps doesn\'t exists'

    def test_heat_maps_invalid_duration(self):
        q = "v1/coverage/main_routing_test/heat_maps?datetime={}&from={}&max_duration={}"
        q = q.format('20120614T080000', s_coord, '-3600')
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 400
        assert (
            'Unable to evaluate, invalid unsigned int\n'
            'max_duration description: Maximum duration of journeys in seconds' in normal_response['message']
        )

        p = "v1/coverage/main_routing_test/isochrones?datetime={}&from={}&max_duration={}"
        p = p.format('20120614T080000', s_coord, 'toto')
        normal_response, error_code = self.query_no_assert(p)

        assert error_code == 400
        assert (
            'Unable to evaluate, invalid literal for int() with base 10: \'toto\'' in normal_response['message']
        )

    def test_heat_maps_null_speed(self):
        q = "v1/coverage/main_routing_test/heat_maps?datetime={}&from={}&max_duration={}&walking_speed=0"
        q = q.format('20120614T080000', s_coord, '3600')
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 400
        assert (
            'The walking_speed argument has to be in range [0.01, 4], you gave : 0' in normal_response['message']
        )

    def test_heat_maps_no_duration(self):
        q = "v1/coverage/main_routing_test/heat_maps?datetime={}&from={}"
        q = q.format('20120614T080000', s_coord)
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 400
        assert normal_response['message'] == "you should provide a 'max_duration' argument"

    def test_heat_maps_date_out_of_bound(self):
        q = "v1/coverage/main_routing_test/heat_maps?datetime={}&from={}&max_duration={}"
        q = q.format('20050614T080000', s_coord, '3600')
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 404
        assert normal_response['error']['id'] == 'date_out_of_bounds'
        assert normal_response['error']['message'] == 'date is not in data production period'

    def test_heat_maps_invalid_datetime(self):
        q = "v1/coverage/main_routing_test/heat_maps?datetime={}&from={}&max_duration={}"
        q = q.format('2005061', s_coord, '3600')
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 400
        assert 'Unable to parse datetime, year' in normal_response['message']

        p = "v1/coverage/main_routing_test/heat_maps?datetime={}&from={}&max_duration={}"
        p = p.format('toto', s_coord, 3600)
        normal_response, error_code = self.query_no_assert(p)

        assert error_code == 400
        assert 'unable to parse datetime, unknown string format' in normal_response['message'].lower()

    def test_graphical_heat_maps_no_heat_maps(self):
        q = "v1/coverage/main_routing_test/heat_maps?datetime={}&from={}&max_duration={}"
        q = q.format('20120614T080000', 'stop_area:OIF:SA:toto', '3600')
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 404
        assert normal_response['error']['id'] == "unknown_object"
        assert normal_response['error']['message'] == 'The entry point: stop_area:OIF:SA:toto is not valid'

    def test_heat_maps_with_from_and_to(self):
        q = "v1/coverage/main_routing_test/" + heat_map_basic_query + "&to={}"
        q = q.format(r_coord)
        normal_response, error_code = self.query_no_assert(q)

        assert error_code == 400
        assert normal_response['message'] == 'you cannot provide a \'from\' and a \'to\' argument'
