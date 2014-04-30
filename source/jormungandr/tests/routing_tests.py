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


def is_valid_journey_response(response, tester):
    journeys = get_not_null(response, "journeys")

    all_sections = unique_dict('id')
    assert len(journeys) > 0, "we must at least have one journey"
    for j in journeys:
        is_valid_journey(j, tester)

        for s in j['sections']:
            all_sections[s['id']] = s

    # check the fare section
    # the fares must be structurally valid and all link to sections must be ok
    all_tickets = unique_dict('id')
    fares = get_not_null(response, "tickets")
    for f in fares:
        is_valid_ticket(f, tester)
        all_tickets[f['id']] = f

    check_internal_links(response, tester)


    #TODO check journey links (prev/next)


def is_valid_journey(journey, tester):
    #TODO!
    pass


def is_valid_ticket(ticket, tester):
    found = get_not_null(ticket, 'found')
    assert is_valid_bool(found)

    get_not_null(ticket, 'id')
    get_not_null(ticket, 'name')
    cost = get_not_null(ticket, 'cost')
    if found:
        #for found ticket, we must have a non empty currency
        get_not_null(cost, 'currency')

    assert is_valid_float(get_not_null(cost, 'value'))

    check_links(ticket, tester)


#default journey query used in varius test
journey_basic_query = "journeys?from={from_coord}&to={to_coord}&datetime={datetime}"\
    .format(from_coord="0.0000898312;0.0000898312",  # coordinate of S in the dataset
            to_coord="0.00188646;0.00071865",  # coordinate of R in the dataset
            datetime="20120614T080000")


@dataset(["main_routing_test"])
class TestJourneys(AbstractTestFixture):
    """
    Test the structure of the journeys response
    """

    def test_journeys(self):
        #NOTE: we query /v1/coverage/main_routing_test/journeys and not directly /v1/journeys
        #not to use the jormungandr database
        response = self.query_region(journey_basic_query, display=False)

        is_valid_journey_response(response, self.tester)


@dataset([])
class TestJourneysNoRegion(AbstractTestFixture):
    """
    If no region loaded we must have a polite error while asking for a journey
    """

    def test_with_region(self):
        response, status = self.query_no_assert("v1/coverage/non_existent_region/" + journey_basic_query)

        assert status != 200, "the response should not be valid"

        assert response['error']['id'] == "unknown_object"
        assert response['error']['message'] == "The region non_existent_region doesn't exists"

    def test_no_region(self):
        response, status = self.query_no_assert("v1/" + journey_basic_query)

        assert status != 200, "the response should not be valid"

        assert response['error']['id'] == "unknown_object"

        error_regexp = re.compile('^No region available for the coordinates.*')
        assert error_regexp.match(response['error']['message'])


