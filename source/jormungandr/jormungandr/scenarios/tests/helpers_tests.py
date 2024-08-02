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
from navitiacommon import type_pb2, response_pb2
from jormungandr.scenarios.helpers import (
    is_car_direct_path,
    fill_best_boarding_position,
)
from jormungandr.street_network.tests.streetnetwork_test_utils import make_pt_object
from jormungandr import utils

BEST_BOARDING_POSITIONS = [response_pb2.FRONT, response_pb2.MIDDLE]


def get_walking_walking_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 120
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60

    return journey


def get_walking_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 60

    return journey


def get_walking_bike_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 30
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.duration = 60
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 40

    return journey


def get_walking_bss_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 40
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 120
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 25
    section = journey.sections.add()
    section.type = response_pb2.BSS_RENT
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 55
    section = journey.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section.duration = 20
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 10

    return journey


def get_walking_car_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 40
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 5
    section = journey.sections.add()
    section.type = response_pb2.LEAVE_PARKING
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Car
    section.duration = 70
    section = journey.sections.add()
    section.type = response_pb2.PARK
    section.duration = 10

    return journey


def get_car_walking_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Car
    section.duration = 70
    section = journey.sections.add()
    section.type = response_pb2.PARK
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 5
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 40
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 10

    return journey


def get_car_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Car
    section.duration = 70
    section = journey.sections.add()
    section.type = response_pb2.PARK
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 5

    return journey


def get_bike_walking_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 55
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 80
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 7

    return journey


def get_bike_bike_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 25
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 15
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 5

    return journey


def get_bike_pt_bike_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 25
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 15
    section.pt_display_informations.has_equipments.has_equipments.append(
        type_pb2.hasEquipments.has_bike_accepted
    )
    section.origin.stop_point.has_equipments.has_equipments.append(type_pb2.hasEquipments.has_bike_accepted)
    section.destination.stop_point.has_equipments.has_equipments.append(type_pb2.hasEquipments.has_bike_accepted)
    section = journey.sections.add()
    section.type = response_pb2.CROW_FLY
    section.street_network.mode = response_pb2.Bike
    section.duration = 5

    return journey


def get_bike_bss_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 25
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 15
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 5
    section = journey.sections.add()
    section.type = response_pb2.BSS_RENT
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 50
    section = journey.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 5

    return journey


def get_bike_car_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 25
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 45
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 5
    section = journey.sections.add()
    section.type = response_pb2.LEAVE_PARKING
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Car
    section.duration = 30
    section = journey.sections.add()
    section.type = response_pb2.PARK
    section.duration = 10

    return journey


def get_bss_walking_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.BSS_RENT
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 25
    section = journey.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 5
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 45
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 25

    return journey


def get_bss_bike_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 25
    section = journey.sections.add()
    section.type = response_pb2.BSS_RENT
    section.duration = 5
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 75
    section = journey.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section.duration = 5
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 15
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 120
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 200

    return journey


def get_bss_bss_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 25
    section = journey.sections.add()
    section.type = response_pb2.BSS_RENT
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 45
    section = journey.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 5
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 120
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 15
    section = journey.sections.add()
    section.type = response_pb2.BSS_RENT
    section.duration = 5
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 25
    section = journey.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section.duration = 5
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 25

    return journey


def get_bss_car_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 25
    section = journey.sections.add()
    section.type = response_pb2.BSS_RENT
    section.duration = 5
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Bike
    section.duration = 15
    section = journey.sections.add()
    section.type = response_pb2.BSS_PUT_BACK
    section.duration = 5
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 15
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 120
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 35
    section = journey.sections.add()
    section.type = response_pb2.LEAVE_PARKING
    section.duration = 10
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Car
    section.duration = 70
    section = journey.sections.add()
    section.type = response_pb2.PARK
    section.duration = 10

    return journey


def get_ridesharing_with_crowfly_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.CROW_FLY
    section.street_network.mode = response_pb2.Ridesharing
    section.duration = 25
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 120
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 35
    return journey


def get_ridesharing_with_car_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Ridesharing
    section.duration = 25
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 120
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.duration = 35
    return journey


def is_car_direct_path_test():
    assert not is_car_direct_path(get_walking_journey())
    assert not is_car_direct_path(get_walking_car_journey())
    assert not is_car_direct_path(get_car_walking_journey())
    assert is_car_direct_path(get_car_journey())
    assert not is_car_direct_path(get_bike_car_journey())
    assert not is_car_direct_path(get_bss_car_journey())


def get_pt_journey():
    journey = response_pb2.Journey()

    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section.duration = 15
    return journey


