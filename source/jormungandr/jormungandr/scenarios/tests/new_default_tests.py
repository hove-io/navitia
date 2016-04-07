# Copyright (c) 2001-2015, Canal TP and/or its affiliates. All rights reserved.
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
import navitiacommon.response_pb2 as response_pb2
from jormungandr.scenarios import new_default
"""
 sections       0   1   2   3   4   5   6   7   8   9   10
 -------------------------------------------------------------
JOURNEYS 1          1   1   1   1                       1    |
JOURNEYS 2          1   1           1   1               1    |
JOURNEYS 3              1   1   1                       1    |
JOURNEYS 4              1           1   1               1    |
JOURNEYS 5      1       1   1           1               1    |
JOURNEYS 6          1   1                    1          1    |
JOURNEYS 7          1   1                        1      1    |
JOURNEYS 8              1                    1          1    |
JOURNEYS 9              1                        1      1    |
JOURNEYS 10     1       1                    1          1    |
JOURNEYS 11         1   1                        1  1        |
JOURNEYS 12             1                        1  1        |
JOURNEYS 13     1       1                        1  1        |
JOURNEYS 14     1                                   1        |
JOURNEYS 15     1                                1  1        |
JOURNEYS 16     1                                            |
JOURNEYS 17                                             1    |
JOURNEYS 18     1                                1  1        | -> same as J15 but arrive later
JOURNEYS 19              1   1   1                       1   | -> same as J3 but arrive later
"""
SECTIONS_CHOICES = (
    (response_pb2.STREET_NETWORK, response_pb2.Bike, 'bike'),
    (response_pb2.PUBLIC_TRANSPORT, None, 'uri_1'),
    (response_pb2.PUBLIC_TRANSPORT, None, 'uri_2'),
    (response_pb2.PUBLIC_TRANSPORT, None, 'uri_3'),
    (response_pb2.PUBLIC_TRANSPORT, None, 'uri_4'),
    (response_pb2.PUBLIC_TRANSPORT, None, 'uri_5'),
    (response_pb2.PUBLIC_TRANSPORT, None, 'uri_6'),
    (response_pb2.PUBLIC_TRANSPORT, None, 'uri_7'),
    (response_pb2.PUBLIC_TRANSPORT, None, 'uri_8'),
    (response_pb2.PUBLIC_TRANSPORT, None, 'uri_9'),
    (response_pb2.STREET_NETWORK, response_pb2.Walking, 'walking'),
)

JOURNEYS = (
    # J1
    # 10/15/2015 @ 12:30pm (UTC)
    (1444905000, 'rapid', (1, 2, 3, 4, 10)),
    # J2
    # 10/15/2015 @ 12:35pm (UTC)
    (1444905300, 'rapid', (1, 2, 5, 6, 10)),
    # J3
    # 10/15/2015 @ 12:40pm (UTC)
    (1444905600, 'rapid', (2, 3, 4, 10)),
    # J4
    # 10/15/2015 @ 12:45pm (UTC)
    (1444905900, 'rapid', (2, 5, 6, 10)),
    # J5
    # 10/15/2015 @ 12:50pm (UTC)
    (1444906200, 'rapid', (0, 2, 3, 6, 10)),
    # J6
    # 10/15/2015 @ 12:55pm (UTC)
    (1444906500, 'rapid', (1, 2, 7, 10)),
    # J7
    # 10/15/2015 @ 13:00pm (UTC)
    (1444906800, 'rapid', (1, 2, 8, 10)),
    # J8
    # 10/15/2015 @ 13:05pm (UTC)
    (1444907100, 'rapid', (2, 7, 10)),
    # J9
    # 10/15/2015 @ 13:10pm (UTC)
    (1444907400, 'rapid', (2, 8, 10)),
    # J10
    # 10/15/2015 @ 13:15pm (UTC)
    (1444907700, 'rapid', (0, 2, 7, 10)),
    # J11
    # 10/15/2015 @ 13:20pm (UTC)
    (1444908800, 'rapid', (1, 2, 8, 9)),
    # J12
    # 10/15/2015 @ 13:25pm (UTC)
    (1444909100, 'rapid', (2, 8, 9)),
    # J13
    # 10/15/2015 @ 13:30pm (UTC)
    (1444909400, 'rapid', (0, 2, 8, 9)),
    # J14
    # 10/15/2015 @ 12:30pm (UTC)
    (1444905000, 'comfort', (0, 9)),
    # J15
    # 10/15/2015 @ 12:08pm (UTC)
    (1444903680, 'best', (0, 8, 9)),
    # J16
    # 10/15/2015 @ 12:08pm (UTC)
    (1444903680, 'non_pt_bike', (0,)),
    # J17
    # 10/15/2015 @ 12:05pm (UTC)
    (1444903500, 'non_pt_walk', (10,)),
    # J18 -> same as J15 but arrive later than J15
    # 10/15/2015 @ 12:10pm (UTC)
    (1444903800, 'rapid', (0, 8, 9)),
    # J19 -> same as J3 but arrive later than J3
    # 10/15/2015 @ 12:42pm (UTC)
    (1444905720, 'rapid', (2, 3, 4, 10)),
)


