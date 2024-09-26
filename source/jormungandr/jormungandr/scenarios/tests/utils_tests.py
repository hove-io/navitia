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
    update_booking_rule_url_in_section,
)
import pytz
from jormungandr import app
from flask import g
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


def journey_with_booking_url_in_booking_rule_test():
    with app.app_context():
        g.timezone = pytz.timezone("Europe/Paris")
        # Get a response with a section of ODT having booking_rule.
        booking_url = (
            "https://domaine/search?departure-address={from_name}&destination-address={to_name}"
            "&requested-departure-time={departure_datetime}&from_coord_lat={from_coord_lat}"
            "&from_coord_lon={from_coord_lon}&to_coord_lat={to_coord_lat}&to_coord_lon={to_coord_lon}"
        )
        response_journey_with_odt = helpers_tests.get_odt_journey(booking_url=booking_url)
        assert len(response_journey_with_odt.journeys) == 1
        journey = response_journey_with_odt.journeys[0]
        assert len(journey.sections) == 3
        odt_section = journey.sections[1]
        assert odt_section.type == response_pb2.ON_DEMAND_TRANSPORT
        booking_rule = odt_section.booking_rule
        assert booking_rule.name == "odt_name_value"
        assert (
            booking_rule.booking_url
            == "https://domaine/search?departure-address={from_name}&destination-address={to_name}&requested-departure-time={departure_datetime}&from_coord_lat={from_coord_lat}&from_coord_lon={from_coord_lon}&to_coord_lat={to_coord_lat}&to_coord_lon={to_coord_lon}"
        )
        assert booking_rule.info_url == "odt_url_value"
        assert booking_rule.message == "odt_conditions_value"
        assert booking_rule.phone_number == "odt_phone_value"
        update_booking_rule_url_in_section(odt_section)
        assert (
            booking_rule.booking_url
            == "https://domaine/search?departure-address=stop%20a%20name%20(city)&destination-address=stop_b_name%20(city)&requested-departure-time=2024-08-06T08:05:00+0200&from_coord_lat=2.0&from_coord_lon=1.0&to_coord_lat=4.0&to_coord_lon=3.0"
        )

        # Use a booking_url with fewer placeholders
        booking_url = (
            "https://domaine/search?departure-address={from_name}&destination-address={to_name}"
            "&requested-departure-time={departure_datetime}&from_coord_lat={from_coord_lat}&from_coord_lon={from_coord_lon}"
        )
        response_journey_with_odt = helpers_tests.get_odt_journey(booking_url=booking_url)
        odt_section = response_journey_with_odt.journeys[0].sections[1]
        update_booking_rule_url_in_section(odt_section)
        assert (
            odt_section.booking_rule.booking_url
            == "https://domaine/search?departure-address=stop%20a%20name%20(city)&destination-address=stop_b_name%20(city)&requested-departure-time=2024-08-06T08:05:00+0200&from_coord_lat=2.0&from_coord_lon=1.0"
        )

        # Add a placeholder which is not predefined in the function to update url
        # This placeholder will not be replaced(updated)
        booking_url = (
            "https://domaine/search?departure-address={from_name}&destination-address={to_name}"
            "&requested-departure-time={departure_datetime}&from_coord_lat={from_coord_lat}"
            "&from_coord_lon={from_coord_lon}&toto={toto}"
        )
        response_journey_with_odt = helpers_tests.get_odt_journey(booking_url=booking_url)
        odt_section = response_journey_with_odt.journeys[0].sections[1]
        update_booking_rule_url_in_section(odt_section)
        assert (
            odt_section.booking_rule.booking_url
            == "https://domaine/search?departure-address=stop%20a%20name%20(city)&destination-address=stop_b_name%20(city)&requested-departure-time=2024-08-06T08:05:00+0200&from_coord_lat=2.0&from_coord_lon=1.0&toto=N/A"
        )
