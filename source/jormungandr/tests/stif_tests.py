# Copyright (c) 2001-2016, Canal TP and/or its affiliates. All rights reserved.
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


@dataset({"main_stif_test": {}})
class TestAutocomplete(AbstractTestFixture):
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
        query = "journeys?from={from_sp}&to={to_sp}&datetime={datetime}&_override_scenario=stif"\
            .format(from_sp="stopA", to_sp="stopB", datetime="20140614T075500")

        response = self.query_region(query, display=True)
        eq_(len(response['journeys']), 2)
        eq_(response['journeys'][0]['arrival_date_time'], '20140614T100000')
        eq_(response['journeys'][1]['arrival_date_time'], '20140614T110000')


    def test_stif_override_min_journeys_calls(self):
        """
        Test of simple request :
        * we only want 1 journey calls (no next call)
        So here we only want journeys 1
        """
        query = "journeys?from={from_sp}&to={to_sp}&datetime={datetime}&_override_scenario=stif&_min_journeys_calls=1"\
            .format(from_sp="stopA", to_sp="stopB", datetime="20140614T075500")

        response = self.query_region(query, display=False)
        eq_(len(response['journeys']), 1)
        eq_(response['journeys'][0]['arrival_date_time'], '20140614T100000')



    def test_stif_override_final_line_filter(self):
        """
        Test of simple request :
        * we want to make at least 2 journey calls (not only the best journey, but also try next)
        * we deactivate the filter on journeys using the same line and changing at same points
        So here we want journeys 1, 2 and 3
        """
        query = "journeys?from={from_sp}&to={to_sp}&datetime={datetime}&_override_scenario=stif&_final_line_filter=false"\
            .format(from_sp="stopA", to_sp="stopB", datetime="20140614T075500")

        response = self.query_region(query, display=False)
        eq_(len(response['journeys']), 3)
        eq_(response['journeys'][0]['arrival_date_time'], '20140614T100000')
        eq_(response['journeys'][1]['arrival_date_time'], '20140614T110000')
        eq_(response['journeys'][2]['arrival_date_time'], '20140614T120000')

    def test_stif_max_successive_buses(self):
        """
                BUS         Bus         Bus         Bus
        stopP ----> stopQ ----> stopR ----> stopS ----> stopT
        15:00       16:00       17:00       18:00       19:00

                                Bus
        stopP ----------------------------------------> stopT
        15:00                                           20:00

        Test of request with parameter "_max_successive_buses":
        * we want to make at least 2 journey calls (not only the best journey, but also try next)
        * we don't want the journey using more than 3 Buses
        So here we want journey1
        """
        query = "journeys?from={from_sp}&to={to_sp}&datetime={datetime}&_override_scenario=stif"\
            .format(from_sp="stopP", to_sp="stopT", datetime="20140614T145500")

        response = self.query_region(query, display=False)
        eq_(len(response['journeys']), 1)

        #As we modifiy the value of _max_successive_buses to 5 we want two journeys
        query = "journeys?from={from_sp}&to={to_sp}&datetime={datetime}&_override_scenario=stif" \
                "&_max_successive_buses=5&_max_additional_changes=10"\
            .format(from_sp="stopP", to_sp="stopT", datetime="20140614T145500")

        response = self.query_region(query, display=False)
        eq_(len(response['journeys']), 2)


    def test_stif_max_successive_buses_with_tram_in_between(self):
        """
                BUS         Bus         Bus         Bus         Tram        Bus         Bus
        stopP ----> stopQ ----> stopR ----> stopS ----> stopT ----> stopU ----> stopV ----> stopW
        15:00       16:00       17:00       18:00       19:00       19:30       20:00       20:30
                                Bus
        stopP ----------------------------------------------------------------------------> stopW
        15:00                                                                               21:00
        Test of request with parameter "_max_successive_buses":
        * we want to make at least 2 journey calls (not only the best journey, but also try next)
        * we don't want the journey using more than 3 Buses successive
        * we have "Bus" and "Tram" as means of transport
        """

        #As there are 4 buses successive to be used from stopP to stopW and _max_successive_buses = 3
        # we have 1 journey
        query = "journeys?from={from_sp}&to={to_sp}&datetime={datetime}&_override_scenario=stif"\
            .format(from_sp="stopP", to_sp="stopW", datetime="20140614T145500")

        response = self.query_region(query, display=False)
        eq_(len(response['journeys']), 1)

        #As we modifiy the value of _max_successive_buses to 5 we want two journeys
        query = "journeys?from={from_sp}&to={to_sp}&datetime={datetime}&_override_scenario=stif" \
                "&_max_successive_buses=5&_max_additional_changes=10"\
            .format(from_sp="stopP", to_sp="stopW", datetime="20140614T145500")

        response = self.query_region(query, display=False)
        eq_(len(response['journeys']), 2)

        # As we modifiy the value of _max_additional_changes to 2 we delete the second journey because
        # it contains more then nb_correspondence + 2 ()
        query = "journeys?from={from_sp}&to={to_sp}&datetime={datetime}&_override_scenario=stif&_max_successive_buses=5"\
            .format(from_sp="stopP", to_sp="stopW", datetime="20140614T145500")

        response = self.query_region(query, display=False)
        eq_(len(response['journeys']), 1)

