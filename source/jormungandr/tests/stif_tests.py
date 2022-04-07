# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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

from __future__ import absolute_import
from .tests_mechanism import AbstractTestFixture, dataset
from .check_utils import *


@dataset({"main_stif_test": {}})
class TestStif(AbstractTestFixture):
    """
    Test the stif scenario responses
    Possible journeys from A to B:
    1/   8h00 ====(line A)====> 10h00
    2/   9h00 ==(line B + C)==> 11h00
    3/  10h00 ====(line A)====> 12h00
    """

    def test_stif_simple(self):
        """
        Test of simple request :
        * we want to make at least 2 journey calls (not only the best journey, but also try next)
        * we don't want 2 journeys using the same line and changing at same points
        So here we want journeys 1 and 2
        """
        query = (
            "journeys?from={from_sp}&to={to_sp}&datetime={datetime}&_override_scenario=new_default"
            "&min_nb_journeys=1&_min_journeys_calls=2&_final_line_filter=true&_max_successive_physical_mode=3".format(
                from_sp="stopA", to_sp="stopB", datetime="20140614T075500"
            )
        )

        response = self.query_region(query)
        assert len(response['journeys']) == 2
        assert response['journeys'][0]['arrival_date_time'] == '20140614T100000'
        assert response['journeys'][1]['arrival_date_time'] == '20140614T110000'

    def test_stif_override_min_journeys_calls(self):
        """
        Test of simple request :
        * we only want 1 journey calls (no next call)
        So here we only want journeys 1
        """
        query = (
            "journeys?from={from_sp}&to={to_sp}&datetime={datetime}&_override_scenario=new_default"
            "&min_nb_journeys=1&_min_journeys_calls=1&_final_line_filter=true&_max_successive_physical_mode=3".format(
                from_sp="stopA", to_sp="stopB", datetime="20140614T075500"
            )
        )

        response = self.query_region(query)
        assert len(response['journeys']) == 1
        assert response['journeys'][0]['arrival_date_time'] == '20140614T100000'

    def test_stif_override_final_line_filter(self):
        """
        Test of simple request :
        * we want to make at least 2 journey calls (not only the best journey, but also try next)
        * we deactivate the filter on journeys using the same line and changing at same points
        So here we want journeys 1, 2 and 3
        """
        query = (
            "journeys?from={from_sp}&to={to_sp}&datetime={datetime}&_override_scenario=new_default"
            "&min_nb_journeys=1&_min_journeys_calls=2&_final_line_filter=false&_max_successive_physical_mode=3".format(
                from_sp="stopA", to_sp="stopB", datetime="20140614T075500"
            )
        )

        response = self.query_region(query)
        assert len(response['journeys']) == 3
        assert response['journeys'][0]['arrival_date_time'] == '20140614T100000'
        assert response['journeys'][1]['arrival_date_time'] == '20140614T110000'
        assert response['journeys'][2]['arrival_date_time'] == '20140614T120000'

    def test_stif_max_successive_buses(self):
        """
                BUS         Bus         Bus         Bus
        stopP ----> stopQ ----> stopR ----> stopS ----> stopT
        15:00       16:00       17:00       18:00       19:00

                                Bus
        stopP ----------------------------------------> stopT
        15:00                                           20:00

        Test of request with parameter "_max_successive_physical_mode":
        * we want to make at least 2 journey calls (not only the best journey, but also try next)
        * we don't want the journey using more than 3 Buses
        So here we want journey1
        """
        query = (
            "journeys?from={from_sp}&to={to_sp}&datetime={datetime}&_override_scenario=new_default"
            "&_max_successive_physical_mode=3&_max_additional_connections=10".format(
                from_sp="stopP", to_sp="stopT", datetime="20140614T145500"
            )
        )

        response = self.query_region(query)
        assert len(response['journeys']) == 1

        # As we modify the value of _max_successive_physical_mode to 5 we want two journeys
        query = (
            "journeys?from={from_sp}&to={to_sp}&datetime={datetime}&_override_scenario=new_default"
            "&_max_successive_physical_mode=5&_max_additional_connections=10".format(
                from_sp="stopP", to_sp="stopT", datetime="20140614T145500"
            )
        )

        response = self.query_region(query)
        assert len(response['journeys']) == 2

    def test_stif_max_successive_buses_with_tram_in_between(self):
        """
                BUS         Bus         Bus         Bus         Tram        Bus         Bus
        stopP ----> stopQ ----> stopR ----> stopS ----> stopT ----> stopU ----> stopV ----> stopW
        15:00       16:00       17:00       18:00       19:00       19:30       20:00       20:30
                                Bus
        stopP ----------------------------------------------------------------------------> stopW
        15:00                                                                               21:00
        Test of request with parameter "_max_successive_physical_mode":
        * we want to make at least 2 journey calls (not only the best journey, but also try next)
        * we don't want the journey using more than 3 Buses successive
        * we have "Bus" and "Tramway" as means of transport
        """

        # As there are 4 buses successive to be used from stopP to stopW and _max_successive_physical_mode = 3
        # we have 1 journey
        query = (
            "journeys?from={from_sp}&to={to_sp}&datetime={datetime}&_override_scenario=new_default"
            "&_max_successive_physical_mode=3&_max_additional_connections=10".format(
                from_sp="stopP", to_sp="stopW", datetime="20140614T145500"
            )
        )

        response = self.query_region(query)
        assert len(response['journeys']) == 1

        # As we modify the value of _max_successive_physical_mode to 5 we want two journeys
        query = (
            "journeys?from={from_sp}&to={to_sp}&datetime={datetime}&_override_scenario=new_default"
            "&_max_successive_physical_mode=5&_max_additional_connections=10".format(
                from_sp="stopP", to_sp="stopW", datetime="20140614T145500"
            )
        )

        response = self.query_region(query)
        assert len(response['journeys']) == 2

        # As we modify the value of _max_additional_connections to 2
        # since the best journey is the one with 6 connexions, we will find 2 journeys
        query = (
            "journeys?from={from_sp}&to={to_sp}&datetime={datetime}&_override_scenario=new_default"
            "&_max_successive_physical_mode=5&_max_additional_connections=2".format(
                from_sp="stopP", to_sp="stopW", datetime="20140614T145500"
            )
        )

        response = self.query_region(query)
        assert len(response['journeys']) == 2

    def test_stif_max_waiting_duration(self):
        """
        Test max waiting duration:
        stopA --> StopC --> StopB
        Waiting 30 minutes in StopC
        """
        # default max_waiting_duration = 4*60*60
        query = "journeys?from={from_sp}&to={to_sp}&datetime={datetime}&_override_scenario=new_default".format(
            from_sp="stopA", to_sp="stopB", datetime="20140614T084500"
        )

        response = self.query_region(query)
        assert len(response['journeys']) == 2
        assert response['journeys'][0]['arrival_date_time'] == '20140614T110000'
        assert response['journeys'][0]['sections'][1]["type"] == 'waiting'
        assert response['journeys'][0]['sections'][1]["duration"] == 30 * 60
        assert response['journeys'][1]['arrival_date_time'] == '20140614T120000'

    def test_stif_override_max_waiting_duration(self):
        """
        Test max waiting duration:
        stopA --> StopC --> StopB
        Waiting 30 minutes in StopC
        """
        # override max_waiting_duration 15 minutes
        # max_waiting_duration = 15*60 = 900
        query = (
            "journeys?from={from_sp}&to={to_sp}&datetime={datetime}&_override_scenario=new_default"
            "&max_waiting_duration={max_waiting_duration}".format(
                from_sp="stopA", to_sp="stopB", datetime="20140614T084500", max_waiting_duration=15 * 60
            )
        )

        response = self.query_region(query)
        assert len(response['journeys']) == 1
        assert response['journeys'][0]['arrival_date_time'] == '20140614T120000'

    def test_stif_invalid_max_waiting_duration(self):
        """
        Test max waiting :
        stopA --> StopC --> StopB
        Waiting 30 minutes in StopC
        """
        # override max_waiting_duration
        # max_waiting_duration = -10
        query = (
            "journeys?from={from_sp}&to={to_sp}&datetime={datetime}&_override_scenario=new_default"
            "&max_waiting_duration={max_waiting_duration}".format(
                from_sp="stopA", to_sp="stopB", datetime="20140614T084500", max_waiting_duration=-10
            )
        )

        response, status_code = self.query_region(query, check=False)
        assert status_code == 400
        assert response["message"].startswith('parameter "max_waiting_duration" invalid')
