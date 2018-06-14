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
from copy import deepcopy
from jormungandr.scenarios import journey_filter as jf
from jormungandr.scenarios.utils import DepartureJourneySorter, ArrivalJourneySorter
import navitiacommon.response_pb2 as response_pb2
from navitiacommon import default_values
from jormungandr.scenarios.default import Scenario, are_equals
from jormungandr.utils import str_to_time_stamp
import random
import itertools


def empty_journeys_test():
    scenario = Scenario()
    response = response_pb2.Response()
    scenario.sort_journeys(response, 'arrival_time')
    assert not response.journeys


def different_arrival_times_test():
    scenario = Scenario()
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

    scenario.sort_journeys(response, 'arrival_time')
    assert response.journeys[0].arrival_date_time ==  str_to_time_stamp("20140422T0758")
    assert response.journeys[1].arrival_date_time ==  str_to_time_stamp("20140422T0800")


def different_departure_times_test():
    scenario = Scenario()
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

    scenario.sort_journeys(response, 'departure_time')
    assert response.journeys[0].departure_date_time ==  str_to_time_stamp("20140422T0758")
    assert response.journeys[1].departure_date_time ==  str_to_time_stamp("20140422T0800")


def different_duration_test():
    scenario = Scenario()
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

    scenario.sort_journeys(response, 'arrival_time')
    assert response.journeys[0].arrival_date_time == str_to_time_stamp("20140422T0800")
    assert response.journeys[1].arrival_date_time == str_to_time_stamp("20140422T0800")
    assert response.journeys[0].duration == 3*60
    assert response.journeys[1].duration == 5*60


def different_nb_transfers_test():
    scenario = Scenario()
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

    scenario.sort_journeys(response, 'arrival_time')
    assert response.journeys[0].arrival_date_time == str_to_time_stamp("20140422T0800")
    assert response.journeys[1].arrival_date_time == str_to_time_stamp("20140422T0800")
    assert response.journeys[0].duration == 25*60
    assert response.journeys[1].duration == 25*60
    assert response.journeys[0].nb_transfers == 0
    assert response.journeys[1].nb_transfers == 1


def different_duration_non_pt_test():
    scenario = Scenario()
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

    scenario.sort_journeys(response, 'arrival_time')
    assert response.journeys[0].arrival_date_time == str_to_time_stamp("20140422T0800")
    assert response.journeys[1].arrival_date_time == str_to_time_stamp("20140422T0800")
    assert response.journeys[0].duration == 25 * 60
    assert response.journeys[1].duration == 25 * 60
    assert response.journeys[0].nb_transfers == 1
    assert response.journeys[1].nb_transfers == 1

    #We want to have journey2 in first, this is the one with 4 sections
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


def test_journeys_equality_test_different_journeys():
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


def test_journeys_equality_test_different_nb_sections():
    """
    test the are_equals method, applied to journeys with different number of sections
    """
    journey1 = create_dummy_journey()
    modified_section = journey1.sections.add()
    modified_section.origin.uri = "stop_point_10"
    modified_section.destination.uri = "stop_point_22"
    modified_section.vehicle_journey.uri = "vj_tata"

    journey2 = create_dummy_journey()

    assert not are_equals(journey1, journey2)


def test_journeys_equality_test_same_journeys():
    """No question, a  journey must be equal to self"""
    journey1 = create_dummy_journey()

    assert are_equals(journey1, journey1)

    #and likewise if not the same memory address
    journey2 = create_dummy_journey()
    assert are_equals(journey1, journey2)


def journey_pairs_gen(list_responses):
    return itertools.combinations(jf.get_qualified_journeys(list_responses), 2)

def test_journeys_equality_test_almost_same_journeys():
    """
    test the are_equals method, applied to different journeys, but with meaningless differences
    """
    journey1 = create_dummy_journey()
    modified_section = journey1.sections[4]
    modified_section.duration = 1337

    journey2 = create_dummy_journey()
    modified_section = journey2.sections[0]
    modified_section.length = 42
    modified_section = journey2.sections[1]
    modified_section.transfer_type = response_pb2.stay_in

    assert are_equals(journey1, journey2)


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

