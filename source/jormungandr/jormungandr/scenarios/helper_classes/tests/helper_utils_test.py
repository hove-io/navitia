# -*- coding: utf-8 -*-
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


import navitiacommon.response_pb2 as response_pb2
import navitiacommon.type_pb2 as type_pb2
from jormungandr.utils import str_to_time_stamp, PeriodExtremity
from jormungandr.scenarios.helper_classes.helper_utils import _update_fallback_sections
from jormungandr.street_network.tests.streetnetwork_test_utils import make_pt_object
from jormungandr.street_network.street_network import StreetNetworkPathType


def add_section(journey, origin, destination, duration, section_begin_date_time, section_type, mode, vj_uri):
    s = journey.sections.add()
    s.duration = duration
    s.begin_date_time = section_begin_date_time
    s.end_date_time = s.begin_date_time + s.duration
    s.type = section_type
    if origin:
        s.origin.CopyFrom(origin)
    if destination:
        s.destination.CopyFrom(destination)
    if mode:
        s.street_network.mode = mode
    if vj_uri:
        s.vehicle_journey.uri = vj_uri
    journey.duration += s.duration


# This function add 4 sections to a journey from a stop_point to stop_point.
# Sections PUBLIC_TRANSPORT, TRANSFER, WAITING, PUBLIC_TRANSPORT
def add_whole_pt_section(journey, whole_section_begin_date_time):
    origin = make_pt_object(type_pb2.STOP_POINT, 2.0, 2.0, "stop_point_1")
    destination = make_pt_object(type_pb2.STOP_POINT, 3.0, 3.0, "stop_point_2")
    add_section(
        journey,
        origin,
        destination,
        50 * 60,
        whole_section_begin_date_time,
        response_pb2.PUBLIC_TRANSPORT,
        None,
        "vj_toto",
    )

    # Transfert
    add_section(
        journey, None, None, 3 * 60, journey.sections[-1].end_date_time, response_pb2.TRANSFER, None, None
    )

    # Waiting
    add_section(
        journey, None, None, 2 * 60, journey.sections[-1].end_date_time, response_pb2.WAITING, None, None
    )

    origin = make_pt_object(type_pb2.STOP_POINT, 6.0, 6.0, "stop_point_3")
    destination = make_pt_object(type_pb2.STOP_POINT, 7.0, 7.0, "stop_point_4")
    add_section(
        journey,
        origin,
        destination,
        50 * 60,
        journey.sections[-1].end_date_time,
        response_pb2.PUBLIC_TRANSPORT,
        None,
        "vj_tata",
    )


# This function creates a car journey with 2 sections.
# Sections CAR, PARK
def create_car_journey_with_parking(start_date, origin, destination):
    response = response_pb2.Response()
    journey = response.journeys.add()
    journey.departure_date_time = str_to_time_stamp(start_date)
    journey.duration = 0
    journey.nb_transfers = 0

    # Car
    add_section(
        journey,
        origin,
        destination,
        5 * 60,
        journey.departure_date_time,
        response_pb2.STREET_NETWORK,
        response_pb2.Car,
        None,
    )

    # Parking
    add_section(
        journey,
        destination,
        destination,
        2 * 60,
        journey.sections[-1].end_date_time,
        response_pb2.PARK,
        response_pb2.Car,
        None,
    )

    journey.arrival_date_time = journey.sections[-1].end_date_time

    assert len(journey.sections) == 2
    assert journey.departure_date_time == str_to_time_stamp(start_date)
    assert journey.duration == 420
    assert journey.arrival_date_time == journey.departure_date_time + journey.duration

    return response


# This function creates a car journey with 2 sections.
# Sections LEAVE_PARKING, CAR
def create_car_journey_with_leave_parking(start_date, origin, destination):
    response = response_pb2.Response()
    journey = response.journeys.add()
    journey.departure_date_time = str_to_time_stamp(start_date)
    journey.duration = 0
    journey.nb_transfers = 0

    # Leave Parking
    add_section(
        journey,
        origin,
        origin,
        2 * 60,
        journey.departure_date_time,
        response_pb2.LEAVE_PARKING,
        response_pb2.Car,
        None,
    )

    # Car
    add_section(
        journey,
        origin,
        destination,
        5 * 60,
        journey.sections[-1].end_date_time,
        response_pb2.STREET_NETWORK,
        response_pb2.Car,
        None,
    )

    journey.arrival_date_time = journey.sections[-1].end_date_time

    assert len(journey.sections) == 2
    assert journey.departure_date_time == str_to_time_stamp(start_date)
    assert journey.duration == 420
    assert journey.arrival_date_time == journey.departure_date_time + journey.duration

    return response


