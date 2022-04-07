# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
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
from copy import deepcopy
from jormungandr.scenarios import journey_filter as jf
from jormungandr.scenarios.utils import DepartureJourneySorter, ArrivalJourneySorter
import navitiacommon.response_pb2 as response_pb2
from jormungandr.scenarios.new_default import sort_journeys
from jormungandr.utils import str_to_time_stamp
import random
import itertools
import functools


def empty_journeys_test():
    response = response_pb2.Response()
    sort_journeys(response, 'arrival_time', True)
    assert not response.journeys


def different_arrival_times_test():
    response = response_pb2.Response()
    journey1 = response.journeys.add()
    journey1.arrival_date_time = str_to_time_stamp("20140422T0800")
    journey1.duration = 5 * 60
    journey1.nb_transfers = 0
    journey1.sections.add()
    journey1.sections[0].type = response_pb2.PUBLIC_TRANSPORT
    journey1.sections[0].duration = 5 * 60

    journey2 = response.journeys.add()
    journey2.arrival_date_time = str_to_time_stamp("20140422T0758")
    journey2.duration = 2 * 60
    journey2.nb_transfers = 0
    journey2.sections.add()
    journey2.sections[0].type = response_pb2.PUBLIC_TRANSPORT
    journey2.sections[0].duration = 2 * 60

    sort_journeys(response, 'arrival_time', True)
    assert response.journeys[0].arrival_date_time == str_to_time_stamp("20140422T0758")
    assert response.journeys[1].arrival_date_time == str_to_time_stamp("20140422T0800")


def different_departure_times_test():
    response = response_pb2.Response()
    journey1 = response.journeys.add()
    journey1.departure_date_time = str_to_time_stamp("20140422T0800")
    journey1.duration = 5 * 60
    journey1.nb_transfers = 0
    journey1.sections.add()
    journey1.sections[0].type = response_pb2.PUBLIC_TRANSPORT
    journey1.sections[0].duration = 5 * 60

    journey2 = response.journeys.add()
    journey2.departure_date_time = str_to_time_stamp("20140422T0758")
    journey2.duration = 2 * 60
    journey2.nb_transfers = 0
    journey2.sections.add()
    journey2.sections[0].type = response_pb2.PUBLIC_TRANSPORT
    journey2.sections[0].duration = 2 * 60

    sort_journeys(response, 'departure_time', True)
    assert response.journeys[0].departure_date_time == str_to_time_stamp("20140422T0758")
    assert response.journeys[1].departure_date_time == str_to_time_stamp("20140422T0800")


def different_duration_test():
    response = response_pb2.Response()
    journey1 = response.journeys.add()
    journey1.arrival_date_time = str_to_time_stamp("20140422T0800")
    journey1.duration = 5 * 60
    journey1.nb_transfers = 0
    journey1.sections.add()
    journey1.sections[0].type = response_pb2.PUBLIC_TRANSPORT
    journey1.sections[0].duration = 5 * 60

    journey2 = response.journeys.add()
    journey2.arrival_date_time = str_to_time_stamp("20140422T0800")
    journey2.duration = 3 * 60
    journey2.nb_transfers = 0
    journey2.sections.add()
    journey2.sections[0].type = response_pb2.PUBLIC_TRANSPORT
    journey2.sections[0].duration = 3 * 60

    sort_journeys(response, 'arrival_time', True)
    assert response.journeys[0].arrival_date_time == str_to_time_stamp("20140422T0800")
    assert response.journeys[1].arrival_date_time == str_to_time_stamp("20140422T0800")
    assert response.journeys[0].duration == 3 * 60
    assert response.journeys[1].duration == 5 * 60


def different_nb_transfers_test():
    response = response_pb2.Response()
    journey1 = response.journeys.add()
    journey1.arrival_date_time = str_to_time_stamp("20140422T0800")
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
    journey2.arrival_date_time = str_to_time_stamp("20140422T0800")
    journey2.duration = 25 * 60
    journey2.nb_transfers = 0
    journey2.sections.add()
    journey2.sections[0].type = response_pb2.PUBLIC_TRANSPORT
    journey2.sections[0].duration = 25 * 60

    sort_journeys(response, 'arrival_time', True)
    assert response.journeys[0].arrival_date_time == str_to_time_stamp("20140422T0800")
    assert response.journeys[1].arrival_date_time == str_to_time_stamp("20140422T0800")
    assert response.journeys[0].duration == 25 * 60
    assert response.journeys[1].duration == 25 * 60
    assert response.journeys[0].nb_transfers == 0
    assert response.journeys[1].nb_transfers == 1


