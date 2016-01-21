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
from jormungandr.scenarios import journey_filter
from jormungandr.scenarios.utils import DepartureJourneySorter, ArrivalJourneySorter
from jormungandr.scenarios.journey_filter import _to_be_deleted
import navitiacommon.response_pb2 as response_pb2
from navitiacommon import default_values
from jormungandr.scenarios.default import Scenario, are_equals
from jormungandr.utils import str_to_time_stamp
from nose.tools import eq_
import random


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
    eq_(response.journeys[0].arrival_date_time, str_to_time_stamp("20140422T0758"))
    eq_(response.journeys[1].arrival_date_time, str_to_time_stamp("20140422T0800"))


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
    eq_(response.journeys[0].departure_date_time, str_to_time_stamp("20140422T0758"))
    eq_(response.journeys[1].departure_date_time, str_to_time_stamp("20140422T0800"))


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


def journeys_gen(list_responses):
    for r in list_responses:
        for j in r.journeys:
            if not _to_be_deleted(j):
                yield j


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

    journey_filter._filter_similar_journeys(list(journeys_gen(responses)), {})

    assert len(list(journeys_gen(responses))) == 1


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

    journey_filter._filter_similar_journeys(list(journeys_gen(responses)), {})

    assert len(list(journeys_gen(responses))) == 1


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

    journey_filter._filter_similar_journeys(list(journeys_gen(responses)), {})

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

    journey_filter._filter_similar_journeys(list(journeys_gen(responses)), {})

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

    journey_filter._filter_similar_journeys(list(journeys_gen(responses)), {})

    assert 'to_delete' not in journey1.tags
    assert 'to_delete' not in journey2.tags


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

    journey_filter._filter_not_coherent_journeys(list(journeys_gen(responses)),
                                                 MockInstance(), request, request)

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

    journey_filter._filter_not_coherent_journeys(list(journeys_gen(responses)),
                                                 MockInstance(), request, request)

    assert 'to_delete' not in journey1.tags
    assert 'to_delete' not in journey2.tags


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

    journey_filter._filter_not_coherent_journeys(list(journeys_gen(responses)),
                                                 MockInstance(), request, request)

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
    eq_(result[0], j1)
    eq_(result[1], j2)
    eq_(result[2], j5)
    eq_(result[3], j4)
    eq_(result[4], j3)

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
    eq_(result[0], j1)
    eq_(result[1], j2)
    eq_(result[2], j5)
    eq_(result[3], j4)
    eq_(result[4], j3)

def test_heavy_journey_walking():
    """
    we don't filter any journey with walking
    """
    request = {'_min_bike': 10, '_min_car': 20}
    journey = response_pb2.Journey()
    journey.sections.add()
    journey.sections[-1].type = response_pb2.STREET_NETWORK
    journey.sections[-1].street_network.mode = response_pb2.Walking
    journey.sections[-1].duration = 5


    journey_filter._filter_too_short_heavy_journeys([journey], request)

    assert 'to_delete' not in journey.tags

def test_heavy_journey_bike():
    """
    the first time the duration of the biking section is superior to the min value, so we keep the journey
    on the second test the duration is inferior to the min, so we delete the journey
    """
    request = {'_min_bike': 10, '_min_car': 20}
    journey = response_pb2.Journey()
    journey.sections.add()
    journey.sections[-1].type = response_pb2.STREET_NETWORK
    journey.sections[-1].street_network.mode = response_pb2.Bike
    journey.sections[-1].duration = 15

    journey_filter._filter_too_short_heavy_journeys([journey], request)

    assert 'to_delete' not in journey.tags

    journey.sections[-1].duration = 5

    journey_filter._filter_too_short_heavy_journeys([journey], request)

    assert 'to_delete' in journey.tags

def test_heavy_journey_car():
    """
    the first time the duration of the car section is superior to the min value, so we keep the journey
    on the second test the duration is inferior to the min, so we delete the journey
    """
    request = {'_min_bike': 10, '_min_car': 20}
    journey = response_pb2.Journey()
    journey.sections.add()
    journey.sections[-1].type = response_pb2.STREET_NETWORK
    journey.sections[-1].street_network.mode = response_pb2.Car
    journey.sections[-1].duration = 25

    journey_filter._filter_too_short_heavy_journeys([journey], request)

    assert 'to_delete' not in journey.tags

    journey.sections[-1].duration = 15

    journey_filter._filter_too_short_heavy_journeys([journey], request)

    assert 'to_delete' in journey.tags

def test_heavy_journey_bss():
    """
    we should not remove any bss journey since it is already in concurrence with the walking
    """
    request = {'_min_bike': 10, '_min_car': 20}
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

    journey_filter._filter_too_short_heavy_journeys([journey], request)

    assert 'to_delete' not in journey.tags