def build_mocked_response():
    response = response_pb2.Response()
    for jrny in JOURNEYS:
        arrival_time, jrny_type, sections_idx = jrny
        pb_j = response.journeys.add()
        pb_j.arrival_date_time = arrival_time
        if jrny_type:
            pb_j.type = jrny_type
        for idx in sections_idx:
            section_type, network_mode, line_uri = SECTIONS_CHOICES[idx]
            section = pb_j.sections.add()
            section.type = section_type
            if network_mode:
                section.street_network.mode = network_mode
            if line_uri:
                section.uris.line = line_uri
    return response


def create_candidate_pool_and_sections_set_test():
    """
    Given response, the tested function should return a candidate pool and a section set
    """
    mocked_pb_response = build_mocked_response()
    candidates_pool, sections_set, idx_jrny_must_keep = \
        new_default._build_candidate_pool_and_sections_set(mocked_pb_response)

    # We got 17 journeys in all and 4 of them are tagged with 'best', 'comfort', 'non_pt_bike', 'non_pt_walk'
    assert candidates_pool.shape[0] == 19
    assert len(idx_jrny_must_keep) == 4

    assert len(sections_set) == 11


def build_candidate_pool_and_sections_set_test():
    mocked_pb_response = build_mocked_response()
    candidates_pool, sections_set, idx_jrny_must_keep = \
        new_default._build_candidate_pool_and_sections_set(mocked_pb_response)
    selected_sections_matrix = new_default._build_selected_sections_matrix(sections_set, candidates_pool)

    # selected_sections_matrix should have 17 lines(17 journeys) and 11 columns(11 sections)
    assert selected_sections_matrix.shape == (19, 11)

    # it's too verbose to check the entire matrix... we check only two lines
    assert [1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1] in selected_sections_matrix
    assert [0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1] in selected_sections_matrix


def get_sorted_solutions_indexes_test():
    mocked_pb_response = build_mocked_response()
    candidates_pool, sections_set, idx_jrny_must_keep = \
        new_default._build_candidate_pool_and_sections_set(mocked_pb_response)
    selected_sections_matrix = new_default._build_selected_sections_matrix(sections_set, candidates_pool)
    # 4 journeys are must-have, we'd like to select another 5 journeys
    best_indexes, selection_matrix = \
        new_default._get_sorted_solutions_indexes(selected_sections_matrix, (5 + 4), idx_jrny_must_keep)

    assert best_indexes.shape[0] == 33
    assert all(selection_matrix[best_indexes[0]] == [0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0])


def culling_jounreys_1_test():
    """
    Test when max_nb_journeys is bigger than journey's length in response,
    none of journeys are cut out
    """
    mocked_pb_response = build_mocked_response()
    nb_journeys = len(mocked_pb_response.journeys)
    mocked_request = {'max_nb_journeys': nb_journeys + 1, 'debug': False}
    new_default.culling_journeys(mocked_pb_response, mocked_request)
    assert len(mocked_pb_response.journeys) == nb_journeys


def culling_jounreys_2_test():
    """
    max_nb_journeys equals to nb of must-keep journeys ('comfort', 'best')
    """
    mocked_pb_response = build_mocked_response()
    mocked_request = {'max_nb_journeys': 2, 'debug': False}
    new_default.culling_journeys(mocked_pb_response, mocked_request)
    assert len(mocked_pb_response.journeys) == 2
    assert all([j.type in new_default.JOURNEY_TYPES_TO_RETAIN
                for j in mocked_pb_response.journeys])


def culling_jounreys_3_test():
    mocked_pb_response = build_mocked_response()
    mocked_request = {'max_nb_journeys': 6, 'debug': False, 'datetime': 1444903200}
    new_default.culling_journeys(mocked_pb_response, mocked_request)
    assert len(mocked_pb_response.journeys) == 6

    journey_uris = {(u'uri_1', u'uri_2', u'uri_5', u'uri_6', u'walking'),
                    (u'uri_2', u'uri_3', u'uri_4', u'walking'),
                    (u'bike', u'uri_2', u'uri_7', u'walking'),
                    (u'bike', u'uri_9'),
                    (u'bike', u'uri_8', u'uri_9'),
                    (u'bike',),
                    (u'walking',)}
    for j in mocked_pb_response.journeys:
        assert tuple(s.uris.line for s in j.sections) in journey_uris