def different_duration_non_pt_test():
    response = response_pb2.Response()
    journey1 = response.journeys.add()
    journey1.arrival_date_time = str_to_time_stamp("20140422T0800")
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
    journey2.arrival_date_time = str_to_time_stamp("20140422T0800")
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

    sort_journeys(response, 'arrival_time', True)
    assert response.journeys[0].arrival_date_time == str_to_time_stamp("20140422T0800")
    assert response.journeys[1].arrival_date_time == str_to_time_stamp("20140422T0800")
    assert response.journeys[0].duration == 25 * 60
    assert response.journeys[1].duration == 25 * 60
    assert response.journeys[0].nb_transfers == 1
    assert response.journeys[1].nb_transfers == 1

    # We want to have journey2 in first, this is the one with 4 sections
    assert len(response.journeys[0].sections) == 4
    assert len(response.journeys[1].sections) == 5


def create_dummy_journey():
    journey = response_pb2.Journey()
    journey.arrival_date_time = str_to_time_stamp("20140422T0800")
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


def journey_pairs_gen(list_responses):
    return itertools.combinations(jf.get_qualified_journeys(list_responses), 2)


def test_get_qualified_journeys():
    responses = [response_pb2.Response()]
    journey1 = responses[0].journeys.add()
    journey1.tags.append("a_tag")

    journey2 = responses[0].journeys.add()
    journey2.tags.append("to_delete")

    journey3 = responses[0].journeys.add()
    journey3.tags.append("another_tag")
    journey3.tags.append("to_delete")

    for qualified in jf.get_qualified_journeys(responses):
        assert qualified.tags[0] == 'a_tag'


def test_num_qualifed_journeys():
    responses = [response_pb2.Response()]
    journey1 = responses[0].journeys.add()
    journey1.tags.append("a_tag")

    journey2 = responses[0].journeys.add()
    journey2.tags.append("to_delete")

    journey3 = responses[0].journeys.add()
    journey3.tags.append("another_tag")

    assert jf.nb_qualifed_journeys(responses) == 2


def test_similar_journeys():

    responses = [response_pb2.Response()]
    journey1 = responses[0].journeys.add()
    journey1.sections.add()
    journey1.duration = 42
    journey1.sections[0].uris.vehicle_journey = 'bob'

    journey2 = responses[0].journeys.add()
    journey2.sections.add()
    journey2.duration = 43
    journey2.sections[0].uris.vehicle_journey = 'bob'

    jf.filter_similar_vj_journeys(list(journey_pairs_gen(responses)), {})
    assert len(list(jf.get_qualified_journeys(responses))) == 1


def test_similar_journeys_test2():

    responses = [response_pb2.Response()]
    journey1 = responses[0].journeys.add()
    journey1.sections.add()
    journey1.duration = 42
    journey1.sections[0].uris.vehicle_journey = 'bob'

    responses.append(response_pb2.Response())
    journey2 = responses[-1].journeys.add()
    journey2.sections.add()
    journey2.duration = 43
    journey2.sections[-1].uris.vehicle_journey = 'bob'

    jf.filter_similar_vj_journeys(list(journey_pairs_gen(responses)), {})

    assert len(list(jf.get_qualified_journeys(responses))) == 1


def test_similar_journeys_test3():

    responses = [response_pb2.Response()]
    journey1 = responses[0].journeys.add()
    journey1.sections.add()
    journey1.duration = 42
    journey1.sections[0].uris.vehicle_journey = 'bob'

    responses.append(response_pb2.Response())
    journey2 = responses[-1].journeys.add()
    journey2.sections.add()
    journey2.duration = 43
    journey2.sections[-1].uris.vehicle_journey = 'bobette'

    jf.filter_similar_vj_journeys(list(journey_pairs_gen(responses)), {})

    assert 'to_delete' not in journey1.tags
    assert 'to_delete' in journey2.tags


def test_similar_journeys_different_transfer():
    """
     If 2 journeys take the same vjs but with a different number of sections,
     one should be filtered
    """
    responses = [response_pb2.Response()]
    journey1 = responses[0].journeys.add()
    journey1.sections.add()
    journey1.duration = 42
    journey1.sections[-1].uris.vehicle_journey = 'bob'
    journey1.sections.add()
    journey1.duration = 42
    journey1.sections[-1].uris.vehicle_journey = 'bobette'

    responses.append(response_pb2.Response())
    journey2 = responses[-1].journeys.add()
    journey2.sections.add()
    journey2.duration = 43
    journey2.sections[-1].uris.vehicle_journey = 'bob'
    journey2.sections.add()
    journey2.duration = 43
    journey2.sections[-1].type = response_pb2.TRANSFER
    journey2.sections.add()
    journey2.duration = 43
    journey2.sections[-1].uris.vehicle_journey = 'bobette'

    jf.filter_similar_vj_journeys(journey_pairs_gen(responses), {})

    assert 'to_delete' not in journey1.tags
    assert 'to_delete' in journey2.tags


