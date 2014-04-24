import navitiacommon.response_pb2 as response_pb2
from ..default import Script
from..default import are_equals

def empty_journeys_test():
    script = Script()
    response = response_pb2.Response()
    script.sort_journeys(response)
    assert not(response.journeys)


def different_arrival_times_test():
    script = Script()
    response = response_pb2.Response()
    journey1 = response.journeys.add()
    journey1.arrival_date_time = "20140422T0800"
    journey1.duration = 5 * 60
    journey1.nb_transfers = 0
    journey1.sections.add()
    journey1.sections[0].type = response_pb2.PUBLIC_TRANSPORT
    journey1.sections[0].duration = 5 * 60

    journey2 = response.journeys.add()
    journey2.arrival_date_time = "20140422T0758"
    journey2.duration = 2 * 60
    journey2.nb_transfers = 0
    journey2.sections.add()
    journey2.sections[0].type = response_pb2.PUBLIC_TRANSPORT
    journey2.sections[0].duration = 2 * 60

    script.sort_journeys(response)
    assert response.journeys[0].arrival_date_time == "20140422T0758"
    assert response.journeys[1].arrival_date_time == "20140422T0800"


def different_duration_test():
    script = Script()
    response = response_pb2.Response()
    journey1 = response.journeys.add()
    journey1.arrival_date_time = "20140422T0800"
    journey1.duration = 5 * 60
    journey1.nb_transfers = 0
    journey1.sections.add()
    journey1.sections[0].type = response_pb2.PUBLIC_TRANSPORT
    journey1.sections[0].duration = 5 * 60

    journey2 = response.journeys.add()
    journey2.arrival_date_time = "20140422T0800"
    journey2.duration = 3 * 60
    journey2.nb_transfers = 0
    journey2.sections.add()
    journey2.sections[0].type = response_pb2.PUBLIC_TRANSPORT
    journey2.sections[0].duration = 3 * 60

    script.sort_journeys(response)
    assert response.journeys[0].arrival_date_time == "20140422T0800"
    assert response.journeys[1].arrival_date_time == "20140422T0800"
    assert response.journeys[0].duration == 3*60
    assert response.journeys[1].duration == 5*60


def different_nb_transfers_test():
    script = Script()
    response = response_pb2.Response()
    journey1 = response.journeys.add()
    journey1.arrival_date_time = "20140422T0800"
    journey1.duration = 25 * 60
    journey1.nb_transfers = 1
    journey1.sections.add()
    journey1.sections.add()
    journey1.sections.add()
    journey1.sections.add()
    journey1.sections[0].type = response_pb2.PUBLIC_TRANSPORT
    journey1.sections[0].duration = 5 * 60
    journey1.sections[1].type = response_pb2.TRANSFER
    journey1.sections[1].duration = 3 * 60
    journey1.sections[2].type = response_pb2.WAITING
    journey1.sections[2].duration = 2 * 60
    journey1.sections[3].type = response_pb2.PUBLIC_TRANSPORT
    journey1.sections[3].duration = 15 * 60

    journey2 = response.journeys.add()
    journey2.arrival_date_time = "20140422T0800"
    journey2.duration = 25 * 60
    journey2.nb_transfers = 0
    journey2.sections.add()
    journey2.sections[0].type = response_pb2.PUBLIC_TRANSPORT
    journey2.sections[0].duration = 25 * 60

    script.sort_journeys(response)
    assert response.journeys[0].arrival_date_time == "20140422T0800"
    assert response.journeys[1].arrival_date_time == "20140422T0800"
    assert response.journeys[0].duration == 25*60
    assert response.journeys[1].duration == 25*60
    assert response.journeys[0].nb_transfers == 0
    assert response.journeys[1].nb_transfers == 1


