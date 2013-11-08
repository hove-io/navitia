from ...scripts import qualifier
import type_pb2
import response_pb2
import datetime


#journey.arrival_date_time
#journey.duration
#journey.nb_transfers
#journeys.sections[i].type
#journeys.sections[i].duration
#journey.sections
def qualifier_one_direct_test():
    journeys = []
    journey_direct = response_pb2.Journey()
    journey_direct.arrival_date_time = "201311071512"
    journey_direct.duration = 25*60
    journey_direct.nb_transfers = 0
    journey_direct.sections.add()
    journey_direct.sections.add()

    journey_direct.sections[0].type = response_pb2.STREET_NETWORK
    journey_direct.sections[0].duration = 2*60

    journey_direct.sections[1].type = response_pb2.STREET_NETWORK
    journey_direct.sections[1].duration = 4*60
    journeys.append(journey_direct)

    qualifier.qualifier_one(journeys)

    assert(journey_direct.type == "rapid")
    

def qualifier_two_test():
    journeys = []
    journey_direct = response_pb2.Journey()
    journey_direct.arrival_date_time = "201311071512"
    journey_direct.duration = 25*60
    journey_direct.nb_transfers = 0
    journey_direct.sections.add()
    journey_direct.sections.add()

    journey_direct.sections[0].type = response_pb2.STREET_NETWORK
    journey_direct.sections[0].duration = 2*60

    journey_direct.sections[1].type = response_pb2.STREET_NETWORK
    journey_direct.sections[1].duration = 5*60
    journeys.append(journey_direct)

    journey_rapid = response_pb2.Journey()
    journey_rapid.arrival_date_time = "201311071510"
    journey_rapid.duration = 24*60
    journey_rapid.nb_transfers = 1
    journey_rapid.sections.add()
    journey_rapid.sections.add()

    journey_rapid.sections[0].type = response_pb2.STREET_NETWORK
    journey_rapid.sections[0].duration = 2*60

    journey_rapid.sections[1].type = response_pb2.STREET_NETWORK
    journey_rapid.sections[1].duration = 2*60
    journeys.append(journey_rapid)

    journey_healt = response_pb2.Journey()
    journey_healt.arrival_date_time = "2013110715115"
    journey_healt.duration = 30*60
    journey_healt.nb_transfers = 0
    journey_healt.sections.add()
    journey_healt.sections.add()

    journey_healt.sections[0].type = response_pb2.STREET_NETWORK
    journey_healt.sections[0].duration = 10*60

    journey_healt.sections[1].type = response_pb2.STREET_NETWORK
    journey_healt.sections[1].duration = 10*60
    journeys.append(journey_healt)

    qualifier.qualifier_one(journeys)
    assert len(journeys) == 3
    assert journey_direct.type == "rapid", "type is '%s'" % journey_direct.type
    assert journey_rapid.type == "comfort", "type is '%s'" % journey_rapid.type
    assert journey_healt.type == "healthy", "type is '%s'" % journey_healt.type


def is_car_test():
    section = response_pb2.Section()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Car
    assert(qualifier.is_car(section))

    section.street_network.mode = response_pb2.Walking
    assert(not qualifier.is_car(section))

    section.street_network.mode = response_pb2.Bike
    assert(not qualifier.is_car(section))

    section.street_network.mode = response_pb2.Vls
    assert(not qualifier.is_car(section))

    section.type = response_pb2.PUBLIC_TRANSPORT
    section.street_network.mode = response_pb2.Car
    assert(not qualifier.is_car(section))

    section.type = response_pb2.WAITING
    assert(not qualifier.is_car(section))

    section.type = response_pb2.TRANSFER
    assert(not qualifier.is_car(section))

    section.type = response_pb2.BIRD_FLY
    assert(not qualifier.is_car(section))

    section.type = response_pb2.boarding
    assert(not qualifier.is_car(section))

    section.type = response_pb2.landing
    assert(not qualifier.is_car(section))