def test_similar_journeys_different_waiting_durations():
    """
     If 2 journeys take the same vj, same number of sections but with different waiting durations,
     filter one with smaller waiting duration
    """
    responses = [response_pb2.Response()]
    journey1 = responses[0].journeys.add()
    journey1.duration = 600
    journey1.sections.add()
    journey1.sections[-1].uris.vehicle_journey = 'bob'
    journey1.sections[-1].duration = 200
    journey1.sections.add()
    journey1.sections[-1].type = response_pb2.TRANSFER
    journey1.sections[-1].duration = 50
    journey1.sections.add()
    journey1.sections[-1].type = response_pb2.WAITING
    journey1.sections[-1].duration = 150
    journey1.sections.add()
    journey1.sections[-1].uris.vehicle_journey = 'bobette'
    journey1.sections[-1].duration = 200

    responses.append(response_pb2.Response())
    journey2 = responses[-1].journeys.add()
    journey2.duration = 600
    journey2.sections.add()
    journey2.sections[-1].uris.vehicle_journey = 'bob'
    journey2.sections[-1].duration = 200
    journey2.sections.add()
    journey2.sections[-1].type = response_pb2.TRANSFER
    journey2.sections[-1].duration = 25
    journey2.sections.add()
    journey2.sections[-1].type = response_pb2.WAITING
    journey2.sections[-1].duration = 175
    journey2.sections.add()
    journey2.sections[-1].uris.vehicle_journey = 'bobette'
    journey2.sections[-1].duration = 200

    jf.filter_similar_vj_journeys(journey_pairs_gen(responses), {})

    assert 'to_delete' not in journey2.tags
    assert 'to_delete' in journey1.tags


def test_similar_journeys_multi_trasfer_and_different_waiting_durations():
    """
     If 2 journeys take the same vj, same number of sections and several waitings with different waiting durations,
     for each journey find "min waiting duration"
     keep the journey which has larger "min waiting duration"
    """
    responses = [response_pb2.Response()]
    journey1 = responses[0].journeys.add()
    journey1.duration = 1000
    journey1.sections.add()
    journey1.sections[-1].uris.vehicle_journey = 'bob'
    journey1.sections[-1].duration = 200
    journey1.sections.add()
    journey1.sections[-1].type = response_pb2.TRANSFER
    journey1.sections[-1].duration = 50
    journey1.sections.add()
    journey1.sections[-1].type = response_pb2.WAITING
    journey1.sections[-1].duration = 150
    journey1.sections.add()
    journey1.sections[-1].uris.vehicle_journey = 'bobette'
    journey1.sections[-1].duration = 200
    journey1.sections.add()
    journey1.sections[-1].type = response_pb2.TRANSFER
    journey1.sections[-1].duration = 10
    journey1.sections.add()
    journey1.sections[-1].type = response_pb2.WAITING
    journey1.sections[-1].duration = 190
    journey1.sections.add()
    journey1.sections[-1].uris.vehicle_journey = 'boby'
    journey1.sections[-1].duration = 200

    responses.append(response_pb2.Response())
    journey2 = responses[-1].journeys.add()
    journey2.duration = 1000
    journey2.sections.add()
    journey2.sections[-1].uris.vehicle_journey = 'bob'
    journey2.sections[-1].duration = 200
    journey2.sections.add()
    journey2.sections[-1].type = response_pb2.TRANSFER
    journey2.sections[-1].duration = 20
    journey2.sections.add()
    journey2.sections[-1].type = response_pb2.WAITING
    journey2.sections[-1].duration = 180
    journey2.sections.add()
    journey2.sections[-1].uris.vehicle_journey = 'bobette'
    journey2.sections[-1].duration = 200
    journey2.sections.add()
    journey2.sections[-1].type = response_pb2.TRANSFER
    journey2.sections[-1].duration = 100
    journey2.sections.add()
    journey2.sections[-1].type = response_pb2.WAITING
    journey2.sections[-1].duration = 100
    journey2.sections.add()
    journey2.sections[-1].uris.vehicle_journey = 'boby'
    journey2.sections[-1].duration = 200

    jf.filter_similar_vj_journeys(list(journey_pairs_gen(responses)), {})

    assert 'to_delete' not in journey1.tags
    assert 'to_delete' in journey2.tags


