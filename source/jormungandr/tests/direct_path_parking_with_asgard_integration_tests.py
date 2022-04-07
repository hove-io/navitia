# Copyright (c) 2001-2016, Hove and/or its affiliates. All rights reserved.
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
import pytest

MOCKED_PARKING_WITH_ASGARD_CONF = [
    {
        "modes": ["car"],
        "class": "jormungandr.street_network.with_parking.WithParking",
        "args": {
            "parking": {
                "class": "jormungandr.street_network.parking.constant_parking.ConstantParking",
                "args": {"park_duration": 600, "leave_parking_duration": 300},
            },
            "street_network": {
                "class": "tests.direct_path_asgard_integration_tests.MockAsgard",
                "args": {"api_key": "", "asgard_socket": "bob_socket", "timeout": 10},
            },
        },
    },
    {
        "modes": ['walking', 'bike', 'bss'],
        "class": "tests.direct_path_asgard_integration_tests.MockAsgard",
        "args": {
            "costing_options": {"bicycle": {"bicycle_type": "Hybrid"}},
            "api_key": "",
            "asgard_socket": "bob_socket",
            "service_url": "http://bob.com",
            "timeout": 10,
        },
    },
]


s_coord = "0.0000898312;0.0000898312"  # coordinate of S in the dataset
r_coord = "0.00188646;0.00071865"  # coordinate of R in the dataset

journey_basic_query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}".format(
    from_coord=s_coord, to_coord=r_coord, datetime="20120614080000"
)


@dataset(
    {
        'main_routing_test': {
            'scenario': 'distributed',
            'instance_config': {'street_network': MOCKED_PARKING_WITH_ASGARD_CONF},
        }
    }
)
class TestWithParkingDirectPath(AbstractTestFixture):
    @pytest.mark.xfail
    def test_journey_direct_path(self):
        query = journey_basic_query + "&first_section_mode[]=car" + "&last_section_mode[]=car" + "&debug=true"

        response = self.query_region(query)

        assert len(response['journeys']) == 1

        # car from asgard
        assert 'car' in response['journeys'][0]['tags']
        assert len(response['journeys'][0]['sections']) == 2
        assert response['journeys'][0]['duration'] == 1100
        assert response['journeys'][0]['durations']['car'] == 500
        assert response['journeys'][0]['durations']['total'] == 1100
        assert response['journeys'][0]['distances']['car'] == 50

        section_1 = response['journeys'][0]['sections'][0]
        section_2 = response['journeys'][0]['sections'][1]
        assert section_1['from']['address']["name"] == "rue bs"
        assert section_1['to']['address']["name"] == "rue ag"
        assert section_1['to'] == section_2['from']
        assert section_2['from'] == section_2['to']

        assert section_2['type'] == 'park'


from jormungandr.street_network.parking.augeas import Augeas


class MockAugeas(Augeas):
    def _request(self, *_, **__):
        return 42


MOCKED_AUGEAS_WITH_ASGARD_CONF = [
    {
        "modes": ["car"],
        "class": "jormungandr.street_network.with_parking.WithParking",
        "args": {
            "parking": {
                "class": "tests.direct_path_parking_with_asgard_integration_tests.MockAugeas",
                "args": {"service_url": "http://who.cares"},
            },
            "street_network": {
                "class": "tests.direct_path_asgard_integration_tests.MockAsgard",
                "args": {"api_key": "", "asgard_socket": "bob_socket", "timeout": 10},
            },
        },
    },
    {
        "modes": ['walking', 'bike', 'bss'],
        "class": "tests.direct_path_asgard_integration_tests.MockAsgard",
        "args": {
            "costing_options": {"bicycle": {"bicycle_type": "Hybrid"}},
            "api_key": "",
            "asgard_socket": "bob_socket",
            "service_url": "http://bob.com",
            "timeout": 10,
        },
    },
]


@dataset(
    {
        'main_routing_test': {
            'scenario': 'distributed',
            'instance_config': {'street_network': MOCKED_AUGEAS_WITH_ASGARD_CONF},
        }
    }
)
class TestWithParkingAugeasDirectPath(AbstractTestFixture):
    @pytest.mark.xfail
    def test_journey_direct_path(self):
        query = journey_basic_query + "&first_section_mode[]=car" + "&last_section_mode[]=car" + "&debug=true"

        response = self.query_region(query)

        assert len(response['journeys']) == 1

        # car from asgard
        assert 'car' in response['journeys'][0]['tags']
        assert len(response['journeys'][0]['sections']) == 2
        assert response['journeys'][0]['duration'] == 542
        assert response['journeys'][0]['durations']['car'] == 500
        assert response['journeys'][0]['durations']['total'] == 542
        assert response['journeys'][0]['distances']['car'] == 50

        section_1 = response['journeys'][0]['sections'][0]
        section_2 = response['journeys'][0]['sections'][1]
        assert section_1['from']['address']["name"] == "rue bs"
        assert section_1['to']['address']["name"] == "rue ag"
        assert section_1['to'] == section_2['from']
        assert section_2['from'] == section_2['to']

        assert section_2['type'] == 'park'
