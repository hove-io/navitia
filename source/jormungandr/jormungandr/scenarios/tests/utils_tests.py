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

import navitiacommon.type_pb2 as type_pb2
import jormungandr.scenarios.tests.helpers_tests as helpers_tests
from jormungandr.scenarios.utils import fill_disruptions_on_pois, fill_disruptions_on_places_nearby
import pytest
from pytest_mock import mocker

def update_disruptions_on_pois_for_ptref_test(mocker):
    instance = lambda: None
    # As in navitia, object poi in the response of ptref doesn't have any impact
    response_pois = helpers_tests.get_object_pois_in_ptref_response()
    assert len(response_pois.impacts) == 0
    assert response_pois.pois[0].uri == "poi:test_uri"
    assert response_pois.pois[0].name == "test poi"

    # Prepare disruptions on poi as response of end point poi_disruptions of loki
    # pt_object poi as impacted object is absent in the response of poi_disruptions
    disruptions_with_poi = helpers_tests.get_response_with_a_disruption_on_poi()
    assert len(disruptions_with_poi.impacts) == 1
    assert disruptions_with_poi.impacts[0].uri == "test_impact_uri"
    assert len(disruptions_with_poi.impacts[0].impacted_objects) == 1
    assert disruptions_with_poi.impacts[0].impacted_objects[0].pt_object.name == "poi"
    assert disruptions_with_poi.impacts[0].impacted_objects[0].pt_object.uri == "poi:test_uri"
    assert disruptions_with_poi.impacts[0].impacted_objects[0].pt_object.embedded_type == type_pb2.POI
    assert disruptions_with_poi.impacts[0].impacted_objects[0].pt_object.poi.uri == ''
    assert disruptions_with_poi.impacts[0].impacted_objects[0].pt_object.poi.name == ''

    mock = mocker.patch('jormungandr.scenarios.utils.get_disruptions_on_poi', return_value=disruptions_with_poi)
    fill_disruptions_on_pois(instance, response_pois)

    # In the final response, we should have a disruption as well as object poi in disruption.
    assert len(response_pois.impacts) == 1
    assert response_pois.pois[0].uri == "poi:test_uri"
    assert response_pois.pois[0].name == "test poi"
    assert len(response_pois.impacts[0].impacted_objects) == 1
    assert response_pois.impacts[0].impacted_objects[0].pt_object.name == "poi"
    assert response_pois.impacts[0].impacted_objects[0].pt_object.uri == "poi:test_uri"
    assert response_pois.impacts[0].impacted_objects[0].pt_object.embedded_type == type_pb2.POI

    assert response_pois.impacts[0].impacted_objects[0].pt_object.poi.name == 'test poi'
    assert response_pois.impacts[0].impacted_objects[0].pt_object.poi.uri == 'poi:test_uri'
    assert response_pois.impacts[0].impacted_objects[0].pt_object.poi.coord.lon == 1.0
    assert response_pois.impacts[0].impacted_objects[0].pt_object.poi.coord.lat == 2.0

    mock.assert_called_once()
    return

def update_disruptions_on_pois_for_places_nearby_test(mocker):
    instance = lambda: None
    # As in navitia, object poi in the response of places_nearby doesn't have any impact
    response_places_nearby = helpers_tests.get_object_pois_in_places_nearby_response()
    assert len(response_places_nearby.impacts) == 0
    assert len(response_places_nearby.places_nearby) == 1
    assert response_places_nearby.places_nearby[0].uri == "poi:test_uri"
    assert response_places_nearby.places_nearby[0].name == "test poi"


    # As above Prepare disruptions on poi as response of end point poi_disruptions of loki
    disruptions_with_poi = helpers_tests.get_response_with_a_disruption_on_poi()
    assert len(disruptions_with_poi.impacts) == 1
    assert disruptions_with_poi.impacts[0].uri == "test_impact_uri"

    mock = mocker.patch('jormungandr.scenarios.utils.get_disruptions_on_poi', return_value=disruptions_with_poi)
    fill_disruptions_on_places_nearby(instance, response_places_nearby)

    # In the final response, we should have a disruption as well as object poi in disruption.
    assert len(response_places_nearby.impacts) == 1
    assert response_places_nearby.places_nearby[0].uri == "poi:test_uri"
    assert response_places_nearby.places_nearby[0].name == "test poi"
    assert len(response_places_nearby.impacts[0].impacted_objects) == 1
    assert response_places_nearby.impacts[0].impacted_objects[0].pt_object.name == "poi"
    assert response_places_nearby.impacts[0].impacted_objects[0].pt_object.uri == "poi:test_uri"
    assert response_places_nearby.impacts[0].impacted_objects[0].pt_object.embedded_type == type_pb2.POI

    assert response_places_nearby.impacts[0].impacted_objects[0].pt_object.poi.name == 'test poi'
    assert response_places_nearby.impacts[0].impacted_objects[0].pt_object.poi.uri == 'poi:test_uri'
    assert response_places_nearby.impacts[0].impacted_objects[0].pt_object.poi.coord.lon == 1.0
    assert response_places_nearby.impacts[0].impacted_objects[0].pt_object.poi.coord.lat == 2.0

    mock.assert_called_once()
    return