def test_similar_journeys_with_and_without_waiting_section():
    """
     If 2 journeys take the same vj, one with a waiting section and another without,
     filtere one with transfer but without waiting section
    """
    responses = [response_pb2.Response()]
    journey1 = responses[0].journeys.add()
    journey1.duration = 600
    journey1.sections.add()
    journey1.sections[-1].uris.vehicle_journey = 'bob'
    journey1.sections[-1].duration = 200
    journey1.sections.add()
    journey1.sections[-1].type = response_pb2.TRANSFER
    journey1.sections[-1].duration = 50
    journey1.sections.add()
    journey1.sections[-1].type = response_pb2.WAITING
    journey1.sections[-1].duration = 150
    journey1.sections.add()
    journey1.sections[-1].uris.vehicle_journey = 'bobette'
    journey1.sections[-1].duration = 200

    responses.append(response_pb2.Response())
    journey2 = responses[-1].journeys.add()
    journey2.duration = 600
    journey2.sections.add()
    journey2.sections[-1].uris.vehicle_journey = 'bob'
    journey2.sections[-1].duration = 200
    journey2.sections.add()
    journey2.sections[-1].type = response_pb2.TRANSFER
    journey2.sections[-1].duration = 200
    journey2.sections.add()
    journey2.sections[-1].uris.vehicle_journey = 'bobette'
    journey2.sections[-1].duration = 200

    jf.filter_similar_vj_journeys(list(journey_pairs_gen(responses)), {})

    assert 'to_delete' not in journey1.tags
    assert 'to_delete' in journey2.tags


def test_similar_journeys_walking_bike():
    """
    If we have 2 direct path, one walking and one by bike, we should
     not filter any journey
    """
    responses = [response_pb2.Response()]
    journey1 = responses[0].journeys.add()
    journey1.duration = 42
    journey1.sections.add()
    journey1.sections[-1].type = response_pb2.STREET_NETWORK
    journey1.sections[-1].street_network.mode = response_pb2.Walking

    responses.append(response_pb2.Response())
    journey2 = responses[-1].journeys.add()
    journey2.duration = 42
    journey2.sections.add()
    journey2.sections[-1].type = response_pb2.STREET_NETWORK
    journey2.sections[-1].street_network.mode = response_pb2.Bike

    jf.filter_similar_vj_journeys(list(journey_pairs_gen(responses)), {})

    assert 'to_delete' not in journey1.tags
    assert 'to_delete' not in journey2.tags


def test_similar_journeys_car_park():
    """
    We have to consider a journey with
    CAR / PARK / WALK to be equal to CAR / PARK
    """
    responses = [response_pb2.Response()]
    journey1 = response_pb2.Journey()
    journey1.sections.add()
    journey1.sections[-1].type = response_pb2.STREET_NETWORK
    journey1.sections[-1].street_network.mode = response_pb2.Car
    journey1.sections.add()
    journey1.sections[-1].type = response_pb2.PARK
    journey1.sections.add()
    journey1.sections[-1].type = response_pb2.STREET_NETWORK
    journey1.sections[-1].street_network.mode = response_pb2.Walking

    journey2 = response_pb2.Journey()
    journey2.sections.add()
    journey2.sections[-1].type = response_pb2.STREET_NETWORK
    journey2.sections[-1].street_network.mode = response_pb2.Car
    journey2.sections.add()
    journey2.sections[-1].type = response_pb2.PARK

    assert jf.compare(journey1, journey2, jf.similar_journeys_vj_generator)


def test_similar_journeys_bss_park():
    """
    We have to consider a journey with
    WALK / GET A BIKE / BSS to be equals to GET A BIKE / BSS
    """
    responses = [response_pb2.Response()]
    journey1 = response_pb2.Journey()
    journey1.sections.add()
    journey1.sections[-1].type = response_pb2.STREET_NETWORK
    journey1.sections[-1].street_network.mode = response_pb2.Walking
    journey1.sections.add()
    journey1.sections[-1].type = response_pb2.BSS_RENT
    journey1.sections.add()
    journey1.sections[-1].type = response_pb2.STREET_NETWORK
    journey1.sections[-1].street_network.mode = response_pb2.Bss

    journey2 = response_pb2.Journey()
    journey2.sections.add()
    journey2.sections[-1].type = response_pb2.BSS_RENT
    journey2.sections.add()
    journey2.sections[-1].type = response_pb2.STREET_NETWORK
    journey2.sections[-1].street_network.mode = response_pb2.Bss

    assert jf.compare(journey1, journey2, jf.similar_journeys_vj_generator)


def test_similar_journeys_crowfly_rs():
    """
    We have to consider a journey with
    CROWFLY WALK to be different than CROWFLY Ridesharing
    """
    journey1 = response_pb2.Journey()
    journey1.sections.add()
    journey1.sections[-1].type = response_pb2.CROW_FLY
    journey1.sections[-1].street_network.mode = response_pb2.Walking

    journey2 = response_pb2.Journey()
    journey2.sections.add()
    journey2.sections[-1].type = response_pb2.CROW_FLY
    journey2.sections[-1].street_network.mode = response_pb2.Ridesharing

    assert not jf.compare(journey1, journey2, jf.similar_journeys_vj_generator)


