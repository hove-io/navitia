# coding=utf-8

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

from __future__ import absolute_import, print_function, unicode_literals, division
import navitiacommon.response_pb2 as response_pb2
from functools import partial
from datetime import datetime, timedelta
import logging
from six.moves import filter


#compute the duration to get to the transport plus the transfers duration
def get_nontransport_duration(journey):
    sections = journey.sections
    current_duration = 0
    for section in sections:
        if section.type == response_pb2.STREET_NETWORK \
                or section.type == response_pb2.TRANSFER \
                or section.type == response_pb2.WAITING \
                or section.type == response_pb2.CROW_FLY:
            current_duration += section.duration
    return current_duration

def get_fallback_duration(journey):
    sections = journey.sections
    current_duration = 0
    for section in sections:
        if section.type not in (response_pb2.PUBLIC_TRANSPORT, response_pb2.WAITING,
                response_pb2.boarding, response_pb2.landing):
            current_duration += section.duration
    return current_duration

def has_fall_back_mode(journey, mode):
    return any(s.type in [response_pb2.STREET_NETWORK, response_pb2.CROW_FLY] and
            s.street_network.mode == mode for s in journey.sections)


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
    has_pt = all(section.type != response_pb2.PUBLIC_TRANSPORT
                 for section in journey.sections)
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
            #we stop at the first criterion that answers
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


#comparison of 2 fields. 0=>equality, 1=>1 better than 2
def compare_minus(field_1, field_2):
    # ordering-comparisons
    # https://docs.python.org/3.4/whatsnew/3.0.html#ordering-comparisons
    return (field_2 > field_1) - (field_2 < field_1)


def compare_field(obj1, obj2, func):
    return compare_minus(func(obj1), func(obj2))


def reverse_compare_field(obj1, obj2, func):
    return compare_minus(func(obj2), func(obj1))


#criteria
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


