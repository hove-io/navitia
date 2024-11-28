# Copyright (c) 2001-2024, Hove and/or its affiliates. All rights reserved.
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

@dataset({"park_modes_test": {}})
class TestParkMode(AbstractTestFixture):
    """
    Test park mode features
    """

    def test_first_section_park_mode_none_with_access_point(self):
        """"
        Test park mode none with _access point
        """
        query = "v1/coverage/main_routing_test/journeys?from=2.36893%3B48.88413&to=2.28928%3B48.84710&first_section_mode%5B%5D=bike&park_mode%5B%5D=none&_access_points=true"
        response = self.query(query)
        assert len(response['journeys']) > 0

        for journey in response['journeys']:
            if len(journey['sections']) > 1:
                assert journey['sections'][0]['mode'] == 'bike'
                assert journey['sections'][1]['type'] != 'park'


    def test_first_section_park_mode_none_without_access_point(self):
        """"
        Test park mode none without access point
        """
        query = "v1/coverage/main_routing_test/journeys?from=2.36893%3B48.88413&to=2.28928%3B48.84710&first_section_mode%5B%5D=bike&park_mode%5B%5D=none"
        response = self.query(query)
        assert len(response['journeys']) > 0

        for journey in response['journeys']:
            if len(journey['sections']) > 1:
                assert journey['sections'][0]['mode'] == 'bike'
                assert journey['sections'][1]['type'] != 'park'


    def test_first_section_park_mode_on_street_with_access_point(self):
        """"
        Test park mode on street with access point
        """
        query = "v1/coverage/main_routing_test/journeys?from=2.36893%3B48.88413&to=2.28928%3B48.84710&first_section_mode%5B%5D=bike&park_mode%5B%5D=on_street&_access_points=true"
        response = self.query(query)
        assert len(response['journeys']) > 0

        for journey in response['journeys']:
            if len(journey['sections']) > 0:
                assert len( journey['sections'][0]['vias']) > 0
                assert journey['sections'][0]['mode'] == 'bike'
                assert journey['sections'][1]['type'] == 'park'
                assert journey['sections'][1]['type'] == 'street_network'

                # Verify if the park sections doesnt have any from and to.
                assert journey['sections'][1]['from']["embedded_type"] is None
                assert journey['sections'][1]['to']["embedded_type"] is None


    def test_first_section_park_mode_on_street_without_access_point(self):
        """"
        Test park mode on street without access point
        """
        query = "v1/coverage/main_routing_test/journeys?from=2.36893%3B48.88413&to=2.28928%3B48.84710&first_section_mode%5B%5D=bike&park_mode%5B%5D=on_street"
        response = self.query(query)
        assert len(response['journeys']) > 0

        for journey in response['journeys']:
            if len(journey['sections']) > 0:
                assert len( journey['sections'][0]['vias']) > 0
                assert journey['sections'][0]['mode'] == 'bike'
                assert journey['sections'][1]['type'] == 'park'
                assert journey['sections'][1]['type'] == 'street_network'

                # Verify if the park sections doesnt have any from and to.
                assert journey['sections'][1]['from']["embedded_type"] is None
                assert journey['sections'][1]['to']["embedded_type"] is None


    def test_first_section_park_mode_park_and_ride_with_access_point(self):
        """"
        Test park mode park and ride with access point
        """
        query = "v1/coverage/main_routing_test/journeys?from=2.36893%3B48.88413&to=2.28928%3B48.84710&first_section_mode%5B%5D=bike&park_mode%5B%5D=park_and_ride&_access_points=true"
        response = self.query(query)
        assert len(response['journeys']) > 0

        for journey in response['journeys']:
            if len(journey['sections']) > 1:
                assert journey['sections'][0]['mode'] == 'bike'
                assert journey['sections'][1]['type'] != 'park'

    def test_first_section_park_mode_park_and_ride_without_access_point(self):
        """"
        Test park mode park and ride without access point
        """
        query = "v1/coverage/main_routing_test/journeys?from=2.36893%3B48.88413&to=2.28928%3B48.84710&first_section_mode%5B%5D=bike&park_mode%5B%5D=park_and_ride"
        response = self.query(query)
        assert len(response['journeys']) > 0

        for journey in response['journeys']:
            if len(journey['sections']) > 1:
                assert journey['sections'][0]['mode'] == 'bike'
                assert journey['sections'][1]['type'] != 'park'


    def test_last_section_mode_park_mode_none_with_access_point(self):
        """"
        Test park mode none with _access point
        """
        query = "v1/coverage/main_routing_test/journeys?from=2.36893%3B48.88413&to=2.28928%3B48.84710&last_section_mode%5B%5D=bike&park_mode%5B%5D=none&_access_points=true"
        response = self.query(query)
        assert len(response['journeys']) > 0

        for journey in response['journeys']:
            if len(journey['sections']) > 1:
                assert journey['sections'][0]['mode'] == 'bike'
                assert journey['sections'][1]['type'] != 'park'


    def test_last_section_mode_park_mode_none_without_access_point(self):
        """"
        Test park mode none without access point
        """
        query = "v1/coverage/main_routing_test/journeys?from=2.36893%3B48.88413&to=2.28928%3B48.84710&last_section_mode%5B%5D=bike&park_mode%5B%5D=none"
        response = self.query(query)
        assert len(response['journeys']) > 0

        for journey in response['journeys']:
            if len(journey['sections']) > 1:
                assert journey['sections'][0]['mode'] == 'bike'
                assert journey['sections'][1]['type'] != 'park'


    def test_last_section_mode_park_mode_on_street_with_access_point(self):
        """"
        Test park mode on street with access point
        """
        query = "v1/coverage/main_routing_test/journeys?from=2.36893%3B48.88413&to=2.28928%3B48.84710&last_section_mode%5B%5D=bike&park_mode%5B%5D=on_street&_access_points=true"
        response = self.query(query)
        assert len(response['journeys']) > 0

        for journey in response['journeys']:
            if len(journey['sections']) > 0:
                assert len( journey['sections'][0]['vias']) > 0
                assert journey['sections'][0]['mode'] == 'bike'
                assert journey['sections'][1]['type'] == 'park'
                assert journey['sections'][1]['type'] == 'street_network'

                # Verify if the park sections doesnt have any from and to.
                assert journey['sections'][1]['from']["embedded_type"] is None
                assert journey['sections'][1]['to']["embedded_type"] is None


    def test_last_section_mode_park_mode_on_street_without_access_point(self):
        """"
        Test park mode on street without access point
        """
        query = "v1/coverage/main_routing_test/journeys?from=2.36893%3B48.88413&to=2.28928%3B48.84710&last_section_mode%5B%5D=bike&park_mode%5B%5D=on_street"
        response = self.query(query)
        assert len(response['journeys']) > 0

        for journey in response['journeys']:
            if len(journey['sections']) > 0:
                assert len( journey['sections'][0]['vias']) > 0
                assert journey['sections'][0]['mode'] == 'bike'
                assert journey['sections'][1]['type'] == 'park'
                assert journey['sections'][1]['type'] == 'street_network'

                # Verify if the park sections doesnt have any from and to.
                assert journey['sections'][1]['from']["embedded_type"] is None
                assert journey['sections'][1]['to']["embedded_type"] is None


    def test_last_section_mode_park_mode_park_and_ride_with_access_point(self):
        """"
        Test park mode park and ride with access point
        """
        query = "v1/coverage/main_routing_test/journeys?from=2.36893%3B48.88413&to=2.28928%3B48.84710&last_section_mode%5B%5D=bike&park_mode%5B%5D=park_and_ride&_access_points=true"
        response = self.query(query)
        assert len(response['journeys']) > 0

        for journey in response['journeys']:
            if len(journey['sections']) > 1:
                assert journey['sections'][0]['mode'] == 'bike'
                assert journey['sections'][1]['type'] != 'park'

    def test_last_section_mode_park_mode_park_and_ride_without_access_point(self):
        """"
        Test park mode park and ride without access point
        """
        query = "v1/coverage/main_routing_test/journeys?from=2.36893%3B48.88413&to=2.28928%3B48.84710&last_section_mode%5B%5D=bike&park_mode%5B%5D=park_and_ride"
        response = self.query(query)
        assert len(response['journeys']) > 0

        for journey in response['journeys']:
            if len(journey['sections']) > 1:
                assert journey['sections'][0]['mode'] == 'bike'
                assert journey['sections'][1]['type'] != 'park'