def test_departure_sort():
    """
    we want to sort by departure hour, then by duration
    """
    j1 = response_pb2.Journey()
    j1.departure_date_time = str_to_time_stamp('20151005T071000')
    j1.arrival_date_time = str_to_time_stamp('20151005T081900')
    j1.duration = j1.arrival_date_time - j1.departure_date_time
    j1.nb_transfers = 0

    j2 = response_pb2.Journey()
    j2.departure_date_time = str_to_time_stamp('20151005T072200')
    j2.arrival_date_time = str_to_time_stamp('20151005T083500')
    j2.duration = j2.arrival_date_time - j2.departure_date_time
    j2.nb_transfers = 0

    j3 = response_pb2.Journey()
    j3.departure_date_time = str_to_time_stamp('20151005T074500')
    j3.arrival_date_time = str_to_time_stamp('20151005T091200')
    j3.duration = j3.arrival_date_time - j3.departure_date_time
    j3.nb_transfers = 0

    j4 = response_pb2.Journey()
    j4.departure_date_time = str_to_time_stamp('20151005T074500')
    j4.arrival_date_time = str_to_time_stamp('20151005T091100')
    j4.duration = j4.arrival_date_time - j4.departure_date_time
    j4.nb_transfers = 0

    j5 = response_pb2.Journey()
    j5.departure_date_time = str_to_time_stamp('20151005T074500')
    j5.arrival_date_time = str_to_time_stamp('20151005T090800')
    j5.duration = j5.arrival_date_time - j5.departure_date_time
    j5.nb_transfers = 0

    result = [j1, j2, j3, j4, j5]
    random.shuffle(result)

    comparator = DepartureJourneySorter(True)
    result.sort(key=functools.cmp_to_key(comparator))
    assert result[0] == j1
    assert result[1] == j2
    assert result[2] == j5
    assert result[3] == j4
    assert result[4] == j3


def test_arrival_sort():
    """
    we want to sort by arrival hour, then by duration
    """
    j1 = response_pb2.Journey()
    j1.departure_date_time = str_to_time_stamp('20151005T071000')
    j1.arrival_date_time = str_to_time_stamp('20151005T081900')
    j1.duration = j1.arrival_date_time - j1.departure_date_time
    j1.nb_transfers = 0

    j2 = response_pb2.Journey()
    j2.departure_date_time = str_to_time_stamp('20151005T072200')
    j2.arrival_date_time = str_to_time_stamp('20151005T083500')
    j2.duration = j2.arrival_date_time - j2.departure_date_time
    j2.nb_transfers = 0

    j3 = response_pb2.Journey()
    j3.departure_date_time = str_to_time_stamp('20151005T074500')
    j3.arrival_date_time = str_to_time_stamp('20151005T091200')
    j3.duration = j3.arrival_date_time - j3.departure_date_time
    j3.nb_transfers = 0

    j4 = response_pb2.Journey()
    j4.departure_date_time = str_to_time_stamp('20151005T075000')
    j4.arrival_date_time = str_to_time_stamp('20151005T091200')
    j4.duration = j4.arrival_date_time - j4.departure_date_time
    j4.nb_transfers = 0

    j5 = response_pb2.Journey()
    j5.departure_date_time = str_to_time_stamp('20151005T075500')
    j5.arrival_date_time = str_to_time_stamp('20151005T091200')
    j5.duration = j5.arrival_date_time - j5.departure_date_time
    j5.nb_transfers = 0

    result = [j1, j2, j3, j4, j5]
    random.shuffle(result)

    comparator = ArrivalJourneySorter(True)
    result.sort(key=functools.cmp_to_key(comparator))
    assert result[0] == j1
    assert result[1] == j2
    assert result[2] == j5
    assert result[3] == j4
    assert result[4] == j3


def test_heavy_journey_walking():
    """
    we don't filter any journey with walking
    """
    journey = response_pb2.Journey()
    journey.sections.add()
    journey.sections[-1].type = response_pb2.STREET_NETWORK
    journey.sections[-1].street_network.mode = response_pb2.Walking
    journey.sections[-1].duration = 5

    f = jf.FilterTooShortHeavyJourneys(min_bike=10, min_car=20)
    assert f.filter_func(journey)


