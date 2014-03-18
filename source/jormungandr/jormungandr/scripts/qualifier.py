# coding=utf-8
import navitiacommon.response_pb2 as response_pb2
from functools import partial
from datetime import datetime, timedelta
import logging


#compute the duration to get to the transport plus que transfert
def get_nontransport_duration(journey):
    sections = journey.sections
    current_duration = 0
    for section in sections:
        if section.type == response_pb2.STREET_NETWORK \
                or section.type == response_pb2.TRANSFER:
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


def get_arrival_datetime(journey):
    return datetime.strptime(journey.arrival_date_time, "%Y%m%dT%H%M%S")


def choose_standard(journeys):
    standard = None
    for journey in journeys:
        car = has_car(journey)
        if standard is None or has_car(standard) and not car:
            standard = journey  # the standard shouldnt use the car if possible
            continue
        if not car and standard.arrival_date_time > journey.arrival_date_time:
            standard = journey
    return standard


#comparison of 2 fields. 0=>equality, 1=>1 better than 2
def compare(field_1, field_2):
    if field_1 == field_2:
        return 0
    elif field_1 < field_2:
        return 1
    else:
        return -1

#criteria
transfers_crit = lambda j_1, j_2: compare(j_1.nb_transfers, j_2.nb_transfers)


def arrival_crit(j_1, j_2):
    return compare(j_1.arrival_date_time, j_2.arrival_date_time)


def nonTC_crit(j_1, j_2):
    duration1 = get_nontransport_duration(j_1)
    duration2 = get_nontransport_duration(j_2)
    return compare(duration1, duration2)


def qualifier_one(journeys):

    if not journeys:
        logging.info("no journeys to qualify")
        return

    standard = choose_standard(journeys)
    assert standard is not None

    #constraints
    def journey_length_constraint(journey, max_evolution):
        max_allow_duration = standard.duration * (1 + max_evolution)
        return journey.duration <= max_allow_duration

    def journey_arrival_constraint(journey, max_mn_shift):
        arrival_date_time = get_arrival_datetime(standard)
        max_date_time = arrival_date_time + timedelta(minutes=max_mn_shift)
        return get_arrival_datetime(journey) <= max_date_time

    def nonTC_relative_constraint(journey, evol):
        transport_duration = get_nontransport_duration(standard)
        max_allow_duration = transport_duration * (1 + evol)
        return get_nontransport_duration(journey) <= max_allow_duration

    def nonTC_abs_constraint(journey, max_mn_shift):
        transport_duration = get_nontransport_duration(standard)
        max_allow_duration = transport_duration + max_mn_shift
        return get_nontransport_duration(journey) <= max_allow_duration

    def no_train(journey):
        ter_uris = ["network:TER", "network:SNCF"]  #TODO share this list
        has_train = any(section.pt_display_informations.uris.network in ter_uris
                        for section in journey.sections)

        return not has_train

    def is_possible_cheap(journey):
        return journey.type == "possible_cheap"

    #definition of the journeys to qualify
    trip_caracs = [
        #the cheap journey, is the fastest one without train
        ("cheap", trip_carac([
            partial(is_possible_cheap),
            partial(no_train),
            #partial(journey_length_constraint, max_evolution=.50),
            #partial(journey_arrival_constraint, max_mn_shift=40),
        ],
            [
                transfers_crit,
                arrival_crit,
                nonTC_crit
            ]
        )),
        ("healthy", trip_carac([
            partial(journey_length_constraint, max_evolution=.20),
            partial(journey_arrival_constraint, max_mn_shift=20),
            partial(nonTC_abs_constraint, max_mn_shift=20 * 60),
        ],
            [
                transfers_crit,
                arrival_crit,
                nonTC_crit
            ]
        )),
        ("comfort", trip_carac([
            partial(journey_length_constraint, max_evolution=.40),
            partial(journey_arrival_constraint, max_mn_shift=40),
            partial(nonTC_relative_constraint, evol=-.1),
        ],
            [
                transfers_crit,
                nonTC_crit,
                arrival_crit,
            ]
        )),
        ("rapid", trip_carac([
            partial(journey_length_constraint, max_evolution=.10),
            partial(journey_arrival_constraint, max_mn_shift=10),
        ],
            [
                transfers_crit,
                arrival_crit,
                nonTC_crit
            ]
        )),
    ]

    for name, carac in trip_caracs:
        sublist = filter(and_filters(carac.constraints), journeys)

        best = min_from_criteria(sublist, carac.criteria)

        if best is None:
            continue

        best.type = name
