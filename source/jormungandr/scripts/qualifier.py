# coding=utf-8
import type_pb2
import response_pb2

def get_rabattement_duration(journey):
    sections = journey.sections
    current_duration = 0
    if sections[0].type == response_pb2.STREET_NETWORK:
        current_duration = sections[0].duration
    if sections[-1].type == response_pb2.STREET_NETWORK:
        current_duration = current_duration + sections[-1].duration
    return current_duration


def is_car(section):
    if section.type == response_pb2.STREET_NETWORK:
        if section.street_network.mode == response_pb2.Car:
            return True
    return False

def qualifier_one(journeys):

    trip_type = {"rapid" : None, "rapid_plus" : None,
                  "comfort" : None, "healthy" : None}

    if journeys:
        trip_type["rapid"] = journeys[0]

    for journey in journeys:
        if trip_type["rapid"].arrival_date_time > journey.arrival_date_time :
            trip_type["rapid"] = journey

    if trip_type["rapid"]:
        for journey in journeys :
            if (journey.duration < (1.2 * trip_type["rapid"].duration)) and \
                    (journey.nb_transfers < trip_type["rapid"].nb_transfers):
                trip_type["rapid_plus"] = journey

        if trip_type["rapid_plus"]:
            trip_type["rapid"] = trip_type["rapid_plus"]

        rapid_duration = get_rabattement_duration(trip_type["rapid"])
        for journey in journeys:
            if journey == trip_type['rapid']:
                continue

            if journey.duration < (trip_type["rapid"].duration * 1.5):
                current_duration = get_rabattement_duration(journey)
                sections = journey.sections
                if (not is_car(sections[0])) and (not is_car(sections[-1])) \
                        and current_duration > (rapid_duration * 1.1):
                    trip_type["healthy"] = journey
                if journey.nb_transfers < trip_type["rapid"].nb_transfers \
                        or current_duration < rapid_duration:
                    trip_type["comfort"] = journey
        if trip_type["rapid"]:
            trip_type["rapid"].type =  "rapid"
        if trip_type["comfort"]:
            trip_type["comfort"].type = "comfort"
        if trip_type["healthy"]:
            trip_type["healthy"].type = "healthy"