def test_heavy_journey_bike():
    """
    the first time the duration of the biking section is superior to the min value, so we keep the journey
    on the second test the duration is inferior to the min but we still keep this journey,
    as filter does not apply on direct_path bike.
    Finally we add a pt section and we keep duration_bike inferior to min_bike so the journey will be discard
    """
    journey = response_pb2.Journey()
    journey.sections.add()
    journey.sections[-1].type = response_pb2.STREET_NETWORK
    journey.sections[-1].street_network.mode = response_pb2.Bike
    journey.durations.bike = journey.sections[-1].duration = 15

    f = jf.FilterTooShortHeavyJourneys(min_bike=10, min_car=20)
    assert f.filter_func(journey)

    journey.durations.bike = journey.sections[-1].duration = 5

    f = jf.FilterTooShortHeavyJourneys(min_bike=10, min_car=20)
    assert f.filter_func(journey)

    journey.sections.add()
    journey.sections[-1].type = response_pb2.PUBLIC_TRANSPORT
    journey.sections[-1].street_network.mode = response_pb2.PUBLIC_TRANSPORT

    f = jf.FilterTooShortHeavyJourneys(min_bike=10, min_car=20)
    assert not f.filter_func(journey)


def test_filter_wrapper():
    """
    Testing that filter_wrapper is fine (see filter_wrapper doc)
    """

    class LoveHateFilter(jf.SingleJourneyFilter):
        message = 'i_dont_like_you'

        def __init__(self, love=True):
            self.love = love

        def filter_func(self, journey):
            return self.love

    ref_journey = response_pb2.Journey()

    # first we test when debug-mode deactivated (each time both OK-filter and KO-filter)
    j = deepcopy(ref_journey)
    wrapped_f = jf.filter_wrapper(is_debug=False, filter_obj=LoveHateFilter(love=True))
    assert wrapped_f(j)
    assert 'to_delete' not in j.tags
    assert 'deleted_because_i_dont_like_you' not in j.tags

    j = deepcopy(ref_journey)
    wrapped_f = jf.filter_wrapper(is_debug=False, filter_obj=LoveHateFilter(love=False))
    assert not wrapped_f(j)
    assert 'to_delete' in j.tags
    assert 'deleted_because_i_dont_like_you' not in j.tags

    # test using without debug mode (should be deactivated)
    j = deepcopy(ref_journey)
    wrapped_f = jf.filter_wrapper(filter_obj=LoveHateFilter(love=True))
    assert wrapped_f(j)
    assert 'to_delete' not in j.tags
    assert 'deleted_because_i_dont_like_you' not in j.tags

    j = deepcopy(ref_journey)
    wrapped_f = jf.filter_wrapper(filter_obj=LoveHateFilter(love=False))
    assert not wrapped_f(j)
    assert 'to_delete' in j.tags
    assert 'deleted_because_i_dont_like_you' not in j.tags

    # test when debug-mode is activated
    j = deepcopy(ref_journey)
    wrapped_f = jf.filter_wrapper(is_debug=True, filter_obj=LoveHateFilter(love=True))
    assert wrapped_f(j)
    assert 'to_delete' not in j.tags
    assert 'deleted_because_i_dont_like_you' not in j.tags

    j = deepcopy(ref_journey)
    wrapped_f = jf.filter_wrapper(is_debug=True, filter_obj=LoveHateFilter(love=False))
    assert wrapped_f(j)
    assert 'to_delete' in j.tags
    assert 'deleted_because_i_dont_like_you' in j.tags


def test_heavy_journey_car():
    """
    the first time the duration of the car section is superior to the min value, so we keep the journey
    on the second test the duration is inferior to the min, so we delete the journey
    """
    journey = response_pb2.Journey()
    journey.sections.add()
    journey.sections[-1].type = response_pb2.STREET_NETWORK
    journey.sections[-1].street_network.mode = response_pb2.Car
    journey.durations.car = journey.sections[-1].duration = 25

    f = jf.FilterTooShortHeavyJourneys(min_bike=10, min_car=20)
    assert f.filter_func(journey)

    journey.durations.car = journey.sections[-1].duration = 15

    f = jf.FilterTooShortHeavyJourneys(min_bike=10, min_car=20)
    assert not f.filter_func(journey)


def test_heavy_journey_taxi():
    """
    the first time the duration of the taxi section is superior to the min value, so we keep the journey
    on the second test the duration is inferior to the min, so we delete the journey
    """
    journey = response_pb2.Journey()
    journey.sections.add()
    journey.sections[-1].type = response_pb2.STREET_NETWORK
    journey.sections[-1].street_network.mode = response_pb2.Taxi
    journey.durations.taxi = journey.sections[-1].duration = 25

    f = jf.FilterTooShortHeavyJourneys(min_bike=10, min_taxi=20)
    assert f.filter_func(journey)

    journey.durations.taxi = journey.sections[-1].duration = 15

    f = jf.FilterTooShortHeavyJourneys(min_bike=10, min_taxi=20)
    assert not f.filter_func(journey)


