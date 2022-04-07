# coding=utf-8

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
import navitiacommon.response_pb2 as response_pb2
from functools import partial
from datetime import datetime, timedelta
import logging
from six.moves import filter


# compute the duration to get to the transport plus the transfers duration
def get_nontransport_duration(journey):
    sections = journey.sections
    current_duration = 0
    for section in sections:
        if (
            section.type == response_pb2.STREET_NETWORK
            or section.type == response_pb2.TRANSFER
            or section.type == response_pb2.WAITING
            or section.type == response_pb2.CROW_FLY
        ):
            current_duration += section.duration
    return current_duration


def get_fallback_duration(journey):
    sections = journey.sections
    current_duration = 0
    for section in sections:
        if section.type not in (
            response_pb2.PUBLIC_TRANSPORT,
            response_pb2.WAITING,
            response_pb2.boarding,
            response_pb2.landing,
        ):
            current_duration += section.duration
    return current_duration


def has_fall_back_mode(journey, mode):
    return any(
        s.type in [response_pb2.STREET_NETWORK, response_pb2.CROW_FLY] and s.street_network.mode == mode
        for s in journey.sections
    )


def has_car(journey):
    return has_fall_back_mode(journey, response_pb2.Car)


def has_no_car(journey):
    return not has_fall_back_mode(journey, response_pb2.Car)


def has_bike(journey):
    return has_fall_back_mode(journey, response_pb2.Bike)


def has_no_bike(journey):
    return not has_fall_back_mode(journey, response_pb2.Bike)


def has_walk(journey):
    return has_fall_back_mode(journey, response_pb2.Walking)


def has_bss(journey):
    return any(s.type == response_pb2.BSS_RENT for s in journey.sections)


def has_no_bss(journey):
    return not has_bss(journey)


def non_pt_journey(journey):
    """ check if the journey has not public transport section"""
    has_pt = all(section.type != response_pb2.PUBLIC_TRANSPORT for section in journey.sections)
    return has_pt


def has_pt(journey):
    return not non_pt_journey(journey)


def min_from_criteria(journey_list, criteria):
    best = None
    for journey in journey_list:
        if best is None:
            best = journey
            continue
        for crit in criteria:
            val = crit(journey, best)
            # we stop at the first criterion that answers
            if val == 0:  # 0 means the criterion cannot decide
                continue
            if val > 0:
                best = journey
            break
    return best


# The caracteristic consists in 2 things :
# - a list of constraints
#   the constraints filter a sublist of journey
# - a list of optimisation critera
#   the criteria select the best journey in the sublist
class trip_carac:
    def __init__(self, constraints, criteria):
        self.constraints = constraints
        self.criteria = criteria


class and_filters:
    def __init__(self, filters):
        self.filters = filters

    def __call__(self, value):
        for f in self.filters:
            if not f(value):
                return False
        return True


class and_functors:
    def __init__(self, filters):
        self.filters = filters

    def __call__(self, v1, v2):
        for f in self.filters:
            val = f(v1, v2)
            if val != 0:
                return val
        return 1


def get_arrival_datetime(journey):
    return datetime.utcfromtimestamp(float(journey.arrival_date_time))


def get_departure_datetime(journey):
    return datetime.utcfromtimestamp(float(journey.arrival_date_time))


def best_standard(standard, journey, best_criteria):
    if not standard:
        return journey

    journey_has_pt = has_pt(journey)
    standard_has_pt = has_pt(standard)
    if journey_has_pt != standard_has_pt:
        return standard if standard_has_pt else journey
    journey_has_car = has_car(journey)
    standard_has_car = has_car(standard)
    if journey_has_car != standard_has_car:
        return standard if not standard_has_car else journey
    return standard if best_criteria(standard, journey) > 0 else journey


def choose_standard(journeys, best_criteria):
    standard = None
    for journey in journeys:
        standard = best_standard(standard, journey, best_criteria)
    return standard


# comparison of 2 fields. 0=>equality, 1=>1 better than 2
def compare_minus(field_1, field_2):
    # ordering-comparisons
    # https://docs.python.org/3.4/whatsnew/3.0.html#ordering-comparisons
    return (field_2 > field_1) - (field_2 < field_1)


def compare_field(obj1, obj2, func):
    return compare_minus(func(obj1), func(obj2))


def reverse_compare_field(obj1, obj2, func):
    return compare_minus(func(obj2), func(obj1))


# criteria
def transfers_crit(j_1, j_2):
    return compare_minus(j_1.nb_transfers, j_2.nb_transfers)


def arrival_crit(j_1, j_2):
    return compare_minus(j_1.arrival_date_time, j_2.arrival_date_time)


def departure_crit(j_1, j_2):
    return compare_minus(j_2.departure_date_time, j_1.departure_date_time)


def nonTC_crit(j_1, j_2):
    duration1 = get_fallback_duration(j_1)
    duration2 = get_fallback_duration(j_2)
    return compare_minus(duration1, duration2)


def duration_crit(j_1, j_2):
    return compare_minus(j_1.duration, j_2.duration)


def comfort_crit(j_1, j_2):
    # this filter tends to limit the nb of transfers
    # Case 1:
    #   if j_1 and j_2 have PT sections and nb_transfers ARE NOT equal: return the journey has minimum nb transfer
    # Case 2:
    #   if j_1 and j_2 have PT sections and nb_transfers ARE equal: return the journey has minimum non tc duration
    # Case 3:
    #   if one of journeys is direct_path: return the journey has minimum non tc duration

    transfer = compare_minus(j_1.nb_transfers, j_2.nb_transfers)
    if transfer != 0 and has_pt(j_1) and has_pt(j_2):
        return transfer
    # j_1 is better than j_2 and j_1 is not pt
    return nonTC_crit(j_1, j_2)


def get_ASAP_journey(journeys, req):
    best_crit = arrival_crit if req["clockwise"] else departure_crit
    return min_from_criteria(journeys, [best_crit, duration_crit, transfers_crit, nonTC_crit])