class MockInstance(object):
    def __init__(self):
        pass  #TODO when we'll got instances's param


def test_too_late_journeys():
    request = {'datetime': 1000,
               '_night_bus_filter_max_factor': default_values.night_bus_filter_max_factor,
               '_night_bus_filter_base_factor': default_values.night_bus_filter_base_factor,
               }
    responses = [response_pb2.Response()]
    journey1 = responses[0].journeys.add()
    journey1.departure_date_time = 2000
    journey1.arrival_date_time = 3000

    journey2 = responses[0].journeys.add()  # later than journey1, but acceptable
    journey2.departure_date_time = 2700
    journey2.arrival_date_time = 5000

    responses.append(response_pb2.Response())  # too late compared to journey1
    journey3 = responses[-1].journeys.add()
    journey3.departure_date_time = 10000
    journey3.arrival_date_time = 13000

    jf._filter_too_late_journeys(responses, request)

    assert 'to_delete' not in journey1.tags
    assert 'to_delete' not in journey2.tags
    assert 'to_delete' in journey3.tags


def test_not_too_late_journeys():
    request = {'datetime': 1000,
               '_night_bus_filter_max_factor': default_values.night_bus_filter_max_factor,
               '_night_bus_filter_base_factor': default_values.night_bus_filter_base_factor,
               }
    responses = [response_pb2.Response()]
    journey1 = responses[0].journeys.add()
    journey1.departure_date_time = 2000
    journey1.arrival_date_time = 3000

    responses.append(response_pb2.Response())
    journey2 = responses[-1].journeys.add()
    journey2.departure_date_time = 3100
    journey2.arrival_date_time = 3200

    jf._filter_too_late_journeys(responses, request)

    assert 'to_delete' not in journey1.tags
    assert 'to_delete' not in journey2.tags


def test_too_late_journeys_but_better_mode():
    request = {'datetime': 1000000,
               'clockwise': True,
               '_night_bus_filter_max_factor': 2,
               '_night_bus_filter_base_factor': 9,
               }
    responses = [response_pb2.Response()]
    car = responses[0].journeys.add()
    car.departure_date_time = 1001042
    car.arrival_date_time = 1002000
    car.tags.append('car')

    responses.append(response_pb2.Response())
    bike = responses[-1].journeys.add()
    bike.departure_date_time = 1001042
    bike.arrival_date_time = 1004010 # car * 2 + 10
    bike.tags.append('bike')

    responses.append(response_pb2.Response())
    bss = responses[-1].journeys.add()
    bss.departure_date_time = 1001042
    bss.arrival_date_time = 1008030 # bike * 2 + 10
    bss.tags.append('bss')

    responses.append(response_pb2.Response())
    walking = responses[-1].journeys.add()
    walking.departure_date_time = 1001042
    walking.arrival_date_time = 1016070 # bss * 2 + 10
    walking.tags.append('walking')

    jf._filter_too_late_journeys(responses, request)

    assert all('to_delete' not in j.tags for r in responses for j in r.journeys)


def test_too_late_journeys_and_worst_mode():
    request = {'datetime': 1000000,
               'clockwise': True,
               '_night_bus_filter_max_factor': 2,
               '_night_bus_filter_base_factor': 9,
               }
    responses = [response_pb2.Response()]
    walking = responses[0].journeys.add()
    walking.departure_date_time = 1001042
    walking.arrival_date_time = 1002000
    walking.tags.append('walking')

    responses.append(response_pb2.Response())
    bss = responses[-1].journeys.add()
    bss.departure_date_time = 1001042
    bss.arrival_date_time = 1004010 # walking * 2 + 10
    bss.tags.append('bss')

    responses.append(response_pb2.Response())
    bike = responses[-1].journeys.add()
    bike.departure_date_time = 1001042
    bike.arrival_date_time = 1008030 # bss * 2 + 10
    bike.tags.append('bike')

    responses.append(response_pb2.Response())
    car = responses[-1].journeys.add()
    car.departure_date_time = 1001042
    car.arrival_date_time = 1016070 # bike * 2 + 10
    car.tags.append('car')

    jf._filter_too_late_journeys(responses, request)

    assert ['to_delete' not in j.tags for r in responses for j in r.journeys].count(True) == 1


