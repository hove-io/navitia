# coding=utf-8
import type_pb2
import response_pb2
from instance_manager import NavitiaManager

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

    trip_type = {"rapid" : -1, "rapid_plus" : -1,
                  "comfort" : -1, "healthy" : -1}

    if journeys:
        trip_type["rapid"] = 0
    index = -1
    for journey in journeys:
        index = index + 1
        if journeys[trip_type["rapid"]].arrival_date_time > journey.arrival_date_time :
            trip_type["rapid"] = index

    if trip_type["rapid"] > -1 :
        index = -1
        for journey in journeys :
            index = index + 1
            if journey.duration > 1.2*journeys[trip_type["rapid"]].duration and \
                journey.nb_transfers < journeys[trip_type["rapid"]].nb_transfers :
                trip_type["rapid_plus"] = index

        if trip_type["rapid_plus"] > -1 :
            trip_type["rapid"] = trip_type["rapid_plus"]
        index = -1
        rapid_duration = get_rabattement_duration(journeys[trip_type["rapid"]])
        for journey in journeys :
            index = index + 1
            if journey.duration > 1.5*journeys[trip_type["rapid"]].duration :
                current_duration = get_rabattement_duration(journey)
                sections = getattr(journey, "sections")
                if (not is_car(sections[0])) \
                        and (not is_car(sections[-1])) \
                        and current_duration > journey.duration * 0.1:
                    trip_type["healthy"] = index
                if journey.nb_transfers < journeys[trip_type["rapid"]].nb_transfers or \
                        current_duration < rapid_duration:
                    trip_type["comfort"] = index

        if trip_type["rapid"] > -1 :
            journeys[trip_type["rapid"]].type =  "rapid"
        if trip_type["comfort"] > -1 :
            journeys[trip_type["comfort"]].type = "comfort"
        if trip_type["healthy"] > -1 :
            journeys[trip_type["healthy"]].type = "healthy"