def test_heavy_journey_bss():
    """
    we should not remove any bss journey since it is already in concurrence with the walking
    """
    journey = response_pb2.Journey()
    journey.sections.add()
    journey.sections[-1].type = response_pb2.STREET_NETWORK
    journey.sections[-1].street_network.mode = response_pb2.Walking
    journey.sections[-1].duration = 5

    journey.sections.add()
    journey.sections[-1].type = response_pb2.BSS_RENT
    journey.sections[-1].duration = 5

    journey.sections.add()
    journey.sections[-1].type = response_pb2.STREET_NETWORK
    journey.sections[-1].street_network.mode = response_pb2.Bike
    journey.sections[-1].duration = 5

    journey.sections.add()
    journey.sections[-1].type = response_pb2.BSS_PUT_BACK
    journey.sections[-1].duration = 5

    journey.sections.add()
    journey.sections[-1].type = response_pb2.STREET_NETWORK
    journey.sections[-1].street_network.mode = response_pb2.Walking
    journey.sections[-1].duration = 5

    journey.durations.bike = 5
    journey.durations.walking = 10

    f = jf.FilterTooShortHeavyJourneys(min_bike=10, min_car=20)
    assert f.filter_func(journey)


def test_activate_deactivate_min_bike():
    """

      A                 B                           C            D
      *................*============================*.............*
      A: origin
      D: Destination
      A->B : Bike
      B->C : public transport
      C->D : Bike

    """
    # case 1: request with duration_bike greater than min_bike

    journey = response_pb2.Journey()
    journey.sections.add()
    journey.sections[-1].type = response_pb2.STREET_NETWORK
    journey.sections[-1].street_network.mode = response_pb2.Bike
    journey.sections[-1].duration = 5

    journey.sections.add()
    journey.sections[-1].type = response_pb2.PUBLIC_TRANSPORT
    journey.sections[-1].street_network.mode = response_pb2.PUBLIC_TRANSPORT
    journey.sections[-1].duration = 35

    journey.sections.add()
    journey.sections[-1].type = response_pb2.STREET_NETWORK
    journey.sections[-1].street_network.mode = response_pb2.Bike
    journey.sections[-1].duration = 7

    journey.durations.bike = 12

    f = jf.FilterTooShortHeavyJourneys(min_bike=10)
    assert f.filter_func(journey)

    # case 2: request with duration_bike less than min_bike
    journey.sections[-1].duration = 2
    journey.durations.bike = 7

    f = jf.FilterTooShortHeavyJourneys(min_bike=10)
    assert not f.filter_func(journey)


def test_activate_deactivate_min_car():
    """

      A                 B                           C            D
      *................*============================*.............*
      A: origin
      D: Destination
      A->B : car
      B->C : public transport
      C->D : car

    """
    # case 1: request with duration_car greater than min_car

    journey = response_pb2.Journey()
    journey.sections.add()
    journey.sections[-1].type = response_pb2.STREET_NETWORK
    journey.sections[-1].street_network.mode = response_pb2.Car
    journey.sections[-1].duration = 5

    journey.sections.add()
    journey.sections[-1].type = response_pb2.PUBLIC_TRANSPORT
    journey.sections[-1].street_network.mode = response_pb2.PUBLIC_TRANSPORT
    journey.sections[-1].duration = 35

    journey.sections.add()
    journey.sections[-1].type = response_pb2.STREET_NETWORK
    journey.sections[-1].street_network.mode = response_pb2.Car
    journey.sections[-1].duration = 7

    journey.durations.car = 12

    f = jf.FilterTooShortHeavyJourneys(min_car=10)
    assert f.filter_func(journey)

    # case 2: request with duration_car less than min_car
    journey.sections[-1].duration = 2
    journey.durations.car = 7

    f = jf.FilterTooShortHeavyJourneys(min_car=10)
    assert not f.filter_func(journey)


def test_activate_deactivate_min_taxi():
    """

      A                 B                           C            D
      *................*============================*.............*
      A: origin
      D: Destination
      A->B : taxi
      B->C : public transport
      C->D : taxi

    """
    # case 1: request with duration_taxi greater than min_taxi

    journey = response_pb2.Journey()
    journey.sections.add()
    journey.sections[-1].type = response_pb2.STREET_NETWORK
    journey.sections[-1].street_network.mode = response_pb2.Taxi
    journey.sections[-1].duration = 5

    journey.sections.add()
    journey.sections[-1].type = response_pb2.PUBLIC_TRANSPORT
    journey.sections[-1].street_network.mode = response_pb2.PUBLIC_TRANSPORT
    journey.sections[-1].duration = 35

    journey.sections.add()
    journey.sections[-1].type = response_pb2.STREET_NETWORK
    journey.sections[-1].street_network.mode = response_pb2.Taxi
    journey.sections[-1].duration = 7

    journey.durations.taxi = 12

    f = jf.FilterTooShortHeavyJourneys(min_taxi=10)
    assert f.filter_func(journey)

    # case 2: request with duration_taxi less than min_taxi
    journey.sections[-1].duration = 2
    journey.durations.taxi = 7

    f = jf.FilterTooShortHeavyJourneys(min_taxi=10)
    assert not f.filter_func(journey)


