# -*- coding: utf-8 -*-
# Copyright (c) 2001-2018, Hove and/or its affiliates. All rights reserved.
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
import navitiacommon.response_pb2 as response_pb2
import navitiacommon.type_pb2 as type_pb2
from jormungandr.utils import str_to_time_stamp
from jormungandr.street_network.tests.streetnetwork_test_utils import make_pt_object


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


def test_finalise_journey_exception():
    from jormungandr.scenarios.helper_classes import helper_exceptions

    e = Exception("a", "b")
    fe = helper_exceptions.FinaliseException(e)
    assert fe.get().error.message == str(("a", "b"))
    assert type(fe.get().error.id) is int
