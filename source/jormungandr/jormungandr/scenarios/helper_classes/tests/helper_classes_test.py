# -*- coding: utf-8 -*-
# Copyright (c) 2001-2018, Canal TP and/or its affiliates. All rights reserved.
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


from __future__ import absolute_import, print_function, unicode_literals, division
import datetime
import navitiacommon.response_pb2 as response_pb2
import navitiacommon.type_pb2 as type_pb2
from jormungandr.scenarios.distributed import Scenario
from jormungandr.utils import str_to_time_stamp
from jormungandr.street_network.tests.streetnetwork_test_utils import make_pt_object
from jormungandr.utils import PeriodExtremity, SectionSorter
from jormungandr.scenarios.helper_classes.helper_utils import _extend_journey


# This function creates a jouney with 4 sections from a stop_point to stop_point.
# Sections PUBLIC_TRANSPORT, TRANSFER, WAITING, PUBLIC_TRANSPORT
def create_journeys_with_pt():
    journey = response_pb2.Journey()
    journey.departure_date_time = str_to_time_stamp("20180618T060500")
    journey.arrival_date_time = str_to_time_stamp("20180618T075000")
    journey.duration = 105 * 60
    journey.nb_transfers = 1

    s = journey.sections.add()
    s.begin_date_time = journey.departure_date_time
    s.type = response_pb2.PUBLIC_TRANSPORT
    origin = make_pt_object(type_pb2.STOP_POINT, 2.0, 2.0, "stop_point_1")
    s.origin.CopyFrom(origin)
    destination = make_pt_object(type_pb2.STOP_POINT, 3.0, 3.0, "stop_point_2")
    s.destination.CopyFrom(destination)
    s.vehicle_journey.uri = "vj_toto"
    s.duration = 50 * 60
    s.end_date_time = s.begin_date_time + s.duration

    s = journey.sections.add()
    s.type = response_pb2.TRANSFER
    s.duration = 3 * 60
    s.begin_date_time = journey.sections[-2].end_date_time
    s.end_date_time = s.begin_date_time + s.duration

    s = journey.sections.add()
    s.type = response_pb2.WAITING
    s.duration = 2 * 60
    s.begin_date_time = journey.sections[-2].end_date_time
    s.end_date_time = s.begin_date_time + s.duration

    s = journey.sections.add()
    s.type = response_pb2.PUBLIC_TRANSPORT
    s.origin.uri = "stop_point_3"
    s.destination.uri = "stop_point_4"
    s.duration = 50 * 60
    s.begin_date_time = journey.sections[-2].end_date_time
    s.end_date_time = s.begin_date_time + s.duration
    return journey


# This function creates a fallback response with one journey having only one section of CROW_FLY
def create_response_with_crow_fly(start_date, end_date, origin, destination):
    response = response_pb2.Response()
    journey = response.journeys.add()
    journey.departure_date_time = str_to_time_stamp(start_date)
    journey.arrival_date_time = str_to_time_stamp(end_date)
    journey.duration = 5 * 60
    journey.nb_transfers = 0

    s = journey.sections.add()
    s.type = response_pb2.CROW_FLY
    s.origin.CopyFrom(origin)
    s.duration = 5 * 60
    s.begin_date_time = journey.departure_date_time
    s.end_date_time = s.begin_date_time + s.duration

    s.destination.CopyFrom(destination)
    s.street_network.mode = response_pb2.Walking
    return response


# This methode tests building of fallbacks at the begining of a journey.
def extend_journey_for_build_from_test():
    # create a journey with 4 sections having PUBLIC_TRANSPORT at the begining and at the end
    pt_journey = create_journeys_with_pt()

    fallback_extremity = PeriodExtremity(pt_journey.departure_date_time, False)

    # create a response having on single section CROW_FLY journey
    cf_origin = make_pt_object(type_pb2.ADDRESS, 1.0, 1.0, "address_1")
    cf_destination = make_pt_object(type_pb2.STOP_POINT, 2.0, 2.0, "stop_point_1_bis")
    response = create_response_with_crow_fly(start_date="20180618T060000", end_date="20180618T060500",
                                             origin=cf_origin, destination=cf_destination)
    _extend_journey(pt_journey, response, fallback_extremity, is_start_fallback=True)

    pt_journey.sections.sort(SectionSorter())
    assert len(pt_journey.sections) == 5

    # We should have CROW_FLY section at the begining of the journey following by a section PUBLIC_TRANSPORT
    crowfly_section = pt_journey.sections[0]
    pt_section = pt_journey.sections[1]
    assert (crowfly_section.type == response_pb2.CROW_FLY)
    assert (pt_section.type == response_pb2.PUBLIC_TRANSPORT)

    # We should have the same object used for crowfly_section.destination and pt_section.origin
    assert crowfly_section.destination == pt_section.origin
    assert crowfly_section.destination.uri == "stop_point_1"


# This methode tests building of fallbacks at the end of a journey.
def extend_journey_for_build_to_test():
    # create a journey with 4 sections having PUBLIC_TRANSPORT at the begining and at the end
    pt_journey = create_journeys_with_pt()

    fallback_extremity = PeriodExtremity(pt_journey.arrival_date_time, True)

    # create a response having on single section CROW_FLY journey
    cf_origin = make_pt_object(type_pb2.STOP_POINT, 4.0, 4.0, "stop_point_4_bis")
    cf_destination = make_pt_object(type_pb2.ADDRESS, 5.0, 5.0, "address_1")
    response = create_response_with_crow_fly(start_date="20180618T075000", end_date="20180618T075500",
                                             origin=cf_origin, destination=cf_destination)
    _extend_journey(pt_journey, response, fallback_extremity, is_start_fallback=False)

    pt_journey.sections.sort(SectionSorter())
    assert len(pt_journey.sections) == 5

    # We should have CROW_FLY section at the end of last section PUBLIC_TRANSPORT
    crowfly_section = pt_journey.sections[-1]
    pt_section = pt_journey.sections[-2]
    assert (crowfly_section.type == response_pb2.CROW_FLY)
    assert (pt_section.type == response_pb2.PUBLIC_TRANSPORT)

    # We should have the same object used for the last pt_section.destination and crowfly_section.origin
    assert crowfly_section.origin == pt_section.destination
    assert crowfly_section.origin.uri == "stop_point_4"