def fill_best_boarding_position_test():
    journey = get_pt_journey()
    fill_best_boarding_position(journey.sections[0], BEST_BOARDING_POSITIONS)
    assert journey.sections[0].best_boarding_positions
    assert response_pb2.BoardingPosition.FRONT in journey.sections[0].best_boarding_positions
    assert response_pb2.BoardingPosition.MIDDLE in journey.sections[0].best_boarding_positions
    assert response_pb2.BoardingPosition.BACK not in journey.sections[0].best_boarding_positions


def get_response_with_a_disruption_on_poi():
    start_period = "20240712T165200"
    end_period = "20240812T165200"
    response = response_pb2.Response()
    impact = response.impacts.add()
    impact.uri = "test_impact_uri"
    impact.disruption_uri = "test_disruption_uri"
    impacted_object = impact.impacted_objects.add()

    # poi = make_pt_object(type_pb2.POI, lon=1, lat=2, uri='poi:test_uri')
    # impacted_object.pt_object.CopyFrom(poi)
    impacted_object.pt_object.name = "poi_name_from_loki"
    impacted_object.pt_object.uri = "poi_uri"
    impacted_object.pt_object.embedded_type = type_pb2.POI
    impact.updated_at = utils.str_to_time_stamp(u'20240712T205200')
    application_period = impact.application_periods.add()
    application_period.begin = utils.str_to_time_stamp(start_period)
    application_period.end = utils.str_to_time_stamp(end_period)

    # Add a message
    message = impact.messages.add()
    message.text = "This is the message sms"
    message.channel.id = "sms"
    message.channel.name = "sms"
    message.channel.content_type = "text"

    # Add a severity
    impact.severity.effect = type_pb2.Severity.UNKNOWN_EFFECT
    impact.severity.name = ' not blocking'
    impact.severity.priority = 1
    impact.contributor = "shortterm.test_poi"

    return response


def get_object_pois_in_ptref_response():
    response = response_pb2.Response()
    poi = response.pois.add()
    poi.uri = "poi_uri"
    poi.name = "poi_name_from_kraken"
    poi.coord.lat = 2
    poi.coord.lon = 1
    poi.poi_type.uri = "poi_type:amenity:parking"
    poi.poi_type.name = "Parking P+R"
    return response


def get_object_pois_in_places_nearby_response():
    response = response_pb2.Response()
    place_nearby = response.places_nearby.add()
    place_nearby.uri = "poi_uri"
    place_nearby.name = "poi_name_from_kraken"
    place_nearby.embedded_type = type_pb2.POI
    place_nearby.poi.uri = "poi_uri"
    place_nearby.poi.name = "poi_name_from_kraken"
    place_nearby.poi.coord.lat = 2
    place_nearby.poi.coord.lon = 1
    place_nearby.poi.poi_type.uri = "poi_type:amenity:parking"
    place_nearby.poi.poi_type.name = "Parking P+R"
    return response


def get_journey_with_pois():
    response = response_pb2.Response()
    journey = response.journeys.add()

    # Walking section from 'stop_a' to 'poi:test_uri'
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.origin.uri = 'stop_a'
    section.origin.embedded_type = type_pb2.STOP_POINT
    section.destination.uri = 'poi_uri'
    section.destination.embedded_type = type_pb2.POI
    section.destination.poi.uri = 'poi_uri'
    section.destination.poi.name = 'poi_name_from_kraken'
    section.destination.poi.coord.lon = 1.0
    section.destination.poi.coord.lat = 2.0

    # Bss section from 'poi:test_uri' to 'poi_b'
    section = journey.sections.add()
    section.street_network.mode = response_pb2.Bss
    section.type = response_pb2.STREET_NETWORK
    section.origin.uri = 'poi_uri'
    section.origin.embedded_type = type_pb2.POI
    section.origin.poi.uri = 'poi_uri'
    section.origin.poi.name = 'poi_name_from_kraken'
    section.origin.poi.coord.lon = 1.0
    section.origin.poi.coord.lat = 2.0
    section.destination.uri = 'poi_b'
    section.destination.embedded_type = type_pb2.POI

    # Walking section from 'poi_b' to 'stop_b'
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section.origin.uri = 'poi_b'
    section.origin.embedded_type = type_pb2.POI
    section.destination.uri = 'stop_b'
    section.destination.embedded_type = type_pb2.STOP_POINT
    return response


def verify_poi_in_impacted_objects(object, poi_empty=True):
    assert object.name == "poi_name_from_loki"
    assert object.uri == "poi_uri"
    assert object.embedded_type == type_pb2.POI
    if poi_empty:
        assert object.poi.uri == ''
        assert object.poi.name == ''
    else:
        assert object.poi.uri == 'poi_uri'
        assert object.poi.name == 'poi_name_from_kraken'
        assert object.poi.coord.lon == 1.0
        assert object.poi.coord.lat == 2.0