def test_not_too_late_journeys_non_clockwise():
    request = {'datetime': 12000,
               'clockwise': False,
               '_night_bus_filter_max_factor': default_values.night_bus_filter_max_factor,
               '_night_bus_filter_base_factor': default_values.night_bus_filter_base_factor,
               }

    responses = [response_pb2.Response()]
    journey1 = responses[0].journeys.add()
    journey1.departure_date_time = 2000  # way too soon compared to the second one
    journey1.arrival_date_time = 3000

    responses.append(response_pb2.Response())
    journey2 = responses[-1].journeys.add()
    journey2.departure_date_time = 10000
    journey2.arrival_date_time = 11000

    journey3 = responses[-1].journeys.add() # before journey2, but acceptable
    journey3.departure_date_time = 8000
    journey3.arrival_date_time = 9000

    jf._filter_too_late_journeys(responses, request)

    assert 'to_delete' in journey1.tags
    assert 'to_delete' not in journey2.tags
    assert 'to_delete' not in journey3.tags

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

    compartor = DepartureJourneySorter(True)
    result.sort(compartor)
    assert result[0] ==  j1
    assert result[1] ==  j2
    assert result[2] ==  j5
    assert result[3] ==  j4
    assert result[4] ==  j3

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

    compartor = ArrivalJourneySorter(True)
    result.sort(compartor)
    assert result[0] ==  j1
    assert result[1] ==  j2
    assert result[2] ==  j5
    assert result[3] ==  j4
    assert result[4] ==  j3

def test_heavy_journey_walking():
    """
    we don't filter any journey with walking
    """
    journey = response_pb2.Journey()
    journey.sections.add()
    journey.sections[-1].type = response_pb2.STREET_NETWORK
    journey.sections[-1].street_network.mode = response_pb2.Walking
    journey.sections[-1].duration = 5

    f, _ = jf.make_filter_too_short_heavy_journeys(min_bike=10, min_car=20)
    assert f(journey)

def test_heavy_journey_bike():
    """
    the first time the duration of the biking section is superior to the min value, so we keep the journey
    on the second test the duration is inferior to the min, so we delete the journey
    """
    journey = response_pb2.Journey()
    journey.sections.add()
    journey.sections[-1].type = response_pb2.STREET_NETWORK
    journey.sections[-1].street_network.mode = response_pb2.Bike
    journey.durations.bike = journey.sections[-1].duration = 15

    f, _ = jf.make_filter_too_short_heavy_journeys(min_bike=10, min_car=20)
    assert f(journey)

    journey.durations.bike = journey.sections[-1].duration = 5

    f, _ = jf.make_filter_too_short_heavy_journeys(min_bike=10, min_car=20, orig_modes=['bike', 'walking'])
    assert not f(journey)