# This function creates a whole pt journey with a crowfly at the beginning
# Sections CROW_FLY, PUBLIC_TRANSPORT, TRANSFER, WAITING, PUBLIC_TRANSPORT
def create_journey_with_crowfly_and_pt():
    journey = response_pb2.Journey()
    journey.departure_date_time = str_to_time_stamp("20180618T060500")
    journey.duration = 0
    journey.nb_transfers = 1

    # Car crow_fly
    origin = make_pt_object(type_pb2.ADDRESS, 0.0, 0.0, "Chez tonton")
    destination = make_pt_object(type_pb2.ADDRESS, 1.0, 1.0, "Chez tata")
    add_section(
        journey,
        origin,
        destination,
        5 * 60,
        journey.departure_date_time,
        response_pb2.CROW_FLY,
        response_pb2.Car,
        None,
    )

    add_whole_pt_section(journey, journey.sections[-1].end_date_time)

    journey.arrival_date_time = journey.sections[-1].end_date_time

    assert len(journey.sections) == 5
    assert journey.duration == 6600
    assert journey.arrival_date_time == journey.departure_date_time + journey.duration

    return journey


# This function creates a whole pt journey with a crowfly at the end
# Sections PUBLIC_TRANSPORT, TRANSFER, WAITING, PUBLIC_TRANSPORT, CROW_FLY
def create_journey_with_pt_and_crowfly():
    journey = response_pb2.Journey()
    journey.departure_date_time = str_to_time_stamp("20180618T060500")
    journey.duration = 0
    journey.nb_transfers = 1

    add_whole_pt_section(journey, journey.departure_date_time)

    # Car crow_fly
    origin = make_pt_object(type_pb2.ADDRESS, 0.0, 0.0, "Chez tonton")
    destination = make_pt_object(type_pb2.ADDRESS, 1.0, 1.0, "Chez tata")
    add_section(
        journey,
        origin,
        destination,
        5 * 60,
        journey.sections[-1].end_date_time,
        response_pb2.CROW_FLY,
        response_pb2.Car,
        None,
    )

    journey.arrival_date_time = journey.sections[-1].end_date_time

    assert len(journey.sections) == 5
    assert journey.duration == 6600
    assert journey.arrival_date_time == journey.departure_date_time + journey.duration

    return journey


# Tests the insertion of a car section with parking at the beginning of a journey
# containing a crowfly at the beginning
def test_update_fallback_sections_beginning_fallback():
    journey = create_journey_with_crowfly_and_pt()
    origin = make_pt_object(type_pb2.ADDRESS, 9.0, 9.0, "Home")
    destination = make_pt_object(type_pb2.ADDRESS, 12.0, 12.0, "Not Home")
    fallback_dp = create_car_journey_with_parking("20180618T050500", origin, destination)
    fallback_period_extremity = PeriodExtremity(str_to_time_stamp('20180618T050500'), False)
    fallback_type = StreetNetworkPathType.BEGINNING_FALLBACK
    access_point = make_pt_object(type_pb2.ACCESS_POINT, 9.0, 9.0, "access_point_toto")

    _update_fallback_sections(journey, fallback_dp, fallback_period_extremity, fallback_type, access_point)

    # Car + Park + 4 PT
    assert len(journey.sections) == 6
    assert journey.sections[1].destination.uri == "stop_point_1"
    assert journey.sections[1].destination == journey.sections[2].origin
    assert journey.sections[1].vias[0].uri == "access_point_toto"


# Tests the insertion of a car section with leave parking at the end of a journey
# containing a crowfly at the end
def test_update_fallback_sections_ending_fallback():
    journey = create_journey_with_pt_and_crowfly()
    origin = make_pt_object(type_pb2.ADDRESS, 9.0, 9.0, "Not Home")
    destination = make_pt_object(type_pb2.ADDRESS, 12.0, 12.0, "Home")
    fallback_dp = create_car_journey_with_leave_parking("20180618T080500", origin, destination)
    fallback_period_extremity = PeriodExtremity(str_to_time_stamp('20180618T080500'), True)
    fallback_type = StreetNetworkPathType.ENDING_FALLBACK
    access_point = make_pt_object(type_pb2.ACCESS_POINT, 9.0, 9.0, "access_point_toto")

    _update_fallback_sections(journey, fallback_dp, fallback_period_extremity, fallback_type, access_point)

    assert len(journey.sections) == 6
    assert journey.sections[4].origin.uri == "stop_point_4"
    assert journey.sections[4].origin == journey.sections[3].destination
    assert journey.sections[4].vias[0].uri == "access_point_toto"
