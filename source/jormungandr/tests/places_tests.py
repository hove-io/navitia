# encoding: utf-8

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
from __future__ import absolute_import, print_function, unicode_literals, division
import logging

from .tests_mechanism import AbstractTestFixture, dataset
from .check_utils import *


@dataset({"main_routing_test": {}})
class TestPlaces(AbstractTestFixture):
    """
    Test places responses
    """

    def test_places_by_id(self):
        """can we get the complete address from coords"""
        # we transform x,y to lon,lat using N_M_TO_DEG constant
        lon = 10.0 / 111319.9
        lat = 100.0 / 111319.9
        response = self.query_region("places/{};{}".format(lon, lat))

        assert len(response['places']) == 1
        is_valid_places(response['places'])
        assert response['places'][0]['name'] == "42 rue kb (Condom)"

    def test_label_of_admin(self):
        """ test label of admin "Condom (03430)" """
        response = self.query_region("places?q=Condom&type[]=administrative_region")

        assert len(response['places']) == 1
        is_valid_places(response['places'])
        assert response['places'][0]['name'] == "Condom (03430)"

    def test_places_invalid_encoding(self):
        _, status = self.query_no_assert(u'/v1/coverage/main_routing_test/places/?q=ch\xe2teau'.encode('utf-8'))
        assert status != 500

    def test_places_do_not_loose_precision(self):
        """do we have a good precision given back in the id"""

        # it should work for any id with 15 digits max on each coords
        # that returns a result
        id = "8.9831195195e-05;0.000898311281954"
        response = self.query_region("places/{}".format(id))

        assert len(response['places']) == 1
        is_valid_places(response['places'])
        assert response['places'][0]['id'] == id
        assert response['places'][0]['address']['id'] == id

    def test_places_nearby(self):
        """check places_nearby"""

        id = "8.9831195195e-05;0.000898311281954"
        response = self.query_region("places/{}/places_nearby".format(id), display=True)

        assert len(response['places_nearby']) > 0
        is_valid_places(response['places_nearby'])

    def test_places_nearby_with_coord(self):
        """check places_nearby with /coord"""

        id = "8.9831195195e-05;0.000898311281954"
        response = self.query_region("coord/{}/places_nearby".format(id))

        assert len(response['places_nearby']) > 0
        is_valid_places(response['places_nearby'])

    def test_places_nearby_with_coords(self):
        """check places_nearby with /coords"""

        id = "8.9831195195e-05;0.000898311281954"
        response = self.query_region("coords/{}/places_nearby".format(id))

        assert len(response['places_nearby']) > 0
        is_valid_places(response['places_nearby'])

        assert len(response['disruptions']) == 0

    def test_places_nearby_with_coord_without_region(self):
        """check places_nearby with /coord"""
        response = self.query("/v1/coord/0.000001;0.000898311281954/places_nearby")
        assert len(response['places_nearby']) > 0
        is_valid_places(response['places_nearby'])

    def test_places_nearby_with_coords_without_region(self):
        """check places_nearby with /coords"""

        response = self.query("/v1/coords/0.000001;0.000898311281954/places_nearby")
        assert len(response['places_nearby']) > 0
        is_valid_places(response['places_nearby'])

    def test_places_nearby_with_coords_without_region_and_type(self):
        """check places_nearby with /coords and type[]=stop_area"""

        response = self.query("/v1/coords/0.000001;0.000898311281954/places_nearby?type[]=stop_area")
        places_nearby = response['places_nearby']
        assert len(places_nearby) == 3
        is_valid_places(places_nearby)
        for place in places_nearby:
            assert place['embedded_type'] == "stop_area"
            assert place['stop_area'].get('line') is None

    def test_places_nearby_with_coords_with_type_and_pagination(self):
        """check the pagination"""

        query = "/v1/coords/0.000001;0.000898311281954/places_nearby?type[]=stop_point&count=1"
        response = self.query(query)
        assert response['pagination']['total_result'] == 4
        places_nearby = response['places_nearby']
        assert len(places_nearby) == 1
        is_valid_places(places_nearby)
        assert places_nearby[0]["embedded_type"] == "stop_point"
        assert places_nearby[0]["id"] == "stop_point:stopB"

        response = self.query(query + "&start_page=1")
        assert response['pagination']['total_result'] == 4
        places_nearby = response['places_nearby']
        assert len(places_nearby) == 1
        is_valid_places(places_nearby)
        assert places_nearby[0]["embedded_type"] == "stop_point"
        assert places_nearby[0]["id"] == "stop_point:stopC"
        assert response['pagination']['items_per_page'] == 1

        # The max count is limited to 1000 even if &count=1100 is used
        query = "/v1/coords/0.000001;0.000898311281954/places_nearby?type[]=stop_point&count=1100"
        response = self.query(query)
        assert response['pagination']['items_on_page'] == 4
        assert response['pagination']['total_result'] == 4
        assert response['pagination']['items_per_page'] == 1000

    def test_places_nearby_with_coords_with_type_filter_and_count(self):
        """
        check places_nearby with /coords and type[]=stop_point, filter=line.uri=D, count=1

        from the coord, you can find:

        stop_B -> 70m
        stop_C -> 72m ( <- it's the stop_point that we are expecting with filter=line.uri=D)
        stop_A -> 121m
        stop_uselessA -> 121m
        """
        query = "/v1/coords/0.000001;0.000898311281954/places_nearby?type[]=stop_point&count=1"
        response = self.query(query)
        places_nearby = response['places_nearby']
        assert len(places_nearby) == 1
        is_valid_places(places_nearby)
        assert places_nearby[0]["embedded_type"] == "stop_point"
        assert places_nearby[0]["id"] == "stop_point:stopB"

        response = self.query(query + "&filter=line.uri=D")
        places_nearby = response['places_nearby']
        assert len(places_nearby) == 1
        is_valid_places(places_nearby)
        assert places_nearby[0]["embedded_type"] == "stop_point"
        assert places_nearby[0]["id"] == "stop_point:stopC"

    def test_places_nearby_with_filter_and_multi_type(self):
        """
        Check api /places_nearby with combination of multi parameter &type[] and &filter
        """
        # places_nearby with /coords and type[]=poi gives 4 pois
        query = "/v1/coords/0.000001;0.000898311281954/places_nearby?type[]=poi"
        response = self.query(query)
        assert len(response.get('places_nearby', [])) == 4
        assert response.get('places_nearby', [])[0]['embedded_type'] == "poi"

        # places_nearby with /coords and type[]=stop_point gives 4 stop_points
        query = "/v1/coords/0.000001;0.000898311281954/places_nearby?type[]=stop_point"
        response = self.query(query)
        assert len(response.get('places_nearby', [])) == 4
        assert response.get('places_nearby', [])[0]['embedded_type'] == "stop_point"

        # places_nearby with /coords and type[]=poi, filter=line.uri=D gives 0 poi (empty response)
        query = "coords/0.000001;0.000898311281954/places_nearby?type[]=poi&filter=line.uri=D"
        response, status = self.query_region(query, check=False)
        assert status == 200
        assert len(response.get('places_nearby', [])) == 0

        # places_nearby with /coords and type[]=stop_point, filter=line.uri=D gives 2 stop_points
        query = "/v1/coords/0.000001;0.000898311281954/places_nearby?type[]=stop_point&filter=line.uri=D"
        response = self.query(query)
        places_nearby = response['places_nearby']
        assert len(places_nearby) == 2
        is_valid_places(places_nearby)
        assert places_nearby[0]['embedded_type'] == "stop_point"
        assert places_nearby[1]['embedded_type'] == "stop_point"

        # places_nearby with /coords and type[]=stop_point&type[]=poi, filter=line.uri=D gives 2 stop_points
        query = (
            "/v1/coords/0.000001;0.000898311281954/places_nearby?type[]=stop_point&type[]=poi&filter=line.uri=D"
        )
        response = self.query(query)
        places_nearby = response['places_nearby']
        assert len(places_nearby) == 2
        is_valid_places(places_nearby)
        assert places_nearby[0]['embedded_type'] == "stop_point"
        assert places_nearby[1]['embedded_type'] == "stop_point"

        # Invalid filter of type unable_to_parse is managed
        query = (
            "/v1/coords/0.000001;0.000898311281954/places_nearby?type[]=stop_point&type[]=poi&filter=toto=tata"
        )
        response = self.query(query)
        assert 'error' in response
        assert 'unable_to_parse' in response['error']['id']
        assert (
            response["error"]["message"] == "Problem while parsing the query:Filter: unable to parse toto=tata"
        )

    def test_places_nearby_with_coords_current_datetime(self):
        """places_nearby with _current_datetime"""

        id = "8.9831195195e-05;0.000898311281954"
        response = self.query_region("coords/{}/places_nearby?_current_datetime=20120815T160000".format(id))

        assert len(response['places_nearby']) > 0
        is_valid_places(response['places_nearby'])

        assert len(response['disruptions']) == 2
        disruptions = get_not_null(response, 'disruptions')
        disrupt = get_disruption(disruptions, 'too_bad')
        assert disrupt['disruption_id'] == 'disruption_on_stop_A'
        messages = get_not_null(disruptions[0], 'messages')

        assert (messages[0]['text']) == 'no luck'
        assert (messages[1]['text']) == 'try again'

    def test_wrong_places_nearby(self):
        """test that a wrongly formated query do not work on places_neaby"""

        lon = 10.0 / 111319.9
        lat = 100.0 / 111319.9
        response, status = self.query_region("bob/{};{}/places_nearby".format(lon, lat), check=False)

        assert status == 404
        # Note: it's not a canonical Navitia error with an Id and a message, but it don't seems to be
        # possible to do this with 404 (handled by flask)
        assert get_not_null(response, 'message')

        # same with a line (it has no meaning)
        response, status = self.query_region("lines/A/places_nearby", check=False)

        assert status == 404
        assert get_not_null(response, 'message')

    def test_non_existent_places_nearby(self):
        """test that a non existing place URI"""
        place_id = "I_am_not_existent"
        response, status = self.query_region("places/{}/places_nearby".format(place_id), check=False)

        assert response["error"]["message"] == "The entry point: {} is not valid".format(place_id)

    def test_non_existent_addresse(self):
        """test that a non existent addresse"""
        place_id = "-1.5348252000000002;47.2554241"
        response, status = self.query_region("places/{}".format(place_id), check=False)
        assert response["error"]["message"] == u'Unable to find place: -1.5348252000000002;47.2554241'

    def test_line_forbidden(self):
        """ test that line is not an allowed type """
        response, status = self.query_region("places?q=A&type[]=line", check=False)

        assert status == 400
        assert 'parameter "type[]" invalid' in response["message"]
        assert 'you gave line' in response["message"]

    def test_places_nearby_with_disable_disruption(self):
        """places_nearby with disable_disruption"""

        id = "8.9831195195e-05;0.000898311281954"
        response = self.query_region("coords/{}/places_nearby?_current_datetime=20120815T160000".format(id))
        assert len(response['places_nearby']) > 0
        is_valid_places(response['places_nearby'])
        assert len(response['disruptions']) == 2

        response = self.query_region(
            "coords/{}/places_nearby?_current_datetime=20120815T160000" "&disable_disruption=true".format(id)
        )
        assert len(response['places_nearby']) > 0
        is_valid_places(response['places_nearby'])
        assert len(response['disruptions']) == 0

        response = self.query_region(
            "coords/{}/places_nearby?_current_datetime=20120815T160000" "&disable_disruption=false".format(id)
        )
        assert len(response['places_nearby']) > 0
        is_valid_places(response['places_nearby'])
        assert len(response['disruptions']) == 2

    def test_main_stop_area_weight_factor(self):
        response = self.query_region("places?type[]=stop_area&q=stop")
        places = response['places']
        assert len(places) == 3
        assert places[0]['id'] == 'stopA'
        assert places[1]['id'] == 'stopB'
        assert places[2]['id'] == 'stopC'

        response = self.query_region("places?type[]=stop_area&q=stop&_main_stop_area_weight_factor=5")
        places = response['places']
        assert len(places) == 3
        assert places[0]['id'] == 'stopC'
        assert places[1]['id'] == 'stopA'
        assert places[2]['id'] == 'stopB'

    def test_disruptions_in_places(self):
        """check disruptions in places"""

        response = self.query_region("places?type[]=stop_area&q=stopA&_current_datetime=20120815T160000")
        places = response['places']
        assert len(places) == 1
        is_valid_places(places)
        assert len(response['disruptions']) == 0

        response = self.query_region(
            "places?type[]=stop_area&q=stopA&_current_datetime=20120815T160000" "&disable_disruption=true"
        )
        places = response['places']
        assert len(places) == 1
        is_valid_places(places)
        assert len(response['disruptions']) == 0

    def test_places_has_no_distance(self):
        """So far, Kraken has no distance returned from its API (as opposed to Bragi)
        We want to make sure that the distance field isn't returned in the response (as it will be zeroed)"
        """

        response = self.query_region("places?q=rue ab")
        places = response['places']
        assert len(places) == 1
        assert 'distance' not in places[0]

    def test_stop_area_attributes_with_different_depth(self):
        """ verify that stop_area contains lines in all apis with depth>2 """
        # API places without depth
        response = self.query_region("places?type[]=stop_area&q=stopA")
        places = response['places']
        assert len(places) == 1
        is_valid_places(places)
        assert places[0]['stop_area'].get('lines') is None

        # API places with depth = 3
        response = self.query_region("places?type[]=stop_area&q=stopA&depth=3")
        places = response['places']
        assert len(places) == 1
        is_valid_places(places)
        lines = places[0]['stop_area'].get('lines')
        assert len(lines) > 1
        # Verify line attributes
        for line in lines:
            is_valid_line(line, depth_check=0)

        # API places_nearby with depth = 3
        response = self.query("/v1/coords/0.000001;0.000898311281954/places_nearby?type[]=stop_area&depth=3")
        places_nearby = response['places_nearby']
        assert len(places_nearby) == 3
        is_valid_places(places_nearby)
        for place in places_nearby:
            assert place['embedded_type'] == "stop_area"
            assert len(place['stop_area'].get('lines')) > 0

    def test_places_with_empty_q(self):
        response, status = self.query_region("places?q=", check=False)
        assert status == 400
        assert response["message"] == u'Search word absent'

    def test_places_with_1025_char(self):
        q = "K" * 1025
        response, status = self.query_region("places?q={q}".format(q=q), check=False)
        assert status == 413
        assert response["message"] == u'Number of characters allowed for the search is 1024'

    def test_places_with_1024_char(self):
        q = "K" * 1024
        response, status = self.query_region("places?q={q}".format(q=q), check=False)
        assert status == 200

    def test_stop_point_attributes_with_different_depth(self):
        """ verify that stop_area contains lines in all apis with depth>2 """
        # API places without depth
        response = self.query_region("places?type[]=stop_point&q=stopA")
        places = response['places']
        assert len(places) == 1
        is_valid_places(places)
        assert places[0]['stop_point'].get('lines') is None

        # API places with depth = 3
        response = self.query_region("places?type[]=stop_point&q=stopA&depth=3")
        places = response['places']
        assert len(places) == 1
        is_valid_places(places)
        lines = places[0]['stop_point'].get('lines')
        assert len(lines) > 1
        # Verify line attributes
        for line in lines:
            is_valid_line(line, depth_check=0)

        # API places_nearby with depth = 3
        response = self.query("/v1/coords/0.000001;0.000898311281954/places_nearby?type[]=stop_point&depth=3")
        places_nearby = response['places_nearby']
        assert len(places_nearby) == 4
        is_valid_places(places_nearby)
        for place in places_nearby:
            assert place['embedded_type'] == "stop_point"
            # stop_point stop_point:uselessA doesn't have any lines
            if place['stop_point'].get('id') != "stop_point:uselessA":
                assert len(place['stop_point'].get('lines')) > 0
            else:
                assert place['stop_point'].get('lines') is None


@dataset({"basic_routing_test": {}})
class TestAdministrativesRegions(AbstractTestFixture):
    def test_administrative_regions_are_ordered_by_level(self):
        """
        In kraken's data, administrative regions aren't sorted.
        At serialization time, we should order them by level ascending
        """
        response = self.query_region("stop_areas/A")
        admins = response['stop_areas'][0]['administrative_regions']
        levels = [admin['level'] for admin in admins]
        assert levels[0] < levels[1] < levels[2]