def test_filter_wrapper_on_heavy_journey_bike():
    """
    same test as above, but this time we wrap the filter to test wrapping is ok
    """
    ref_journey = response_pb2.Journey()
    ref_journey.sections.add()
    ref_journey.sections[-1].type = response_pb2.STREET_NETWORK
    ref_journey.sections[-1].street_network.mode = response_pb2.Bike
    ref_journey.durations.bike = ref_journey.sections[-1].duration = 15

    # first we test when debug-mode deactivated
    j = deepcopy(ref_journey)

    wrapped_f = jf.filter_wrapper(is_debug=False,
            f_msg_pair=jf.make_filter_too_short_heavy_journeys(min_bike=10, min_car=20))
    assert wrapped_f(j)
    assert 'to_delete' not in j.tags
    assert 'deleted_because_too_short_heavy_mode_fallback' not in j.tags

    j.durations.bike = j.sections[-1].duration = 5

    wrapped_f = jf.filter_wrapper(is_debug=False,
            f_msg_pair=jf.make_filter_too_short_heavy_journeys(min_bike=10, min_car=20,
                                                               orig_modes=['bike', 'walking']))
    assert not wrapped_f(j)
    assert 'to_delete' in j.tags
    assert 'deleted_because_too_short_heavy_mode_fallback' not in j.tags

    # test using without debug mode (should be deactivated)
    j = deepcopy(ref_journey)

    wrapped_f = jf.filter_wrapper(jf.make_filter_too_short_heavy_journeys(min_bike=10, min_car=20))
    assert wrapped_f(j)
    assert 'to_delete' not in j.tags
    assert 'deleted_because_too_short_heavy_mode_fallback' not in j.tags

    j.durations.bike = j.sections[-1].duration = 5

    wrapped_f = jf.filter_wrapper(jf.make_filter_too_short_heavy_journeys(min_bike=10, min_car=20,
                                                                          orig_modes=['bike', 'walking']))
    assert not wrapped_f(j)
    assert 'to_delete' in j.tags
    assert 'deleted_because_too_short_heavy_mode_fallback' not in j.tags

    # test when debug-mode is activated
    j = deepcopy(ref_journey)

    wrapped_f = jf.filter_wrapper(is_debug=True,
            f_msg_pair=jf.make_filter_too_short_heavy_journeys(min_bike=10, min_car=20))
    assert wrapped_f(j)
    assert 'to_delete' not in j.tags
    assert 'deleted_because_too_short_heavy_mode_fallback' not in j.tags

    j.durations.bike = j.sections[-1].duration = 5

    wrapped_f = jf.filter_wrapper(is_debug=True,
            f_msg_pair=jf.make_filter_too_short_heavy_journeys(min_bike=10, min_car=20,
                                                               orig_modes=['bike', 'walking']))
    assert wrapped_f(j)
    assert 'to_delete' in j.tags
    assert 'deleted_because_too_short_heavy_mode_fallback' in j.tags


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

    f, _ = jf.make_filter_too_short_heavy_journeys(min_bike=10, min_car=20)
    assert f(journey)

    journey.durations.car = journey.sections[-1].duration = 15

    f, _ = jf.make_filter_too_short_heavy_journeys(min_bike=10, min_car=20,
                                                   orig_modes=['bike', 'walking'])
    assert not f(journey)

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

    f, _ = jf.make_filter_too_short_heavy_journeys(min_bike=10, min_car=20)
    assert f(journey)


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
    # case 1: request without origin_mode and destination_mode

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

    f, _ = jf.make_filter_too_short_heavy_journeys(min_bike=10)
    assert f(journey)

    # case 2: request without origin_mode
    journey.sections[-1].duration = 15
    journey.durations.bike = 20

    f, _ = jf.make_filter_too_short_heavy_journeys(min_bike=8, dest_modes=['bike', 'walking'])
    assert f(journey)

    # case 3: request without destination_mode
    journey.sections[0].duration = 15
    journey.sections[-1].duration = 5
    journey.durations.bike = 20

    f, _ = jf.make_filter_too_short_heavy_journeys(min_bike=8, orig_modes=['bike', 'walking'])
    assert f(journey)

    # case 4: request without walking in origin_mode
    journey.sections[0].duration = 5
    journey.sections[-1].duration = 15
    journey.durations.bike = 20

    f, _ = jf.make_filter_too_short_heavy_journeys(min_bike=8, orig_modes=['bike'])
    assert f(journey)

    # case 5: request without walking in destination_mode
    journey.sections[0].duration = 15
    journey.sections[-1].duration = 5
    journey.durations.bike = 20

    f, _ = jf.make_filter_too_short_heavy_journeys(min_bike=8, dest_modes=['bike'])
    assert f(journey)

    # case 6: request with bike only in origin_mode destination_mode
    journey.sections[0].duration = 15
    journey.sections[-1].duration = 14
    journey.durations.bike = 29

    f, _ = jf.make_filter_too_short_heavy_journeys(min_bike=17, orig_modes=['bike'], dest_modes=['bike'])
    assert f(journey)

    # case 7: request with walking in destination_mode
    journey.sections[0].duration = 15
    journey.sections[-1].duration = 5
    journey.durations.bike = 20

    f, _ = jf.make_filter_too_short_heavy_journeys(min_bike=8, dest_modes=['bike', 'walking'])
    assert not f(journey)

    # case 8: request with walking in origin_mode
    journey.sections[0].duration = 5
    journey.sections[-1].duration = 15
    journey.durations.bike = 20

    f, _ = jf.make_filter_too_short_heavy_journeys(min_bike=8, orig_modes=['bike', 'walking'])
    assert not f(journey)

    # case 9: request with bike in origin_mode and bike, walking in destination_mode
    journey.sections[0].duration = 5
    journey.sections[-1].duration = 7
    journey.durations.bike = 12

    f, _ = jf.make_filter_too_short_heavy_journeys(min_bike=8, orig_modes=['bike'],
                                                   dest_modes=['bike', 'walking'])
    assert not f(journey)

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
    # case 1: request without origin_mode and destination_mode

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

    f, _ = jf.make_filter_too_short_heavy_journeys(min_car=10)
    assert f(journey)

    # case 2: request without origin_mode
    journey.sections[-1].duration = 15
    journey.durations.car = 20

    f, _ = jf.make_filter_too_short_heavy_journeys(min_car=8, dest_modes=['car', 'walking'])
    assert f(journey)

    # case 3: request without destination_mode
    journey.sections[0].duration = 15
    journey.sections[-1].duration = 5
    journey.durations.car = 20

    f, _ = jf.make_filter_too_short_heavy_journeys(min_car=8, orig_modes=['car', 'walking'])
    assert f(journey)

    # case 4: request without walking in origin_mode
    journey.sections[0].duration = 5
    journey.sections[-1].duration = 15
    journey.durations.car = 20

    f, _ = jf.make_filter_too_short_heavy_journeys(min_car=8, orig_modes=['car'])
    assert f(journey)

    # case 5: request without walking in destination_mode
    journey.sections[0].duration = 15
    journey.sections[-1].duration = 5
    journey.durations.car = 20

    f, _ = jf.make_filter_too_short_heavy_journeys(min_car=8, dest_modes=['car'])
    assert f(journey)

    # case 6: request with car only in origin_mode destination_mode
    journey.sections[0].duration = 15
    journey.sections[-1].duration = 14
    journey.durations.car = 29

    f, _ = jf.make_filter_too_short_heavy_journeys(min_car=17, orig_modes=['car'], dest_modes=['car'])
    assert f(journey)

    # case 7: request with walking in destination_mode
    journey.sections[0].duration = 15
    journey.sections[-1].duration = 5
    journey.durations.car = 20

    f, _ = jf.make_filter_too_short_heavy_journeys(min_car=8, dest_modes=['car', 'walking'])
    assert not f(journey)

    # case 8: request with walking in origin_mode
    journey.sections[0].duration = 5
    journey.sections[-1].duration = 15
    journey.durations.car = 20

    f, _ = jf.make_filter_too_short_heavy_journeys(min_car=8, orig_modes=['car', 'walking'])
    assert not f(journey)

    # case 9: request with bike in origin_mode and bike, walking in destination_mode
    journey.sections[0].duration = 5
    journey.sections[-1].duration = 7
    journey.durations.car = 12

    f, _ = jf.make_filter_too_short_heavy_journeys(min_car=8, orig_modes=['car'],
                                                   dest_modes=['car', 'walking'])
    assert not f(journey)