def test_activate_deactivate_mixed_min_taxi_bike_car():
    """

      A          B               C      D           E
      *..........*===============*......*###########*
      A: origin
      E: Destination
      A->B : taxi
      B->C : public transport
      C->D : bike
      D->E : car

    """
    # case 1: request with all duration_{mode} greater than min_{mode}

    journey = response_pb2.Journey()
    journey.sections.add()
    journey.sections[-1].type = response_pb2.STREET_NETWORK
    journey.sections[-1].street_network.mode = response_pb2.Taxi
    journey.sections[-1].duration = 5

    journey.sections.add()
    journey.sections[-1].type = response_pb2.PUBLIC_TRANSPORT
    journey.sections[-1].street_network.mode = response_pb2.PUBLIC_TRANSPORT
    journey.sections[-1].duration = 10

    journey.sections.add()
    journey.sections[-1].type = response_pb2.STREET_NETWORK
    journey.sections[-1].street_network.mode = response_pb2.Bike
    journey.sections[-1].duration = 15

    journey.sections.add()
    journey.sections[-1].type = response_pb2.STREET_NETWORK
    journey.sections[-1].street_network.mode = response_pb2.Car
    journey.sections[-1].duration = 20

    journey.durations.taxi = 5
    journey.durations.car = 20
    journey.durations.bike = 15

    f = jf.FilterTooShortHeavyJourneys(min_taxi=2, min_bike=10, min_car=15)
    assert f.filter_func(journey)

    # case 2: request with only duration_taxi less than min_taxi
    f = jf.FilterTooShortHeavyJourneys(min_taxi=10, min_bike=10, min_car=15)
    assert not f.filter_func(journey)

    # case 2: request with only duration_bike less than min_bike
    f = jf.FilterTooShortHeavyJourneys(min_taxi=0, min_bike=20, min_car=0)
    assert not f.filter_func(journey)

    # case 2: request with all duration_{mode} less than min_{mode}
    f = jf.FilterTooShortHeavyJourneys(min_taxi=10, min_bike=20, min_car=25)
    assert not f.filter_func(journey)


def test_filter_direct_path_mode_car():
    # is_dp and not is_in_direct_path_mode_list
    journey = response_pb2.Journey()
    journey.tags.append("car")
    journey.tags.append("non_pt")
    f = jf.FilterDirectPathMode(["bike"])
    assert not f.filter_func(journey)

    # is_dp and is_in_direct_path_mode_list
    journey = response_pb2.Journey()
    journey.tags.append("car")
    journey.tags.append("non_pt")
    f = jf.FilterDirectPathMode(["car"])
    assert f.filter_func(journey)

    # is_dp and is_in_direct_path_mode_list
    journey = response_pb2.Journey()
    journey.tags.append("car")
    journey.tags.append("non_pt")
    f = jf.FilterDirectPathMode(["taxi", "surf", "car", "bike"])
    assert f.filter_func(journey)

    # not is_dp and not is_in_direct_path_mode_list
    journey = response_pb2.Journey()
    journey.tags.append("car")
    f = jf.FilterDirectPathMode(["bike"])
    assert f.filter_func(journey)

    # not is_dp and not is_in_direct_path_mode_list
    journey = response_pb2.Journey()
    journey.tags.append("car")
    f = jf.FilterDirectPathMode(["car"])
    assert f.filter_func(journey)


def test_heavy_journey_ridesharing():
    """
    the first time the duration of the ridesharing section is superior to the min value, so we keep the journey
    on the second test the duration is inferior to the min, so we delete the journey
    """
    journey = response_pb2.Journey()
    journey.sections.add()
    journey.sections[-1].type = response_pb2.STREET_NETWORK
    journey.sections[-1].street_network.mode = response_pb2.Ridesharing
    journey.durations.ridesharing = journey.sections[-1].duration = 25

    # Ridesharing duration is superior to min_ridesharing value so we have ridesharing section
    f = jf.FilterTooShortHeavyJourneys(min_ridesharing=20)
    assert f.filter_func(journey)

    # Ridesharing duration is inferior to min_ridesharing value
    # In this case we have reject ridesharing section
    journey.durations.ridesharing = journey.sections[-1].duration = 15
    f = jf.FilterTooShortHeavyJourneys(min_ridesharing=20)
    assert not f.filter_func(journey)