def qualifier_one(journeys, request_type):
    logger = logging.getLogger(__name__)

    if not journeys:
        logger.info("no journeys to qualify")
        return

    #The request type is made from the datetime_represents params of the request
    #If it's arrival, we want the earliest arrival time
    #If it's departure, we want the tardiest departure time
    best_crit = arrival_crit if request_type == "arrival" else departure_crit

    standard = choose_standard(journeys, and_functors([
        best_crit,
        duration_crit,
        transfers_crit,
        nonTC_crit]))
    assert standard is not None
    standard.tags.append('standard')

    #constraints
    def journey_length_constraint(journey, max_evolution):
        max_allow_duration = standard.duration * (1 + max_evolution)
        return journey.duration <= max_allow_duration

    def journey_arrival_constraint(journey, max_mn_shift):
        arrival_date_time = get_arrival_datetime(standard)
        max_date_time = arrival_date_time + timedelta(minutes=max_mn_shift)
        return get_arrival_datetime(journey) <= max_date_time

    def journey_departure_constraint(journey, max_mn_shift):
        departure_date_time = get_departure_datetime(standard)
        max_date_time = departure_date_time - timedelta(minutes=max_mn_shift)
        return get_departure_datetime(journey) >= max_date_time

    def journey_goal_constraint(journey, max_mn_shift, r_type):
        if r_type == "arrival":
            return journey_arrival_constraint(journey, max_mn_shift)
        else:
            return journey_departure_constraint(journey, max_mn_shift)


    def nonTC_relative_constraint(journey, evol):
        transport_duration = get_nontransport_duration(standard)
        max_allow_duration = transport_duration * (1 + evol)
        return get_nontransport_duration(journey) <= max_allow_duration

    def nb_transfers_constraint(journey, delta_transfers):
        return journey.nb_transfers <= standard.nb_transfers + delta_transfers

    def nonTC_abs_constraint(journey, max_allow_duration):
        duration = get_nontransport_duration(journey)
        return duration <= max_allow_duration

    max_waiting_duration = 3600*4
    # definition of the journeys to qualify
    # the last defined carac will take over the first one
    # if a journey is eligible to multiple tags
    trip_caracs = [
        # comfort tends to limit the number of transfers and fallback
        ("comfort", trip_carac([
            partial(has_no_car),
            partial(journey_length_constraint, max_evolution=.40),
            partial(journey_goal_constraint, max_mn_shift=40, r_type=request_type),
            partial(nonTC_abs_constraint, max_allow_duration=max_waiting_duration),
        ],
            [
                transfers_crit,
                nonTC_crit,
                best_crit,
                duration_crit
            ]
        )),
        # for car we want at most one journey, the earliest one
        ("car", trip_carac([
            partial(has_car),
            partial(has_pt),  # We don't want car only solution, we MUST have PT
            partial(nonTC_abs_constraint, max_allow_duration=max_waiting_duration)
        ],
            [
                best_crit,
                transfers_crit,
                nonTC_crit,
                duration_crit
            ]
        )),
        # less_fallback tends to limit the fallback while walking
        ("less_fallback_walk", trip_carac([
            partial(has_no_car),
            partial(has_no_bike),
            partial(journey_length_constraint, max_evolution=.40),
            partial(journey_goal_constraint, max_mn_shift=40, r_type=request_type),
            partial(nb_transfers_constraint, delta_transfers=1),
            partial(nonTC_abs_constraint, max_allow_duration=max_waiting_duration),
        ],
            [
                nonTC_crit,
                transfers_crit,
                duration_crit,
                best_crit,
            ]
        )),
        # less_fallback tends to limit the fallback for biking and bss
        ("less_fallback_bike", trip_carac([
            partial(has_no_car),
            partial(has_bike),
            partial(has_no_bss),
            partial(journey_length_constraint, max_evolution=.40),
            partial(journey_goal_constraint, max_mn_shift=40, r_type=request_type),
            partial(nb_transfers_constraint, delta_transfers=1),
            partial(nonTC_abs_constraint, max_allow_duration=max_waiting_duration),
        ],
            [
                nonTC_crit,
                transfers_crit,
                duration_crit,
                best_crit,
            ]
        )),
        # less_fallback tends to limit the fallback for biking and bss
        ("less_fallback_bss", trip_carac([
            partial(has_no_car),
            partial(has_bss),
            partial(journey_length_constraint, max_evolution=.40),
            partial(journey_goal_constraint, max_mn_shift=40, r_type=request_type),
            partial(nb_transfers_constraint, delta_transfers=1),
            partial(nonTC_abs_constraint, max_allow_duration=max_waiting_duration),
        ],
            [
                nonTC_crit,
                transfers_crit,
                duration_crit,
                best_crit,
            ]
        )),
        # the fastest is quite explicit
        ("fastest", trip_carac([
            partial(has_no_car),
            partial(journey_goal_constraint, max_mn_shift=10, r_type=request_type),
            partial(nb_transfers_constraint, delta_transfers=1),
            partial(nonTC_abs_constraint, max_allow_duration=max_waiting_duration),
        ],
            [
                duration_crit,
                transfers_crit,
                nonTC_crit,
                best_crit,
            ]
        )),
        # the rapid is what we consider to be the best one
        ("rapid", trip_carac([
            partial(has_no_car),
            partial(has_pt),
            partial(journey_length_constraint, max_evolution=.10),
            partial(journey_goal_constraint, max_mn_shift=10, r_type=request_type),
            partial(nonTC_abs_constraint, max_allow_duration=max_waiting_duration),
        ],
            [
                transfers_crit,
                duration_crit,
                best_crit,
                nonTC_crit
            ]
        )),
        # the non_pt journeys is the earliest journey without any public transport
        ("non_pt_walk", trip_carac([
            partial(non_pt_journey),
            partial(has_no_car),
            partial(has_walk)
        ],
            [
                best_crit
            ]
        )),
        # the non_pt journey is the earliest journey without any public transport
        # only walking, biking or driving
        ("non_pt_bike", trip_carac([
            partial(non_pt_journey),
            partial(has_no_car),
            partial(has_bike)
        ],
            [
                best_crit
            ]
        )),
        ("non_pt_bss", trip_carac([
            partial(non_pt_journey),
            partial(has_no_car),
            partial(has_bss),
        ],
            [
                best_crit
            ]
        )),
    ]

    for name, carac in trip_caracs:
        sublist = list(filter(and_filters(carac.constraints), journeys))

        best = min_from_criteria(sublist, carac.criteria)

        if best is None:
            continue

        best.type = name
