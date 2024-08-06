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

from navitiacommon import type_pb2, response_pb2
import jormungandr.scenarios.tests.helpers_tests as helpers_tests
from jormungandr.scenarios.utils import (
    fill_disruptions_on_pois,
    fill_disruptions_on_places_nearby,
    update_deeplink_in_odt_information_in_section,
)

import pytest
from pytest_mock import mocker


def update_disruptions_on_pois_for_ptref_test(mocker):
    instance = lambda: None
    # As in navitia, object poi in the response of ptref doesn't have any impact
    response_pois = helpers_tests.get_object_pois_in_ptref_response()
    assert len(response_pois.impacts) == 0
    assert response_pois.pois[0].uri == "poi_uri"
    assert response_pois.pois[0].name == "poi_name_from_kraken"

    # Prepare disruptions on poi as response of end point poi_disruptions of loki
    # pt_object poi as impacted object is absent in the response of poi_disruptions
    disruptions_with_poi = helpers_tests.get_response_with_a_disruption_on_poi()
    assert len(disruptions_with_poi.impacts) == 1
    assert disruptions_with_poi.impacts[0].uri == "test_impact_uri"
    assert len(disruptions_with_poi.impacts[0].impacted_objects) == 1
    pt_object = disruptions_with_poi.impacts[0].impacted_objects[0].pt_object
    helpers_tests.verify_poi_in_impacted_objects(object=pt_object, poi_empty=True)

    mock = mocker.patch('jormungandr.scenarios.utils.get_disruptions_on_poi', return_value=disruptions_with_poi)
    fill_disruptions_on_pois(instance, response_pois)

    # In the final response, we should have a disruption as well as object poi in disruption.
    assert len(response_pois.impacts) == 1
    assert response_pois.pois[0].uri == "poi_uri"
    assert response_pois.pois[0].name == "poi_name_from_kraken"
    assert len(response_pois.impacts[0].impacted_objects) == 1
    object = response_pois.impacts[0].impacted_objects[0].pt_object
    helpers_tests.verify_poi_in_impacted_objects(object=pt_object, poi_empty=False)
    mock.assert_called_once()
    return


def update_disruptions_on_pois_for_places_nearby_test(mocker):
    instance = lambda: None
    # As in navitia, object poi in the response of places_nearby doesn't have any impact
    response_places_nearby = helpers_tests.get_object_pois_in_places_nearby_response()
    assert len(response_places_nearby.impacts) == 0
    assert len(response_places_nearby.places_nearby) == 1
    assert response_places_nearby.places_nearby[0].uri == "poi_uri"
    assert response_places_nearby.places_nearby[0].name == "poi_name_from_kraken"

    # As above Prepare disruptions on poi as response of end point poi_disruptions of loki
    disruptions_with_poi = helpers_tests.get_response_with_a_disruption_on_poi()
    assert len(disruptions_with_poi.impacts) == 1
    assert disruptions_with_poi.impacts[0].uri == "test_impact_uri"
    object = disruptions_with_poi.impacts[0].impacted_objects[0].pt_object
    helpers_tests.verify_poi_in_impacted_objects(object=object, poi_empty=True)

    mock = mocker.patch('jormungandr.scenarios.utils.get_disruptions_on_poi', return_value=disruptions_with_poi)
    fill_disruptions_on_places_nearby(instance, response_places_nearby)

    # In the final response, we should have a disruption as well as object poi in disruption.
    assert len(response_places_nearby.impacts) == 1
    assert response_places_nearby.places_nearby[0].uri == "poi_uri"
    assert response_places_nearby.places_nearby[0].name == "poi_name_from_kraken"
    assert len(response_places_nearby.impacts[0].impacted_objects) == 1
    object = response_places_nearby.impacts[0].impacted_objects[0].pt_object
    helpers_tests.verify_poi_in_impacted_objects(object=object, poi_empty=False)

    mock.assert_called_once()
    return


def journey_with_deeplink_in_odt_information_test():
    instance = lambda: None
    # Get a response with a section of ODT having odt_information.
    deeplink = "https://domaine/search?departure-address={from_name}&destination-address={to_name}" \
               "&requested-departure-time={datetime}&from_coord_lat={from_coord_lat}" \
               "&from_coord_lon={from_coord_lon}&to_coord_lat={to_coord_lat}&to_coord_lon={to_coord_lon}"
    response_journey_with_odt = helpers_tests.get_odt_journey(deeplink=deeplink)
    assert len(response_journey_with_odt.journeys) == 1
    journey = response_journey_with_odt.journeys[0]
    assert len(journey.sections) == 3
    odt_section = journey.sections[1]
    assert odt_section.type == response_pb2.ON_DEMAND_TRANSPORT
    odt_information = odt_section.odt_information
    assert odt_information.name == "odt_name_value"
    assert odt_information.deeplink == "https://domaine/search?departure-address={from_name}&destination-address={to_name}&requested-departure-time={datetime}&from_coord_lat={from_coord_lat}&from_coord_lon={from_coord_lon}&to_coord_lat={to_coord_lat}&to_coord_lon={to_coord_lon}"
    assert odt_information.url == "odt_url_value"
    assert odt_information.conditions == "odt_conditions_value"
    assert odt_information.phone == "odt_phone_value"
    update_deeplink_in_odt_information_in_section(odt_section)
    assert odt_information.deeplink == "https://domaine/search?departure-address=stop_a_name&destination-address=stop_b_name&requested-departure-time=1722924300&from_coord_lat=2.0&from_coord_lon=1.0&to_coord_lat=4.0&to_coord_lon=3.0"

    # Use a deeplink with fewer placeholders
    deeplink = "https://domaine/search?departure-address={from_name}&destination-address={to_name}" \
               "&requested-departure-time={datetime}&from_coord_lat={from_coord_lat}&from_coord_lon={from_coord_lon}"
    response_journey_with_odt = helpers_tests.get_odt_journey(deeplink=deeplink)
    odt_section = response_journey_with_odt.journeys[0].sections[1]
    update_deeplink_in_odt_information_in_section(odt_section)
    assert odt_section.odt_information.deeplink == "https://domaine/search?departure-address=stop_a_name&destination-address=stop_b_name&requested-departure-time=1722924300&from_coord_lat=2.0&from_coord_lon=1.0"

    # Add a placeholder which is not predefined in the function to update deeplink
    # This placeholder will not be replaced(updated)
    deeplink = "https://domaine/search?departure-address={from_name}&destination-address={to_name}" \
               "&requested-departure-time={datetime}&from_coord_lat={from_coord_lat}" \
               "&from_coord_lon={from_coord_lon}&toto={toto}"
    response_journey_with_odt = helpers_tests.get_odt_journey(deeplink=deeplink)
    odt_section = response_journey_with_odt.journeys[0].sections[1]
    update_deeplink_in_odt_information_in_section(odt_section)
    assert odt_section.odt_information.deeplink == "https://domaine/search?departure-address=stop_a_name&destination-address=stop_b_name&requested-departure-time=1722924300&from_coord_lat=2.0&from_coord_lon=1.0&toto=N/A"
