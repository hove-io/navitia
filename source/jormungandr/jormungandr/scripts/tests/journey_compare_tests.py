import navitiacommon.response_pb2 as response_pb2
from ..default import Script

def empty_journeys_test():
    script = Script()
    response = response_pb2.Response()
    script.sort_journeys(response)
    assert(not(response.journeys))


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
    assert(response.journeys[0].arrival_date_time == "20140422T0758")
    assert(response.journeys[1].arrival_date_time == "20140422T0800")


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
    assert(response.journeys[0].arrival_date_time == "20140422T0800")
    assert(response.journeys[1].arrival_date_time == "20140422T0800")
    assert(response.journeys[0].duration == 3*60)
    assert(response.journeys[1].duration == 5*60)


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
    assert(response.journeys[0].arrival_date_time == "20140422T0800")
    assert(response.journeys[1].arrival_date_time == "20140422T0800")
    assert(response.journeys[0].duration == 25*60)
    assert(response.journeys[1].duration == 25*60)
    assert(response.journeys[0].nb_transfers == 0)
    assert(response.journeys[1].nb_transfers == 1)


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
    assert(response.journeys[0].arrival_date_time == "20140422T0800")
    assert(response.journeys[1].arrival_date_time == "20140422T0800")
    assert(response.journeys[0].duration == 25 * 60)
    assert(response.journeys[1].duration == 25 * 60)
    assert(response.journeys[0].nb_transfers == 1)
    assert(response.journeys[1].nb_transfers == 1)
    duration_not_pt1 = duration_not_pt2 = -1
    for journey in [response.journeys[0], response.journeys[1]]:
        duration_not_pt = 0
        for section in journey.sections:
            if section.type != response_pb2.PUBLIC_TRANSPORT:
                duration_not_pt += section.duration
        if duration_not_pt1 == -1:
            duration_not_pt1 = duration_not_pt
        else:
            duration_not_pt2 = duration_not_pt
    assert(duration_not_pt1 < duration_not_pt2)


