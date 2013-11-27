# coding=utf-8
import response_pb2
from functools import partial
from datetime import datetime, timedelta
import logging

#compute the duration to get to the transport plus que transfert
def get_nontransport_duration(journey):
    sections = journey.sections
    current_duration = 0
    for section in sections:
        if section.type == response_pb2.STREET_NETWORK or section.type == response_pb2.TRANSFER:
            current_duration += section.duration
    return current_duration


def has_car(journey):
    for section in journey.sections:
        if section.type == response_pb2.STREET_NETWORK:
            if section.street_network.mode == response_pb2.Car:
                return True
    return False

def min_from_criteria(journey_list, criteria):
    best = None
    for journey in journey_list:
        if best == None:
            best = journey
            continue
        for crit in criteria:
            val = crit(journey, best)
            #we stop at the first criterion that answers
            if val == 0: #0 means the criterion cannot decide
                continue
            if val > 0:
                best = journey
            break
    return best

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

def get_arrival_datetime(journey):
    return datetime.strptime(journey.arrival_date_time, "%Y%m%dT%H%M%S")

def choose_stallion(journeys):
    stallion = None
    for journey in journeys:
        car = has_car(journey)
        if stallion == None or has_car(stallion) and not car:
            stallion = journey #the stallion should not use the car if possible
            continue
        if not car and stallion.arrival_date_time > journey.arrival_date_time :
            stallion = journey
    return stallion


#comparison of 2 fields. 0=>equality, 1=>1 better than 2
field_compare = lambda field_1, field_2 : 0 if field_1 == field_2 else 1 if field_1 < field_2 else -1

#criteria
transfers_crit = lambda journey_1, journey_2 : field_compare(journey_1.nb_transfers, journey_2.nb_transfers)
arrival_crit = lambda journey_1, journey_2 : field_compare(journey_1.arrival_date_time, journey_2.arrival_date_time)
nonTC_crit = lambda journey_1, journey_2 : field_compare(get_nontransport_duration(journey_1), get_nontransport_duration(journey_2))


def qualifier_one(journeys):

    if not journeys:
        logging.info("no journeys to qualify")
        return

    stallion = choose_stallion(journeys)

    assert stallion != None
    #constraints
    journey_length_constraint = lambda journey, max_evolution : journey.duration <= stallion.duration * (1 + max_evolution)
    journey_arrival_constraint = lambda journey, max_mn_shift : get_arrival_datetime(journey) <= get_arrival_datetime(stallion) + timedelta(minutes = max_mn_shift)
    nonTC_relative_constraint = lambda journey, evol : get_nontransport_duration(journey) <= get_nontransport_duration(stallion) * (1 + evol)
    nonTC_abs_constraint = lambda journey, max_mn_shift : get_nontransport_duration(journey) <= get_nontransport_duration(stallion) + max_mn_shift
    trip_caracs = {
                "rapid" : trip_carac([
                                        partial(journey_length_constraint, max_evolution = .10),
                                        partial(journey_arrival_constraint, max_mn_shift = 10),
                                      ],
                                     [
                                        transfers_crit,
                                        arrival_crit,
                                        nonTC_crit
                                     ]
                                     ),
                "healthy" : trip_carac([
                                        partial(journey_length_constraint, max_evolution = .20),
                                        partial(journey_arrival_constraint, max_mn_shift = 20),
                                        partial(nonTC_abs_constraint, max_mn_shift = 20*60),
                                      ],
                                     [
                                        transfers_crit,
                                        arrival_crit,
                                        nonTC_crit
                                     ]
                                     ),
                "comfort" : trip_carac([
                                        partial(journey_length_constraint, max_evolution = .40),
                                        partial(journey_arrival_constraint, max_mn_shift = 40),
                                        partial(nonTC_relative_constraint, evol = -.1),
                                      ],
                                     [
                                        transfers_crit,
                                        nonTC_crit,
                                        arrival_crit,
                                     ]
                                     ),
                }

    for name, carac in trip_caracs.iteritems():
        sublist = filter(and_filters(carac.constraints), journeys)

        best = min_from_criteria(sublist, carac.criteria)

        if best == None:
            continue

        best.type = name