def culling_jounreys_4_test():
    """
    When max_nb_journeys == 3 and nb_must_have_journeys == 4
    """
    mocked_pb_response = build_mocked_response()
    mocked_request = {'max_nb_journeys': 3, 'debug': False, 'datetime': 1444903200}
    new_default.culling_journeys(mocked_pb_response, mocked_request)
    assert len(mocked_pb_response.journeys) == 3
    for jrny in mocked_pb_response.journeys:
        # 'non_pt_bike' shouldn't appear in returned journeys
        assert jrny.type in ('best', 'comfort', 'non_pt_walk')


def merge_responses_on_errors_test():
    """
    check the merge responses when several errors are provided
    """
    resp1 = response_pb2.Response()
    resp1.error.id = response_pb2.Error.date_out_of_bounds
    resp1.error.message = "you're out of the bound"
    resp2 = response_pb2.Response()
    resp2.error.id = response_pb2.Error.bad_format
    resp2.error.message = "you've been bad"
    r = [resp1, resp2]
    
    merged_response = new_default.merge_responses(r)
    
    assert merged_response.HasField(b'error')
    assert merged_response.error.id == response_pb2.Error.no_solution
    # both messages must be in the composite error
    assert resp1.error.message in merged_response.error.message
    assert resp2.error.message in merged_response.error.message


def add_pt_sections(journey):
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    section = journey.sections.add()
    section.type = response_pb2.PUBLIC_TRANSPORT
    section = journey.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking


def create_next_kraken_request_test():
    request_clock = {'datetime': 100000, 'clockwise': True}
    request_anticlock = {'datetime': 110000, 'clockwise': False}
    response = response_pb2.Response()

    # test with one walk and one pt
    journey_walk = response.journeys.add()
    journey_walk.departure_date_time = 100000
    journey_walk.arrival_date_time = 101500
    journey_walk.duration = 1500
    section = journey_walk.sections.add()
    section.type = response_pb2.STREET_NETWORK
    section.street_network.mode = response_pb2.Walking
    journey_pt = response.journeys.add()
    journey_pt.departure_date_time = 100500
    journey_pt.arrival_date_time = 103000
    journey_pt.duration = 2500
    add_pt_sections(journey_pt)
    new_def = new_default.Scenario()
    # clockwise: we should have the next request one second after departure of pt journey
    next_request = new_def.create_next_kraken_request(request_clock, [response])
    assert next_request == {'datetime': 100501, 'clockwise': True}
    # anticlockwise: we should have the next request one second before arrival of pt journey
    next_request = new_def.create_next_kraken_request(request_anticlock, [response])
    assert next_request == {'datetime': 102999, 'clockwise': False}

    # test with one walk, 2 pt 1005->1030 and 1010->1025
    journey_pt = response.journeys.add()
    journey_pt.departure_date_time = 101000
    journey_pt.arrival_date_time = 102500
    journey_pt.duration = 1500
    add_pt_sections(journey_pt)
    # clockwise: we should have the next request one second after departure of pt journey arriving at 1025
    next_request = new_def.create_next_kraken_request(request_clock, [response])
    assert next_request == {'datetime': 101001, 'clockwise': True}
    # anticlockwise: we should have the next request one second before arrival of pt journey leaving at 1010
    next_request = new_def.create_next_kraken_request(request_anticlock, [response])
    assert next_request == {'datetime': 102499, 'clockwise': False}

    # test with one walk, 3 pt 1005->1030, 1010->1025 and 1015->1025
    journey_pt = response.journeys.add()
    journey_pt.departure_date_time = 101500
    journey_pt.arrival_date_time = 102500
    journey_pt.duration = 1000
    add_pt_sections(journey_pt)
    # clockwise: we should have the next request one second after departure of pt journey 1015->1025
    next_request = new_def.create_next_kraken_request(request_clock, [response])
    assert next_request == {'datetime': 101501, 'clockwise': True}

    # test with one walk, 4 pt 1005->1030, 1010->1025, 1015->1025 and 1015->1020
    journey_pt = response.journeys.add()
    journey_pt.departure_date_time = 101500
    journey_pt.arrival_date_time = 102000
    journey_pt.duration = 500
    add_pt_sections(journey_pt)
    # anticlockwise: we should have the next request one second after departure of pt journey 1015->1020
    next_request = new_def.create_next_kraken_request(request_anticlock, [response])
    assert next_request == {'datetime': 101999, 'clockwise': False}