def different_duration_non_pt_test():
    script = Script()
    response = response_pb2.Response()
    journey1 = response.journeys.add()
    journey1.arrival_date_time = "20140422T0800"
    journey1.duration = 25 * 60
    journey1.nb_transfers = 1
    journey1.sections.add()
    journey1.sections.add()
    journey1.sections.add()
    journey1.sections.add()
    journey1.sections.add()
    journey1.sections[0].type = response_pb2.PUBLIC_TRANSPORT
    journey1.sections[0].duration = 5 * 60
    journey1.sections[1].type = response_pb2.TRANSFER
    journey1.sections[1].duration = 3 * 60
    journey1.sections[2].type = response_pb2.WAITING
    journey1.sections[2].duration = 2 * 60
    journey1.sections[3].type = response_pb2.PUBLIC_TRANSPORT
    journey1.sections[3].duration = 15 * 60
    journey1.sections[4].type = response_pb2.STREET_NETWORK
    journey1.sections[4].duration = 10 * 60

    journey2 = response.journeys.add()
    journey2.arrival_date_time = "20140422T0800"
    journey2.duration = 25 * 60
    journey2.nb_transfers = 1
    journey2.sections.add()
    journey2.sections.add()
    journey2.sections.add()
    journey2.sections.add()
    journey2.sections[0].type = response_pb2.PUBLIC_TRANSPORT
    journey2.sections[0].duration = 5 * 60
    journey2.sections[1].type = response_pb2.TRANSFER
    journey2.sections[1].duration = 3 * 60
    journey2.sections[2].type = response_pb2.WAITING
    journey2.sections[2].duration = 2 * 60
    journey2.sections[3].type = response_pb2.PUBLIC_TRANSPORT
    journey2.sections[3].duration = 15 * 60

    script.sort_journeys(response)
    assert response.journeys[0].arrival_date_time == "20140422T0800"
    assert response.journeys[1].arrival_date_time == "20140422T0800"
    assert response.journeys[0].duration == 25 * 60
    assert response.journeys[1].duration == 25 * 60
    assert response.journeys[0].nb_transfers == 1
    assert response.journeys[1].nb_transfers == 1

    #We want to have journey2 in first, this is the one with 4 sections
    assert len(response.journeys[0].sections) == 4
    assert len(response.journeys[1].sections) == 5


def create_dummy_journey():
    journey = response_pb2.Journey()
    journey.arrival_date_time = "20140422T0800"
    journey.duration = 25 * 60
    journey.nb_transfers = 1

    s = journey.sections.add()
    s.type = response_pb2.PUBLIC_TRANSPORT
    s.origin.uri = "stop_point_1"
    s.destination.uri = "stop_point_2"
    s.vehicle_journey.uri = "vj_toto"
    s.duration = 5 * 60

    s = journey.sections.add()
    s.type = response_pb2.TRANSFER
    s.duration = 3 * 60

    s = journey.sections.add()
    s.type = response_pb2.WAITING
    s.duration = 2 * 60

    s = journey.sections.add()
    s.type = response_pb2.PUBLIC_TRANSPORT
    s.origin.uri = "stop_point_3"
    s.destination.uri = "stop_point_4"
    s.duration = 15 * 60

    s = journey.sections.add()
    s.type = response_pb2.STREET_NETWORK
    s.duration = 10 * 60

    return journey

def journeys_equality_test_different_journeys():
    """
    test the are_equals method, applied to different journeys
    """
    journey1 = create_dummy_journey()
    modified_section = journey1.sections[0]
    modified_section.origin.uri = "stop_point_10"
    modified_section.destination.uri = "stop_point_22"
    modified_section.vehicle_journey.uri = "vj_tata"

    journey2 = create_dummy_journey()

    assert not are_equals(journey1, journey2)


def journeys_equality_test_different_journeys2():
    """
    test the are_equals method, applied to journeys with
    """
    journey1 = create_dummy_journey()
    modified_section = journey1.sections.add()
    modified_section.origin.uri = "stop_point_10"
    modified_section.destination.uri = "stop_point_22"
    modified_section.vehicle_journey.uri = "vj_tata"

    journey2 = create_dummy_journey()

    assert not are_equals(journey1, journey2)


def journeys_equality_test_same_journeys():
    """No question, a  journey must be equal to self"""
    journey1 = create_dummy_journey()

    assert are_equals(journey1, journey1)

    #and likewise if not the same memory address
    journey2 = create_dummy_journey()
    assert are_equals(journey1, journey2)

def journeys_equality_test_almost_same_journeys():
    """
    test the are_equals method, applied to different journeys, but with meaningless differences
    """
    journey1 = create_dummy_journey()
    modified_section = journey1.sections[4]
    modified_section.polyline = "blablableu"

    journey2 = create_dummy_journey()
    modified_section = journey2.sections[0]
    modified_section.length = 42
    modified_section = journey2.sections[1]
    modified_section.transfer_type = response_pb2.stay_in

    assert are_equals(journey1, journey2)