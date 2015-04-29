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
from collections import namedtuple
import logging
import itertools
import datetime
from jormungandr.scenarios.utils import compare
from navitiacommon import response_pb2


def filter_journeys(response_list, request):
    """
    Filter by side effect the list of pb responses's journeys

    first draft, we only remove the journeys with the same vjs
    """
    if request.get('debug', False):
        return response_list

    _filter_similar_journeys(response_list, request)

    return response_list


class JourneysIdx:
    def __init__(self, response_idx, journey_idx, journey):
        self.response_idx = response_idx
        self.journey_idx = journey_idx
        self.journey = journey

    @property
    def key(self):
        return self.response_idx, self.journey_idx


def _get_worst_similar_vjs(journey_idxs1, journey_idxs2, request):
    """
    Decide which is the worst journey between 2 similar journeys.

    The choice is made on:
     - asap
     - less fallback
     - duration
    """
    j1 = journey_idxs1.journey
    j2 = journey_idxs2.journey
    if request.get('clockwise', True):
        if j1.arrival_date_time != j2.arrival_date_time:
            return journey_idxs1 if j1.arrival_date_time < j2.arrival_date_time else journey_idxs2
    else:
        if j1.departure_date_time != j2.departure_date_time:
            return journey_idxs1 if j1.departure_date_time > j2.departure_date_time else journey_idxs2

    fallback1 = fallback_duration(j1)
    fallback2 = fallback_duration(j2)

    if fallback1 != fallback2:
        return journey_idxs1 if fallback1 < fallback2 else journey_idxs2

    return journey_idxs1 if j1.duration < j2.duration else journey_idxs2


def _filter_similar_journeys(response_list, request):
    """
    for the moment very simple filter.

    we filter the journeys with the same vjs

    in case of similar journeys we let _get_worst_similar_vjs decide which one to delete
    """
    logger = logging.getLogger(__name__)
    to_delete = []

    # for clarity purpose we build a temporary list
    journeys = [JourneysIdx(r_idx, j_idx, j) for r_idx, r in enumerate(response_list)
                for j_idx, j in enumerate(r.journeys)]

    #DEBUG
    for j in journeys:
        _debug_journey(j)

    for tuple1, tuple2 in itertools.combinations(journeys, 2):
        if tuple2.key in to_delete:
            continue
        if compare(tuple1.journey, tuple2.journey, similar_journeys_generator):
            #chose the best
            worst = _get_worst_similar_vjs(tuple1, tuple2, request)
            logger.debug("the journeys {}, {} are similar, we delete {}".format(tuple1.key,
                                                                                tuple2.key,
                                                                                worst.key))
            to_delete.append(worst.key)

    if to_delete:
        logger.info('filtering {} journeys because they are dupplicates ({})'.format(len(to_delete), to_delete))

    for response_idx, j_idx in reversed(to_delete):
        del response_list[response_idx].journeys[j_idx]


def similar_journeys_generator(journey):
    for s in journey.sections:
        yield s.pt_display_informations.uris.vehicle_journey


def fallback_duration(journey):
    duration = 0
    for section in journey.sections:
        if section.type in (response_pb2.STREET_NETWORK, response_pb2.CROW_FLY):
            duration += section.duration

    return duration


def _debug_journey(journey_and_idx):
    """
    For debug purpose print the journey
    """
    logger = logging.getLogger(__name__)

    __shorten = {
        'Walking': 'W',
        'Bike': 'B',
        'Car': 'C',
        'BSS_RENT': '/',
        'BSS_PUT_BACK': '\\',
        'PARK': '/',
        'LEAVE_PARKING': '\\',
        'TRANSFER': 'T',
        'WAITING': '.',
    }

    def shorten(name):
        return __shorten.get(name, name)

    sections = []
    for s in journey_and_idx.journey.sections:
        if s.type == response_pb2.PUBLIC_TRANSPORT:
            sections.append("{line} ({vj})".format(line=s.pt_display_informations.uris.line,
                                                   vj=s.pt_display_informations.uris.vehicle_journey))
        elif s.type == response_pb2.STREET_NETWORK:
            sections.append(shorten(response_pb2.StreetNetworkMode.Name(s.street_network.mode)))
        else:
            sections.append(shorten(response_pb2.SectionType.Name(s.type)))

    def _datetime_to_str(ts):
        #DEBUG, no tz handling
        dt = datetime.datetime.utcfromtimestamp(ts)
        return dt.strftime("%dT%H:%M:%S")

    logger.debug("journey {}-{}: {dep} -> {arr} - {duration} ({fallback} map) | ({sec})".format(
        journey_and_idx.response_idx, journey_and_idx.journey_idx,
        dep=_datetime_to_str(journey_and_idx.journey.departure_date_time),
        arr=_datetime_to_str(journey_and_idx.journey.arrival_date_time),
        duration=datetime.timedelta(seconds=journey_and_idx.journey.duration),
        fallback=datetime.timedelta(seconds=fallback_duration(journey_and_idx.journey)),
        sec=" - ".join(sections)))