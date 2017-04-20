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
from jormungandr.scenarios import default, helpers
from jormungandr.scenarios.default import is_admin
import copy
import logging
from jormungandr.scenarios.utils import JourneySorter, are_equals, compare, add_link
from navitiacommon import response_pb2
from operator import indexOf, attrgetter
from datetime import datetime
import pytz
from collections import defaultdict, OrderedDict
from jormungandr.scenarios.helpers import has_bss_first_and_walking_last, has_walking_first_and_bss_last, \
        has_bss_first_and_bss_last, has_bike_first_and_walking_last, has_bike_first_and_bss_last, \
        is_non_pt_bss, is_non_pt_bike, is_non_pt_walk
import itertools
from jormungandr.utils import pb_del_if, timestamp_to_str

non_pt_types = ['non_pt_walk', 'non_pt_bike', 'non_pt_bss']

def is_pure_tc(journey):
    has_tc = False
    for section in journey.sections:
        if section.type in (response_pb2.STREET_NETWORK, response_pb2.CROW_FLY) \
                and section.street_network.mode in (response_pb2.Bike, response_pb2.Car, response_pb2.Bss):
            return False
        if section.type == response_pb2.PUBLIC_TRANSPORT:
            has_tc = True

    return has_tc


def has_bike_and_tc(journey):
    has_tc = False
    has_bike = False
    in_bss = False
    for section in journey.sections:
        if section.type == response_pb2.BSS_RENT:
            in_bss = True
        if section.type == response_pb2.BSS_PUT_BACK:
            in_bss = False
        if section.type in (response_pb2.STREET_NETWORK, response_pb2.CROW_FLY) \
                and section.street_network.mode == response_pb2.Bike \
                and not in_bss:
            has_bike = True
        if section.type == response_pb2.PUBLIC_TRANSPORT:
            has_tc = True

    return has_tc and has_bike

def has_bss_and_tc(journey):
    has_tc = False
    has_bss = False
    has_other_fallback_mode = False
    for section in journey.sections:
        if section.type == response_pb2.BSS_RENT:
            has_bss = True
        if section.type == response_pb2.PUBLIC_TRANSPORT:
            has_tc = True
        if section.type in (response_pb2.STREET_NETWORK, response_pb2.CROW_FLY) \
                and section.street_network.mode in (response_pb2.Bike, response_pb2.Car, response_pb2.Bss) \
                and not has_bss:
            #if we take a bike before renting a bss then we have a bike mode and not a bss
            has_other_fallback_mode = True

    return has_tc and has_bss and not has_other_fallback_mode

def has_car_and_tc(journey):
    has_tc = False
    has_car = False
    for section in journey.sections:
        if section.type in (response_pb2.STREET_NETWORK, response_pb2.CROW_FLY) \
                and section.street_network.mode == response_pb2.Car:
            has_car = True
        if section.type == response_pb2.PUBLIC_TRANSPORT:
            has_tc = True

    return has_tc and has_car


def is_alternative(journey):
    return is_non_pt_bike(journey) or is_non_pt_walk(journey)



class DestineoJourneySorter(JourneySorter):
    """
        sort the journeys for destineo:

         - N pure TC journeys, sort by departure time
         - 1 direct BSS journey
         - N TC journeys with bss as fallback mode, sort by departure time
         - N TC journeys with bike as fallback mode, sort by departure time
         - 1 journey with car as fallback mode
         - 1 direct walking journey
         - 1 direct bike journey

    """
    def __init__(self, clockwise, timezone):
        super(DestineoJourneySorter, self).__init__(clockwise)
        self.timezone = timezone

    def sort_by_time(self, j1, j2):
        # we don't care about the clockwise for destineo
        if j1.departure_date_time != j2.departure_date_time:
            return -1 if j1.departure_date_time < j2.departure_date_time else 1

        return self.sort_by_duration_and_transfert(j1, j2)

    def __call__(self, j1, j2):

        j1_dt = datetime.utcfromtimestamp(j1.departure_date_time)
        j2_dt = datetime.utcfromtimestamp(j2.departure_date_time)
        j1_date = pytz.utc.localize(j1_dt).astimezone(pytz.timezone(self.timezone)).date()
        j2_date = pytz.utc.localize(j2_dt).astimezone(pytz.timezone(self.timezone)).date()

        #we first sort by date, we want the journey of the first day in first even if it's a non_pt
        if j1_date != j2_date:
            if j1_date < j2_date:
                return -1
            else:
                return 1

        #first we want the pure tc journey
        if is_pure_tc(j1) and is_pure_tc(j2):
            return self.sort_by_time(j1, j2)
        elif is_pure_tc(j1):
            return -1
        elif is_pure_tc(j2):
            return 1

        #after that we want the non pt bss
        if is_non_pt_bss(j1) and is_non_pt_bss(j2):
            return self.sort_by_time(j1, j2)
        elif is_non_pt_bss(j1):
            return -1
        elif is_non_pt_bss(j2):
            return 1

        #then the TC with bss fallback
        if has_bss_and_tc(j1) and has_bss_and_tc(j2):
            return self.sort_by_time(j1, j2)
        elif has_bss_and_tc(j1):
            return -1
        elif has_bss_and_tc(j2):
            return 1

        #then the TC with bike fallback
        if has_bike_and_tc(j1) and has_bike_and_tc(j2):
            return self.sort_by_time(j1, j2)
        elif has_bike_and_tc(j1):
            return -1
        elif has_bike_and_tc(j2):
            return 1

        #if there is any, the car fallback with TC
        if has_car_and_tc(j1) and has_car_and_tc(j2):
            return self.sort_by_time(j1, j2)
        elif has_car_and_tc(j1):
            return -1
        elif has_car_and_tc(j2):
            return 1

        #the walking only trip
        if is_non_pt_walk(j1) and is_non_pt_walk(j2):
            return self.sort_by_time(j1, j2)
        elif is_non_pt_walk(j1):
            return -1
        elif is_non_pt_walk(j2):
            return 1

        #the bike only trip
        if is_non_pt_bike(j1) and is_non_pt_bike(j2):
            return self.sort_by_time(j1, j2)
        elif is_non_pt_bike(j1):
            return -1
        elif is_non_pt_bike(j2):
            return 1

        return 0


class Scenario(default.Scenario):

    def journeys(self, request, instance):
        logger = logging.getLogger(__name__)
        request_tc = copy.deepcopy(request)
        request_tc['origin_mode'] = ['walking']
        request_tc['destination_mode'] = ['walking']
        max_nb_journeys = request_tc['max_nb_journeys']
        request_tc['max_nb_journeys'] = None

        request_alternative = copy.deepcopy(request)
        request_alternative['min_nb_journeys'] = None
        request_alternative['max_nb_journeys'] = None
        if 'walking' in request_alternative['origin_mode'] \
                and len(set(request_alternative['origin_mode'])) > 1:
            request_alternative['origin_mode'].remove('walking')
        if 'walking' in request_alternative['destination_mode'] \
                and len(set(request_alternative['destination_mode'])) > 1:
            request_alternative['destination_mode'].remove('walking')
        logger.debug('journeys only on TC')
        response_tc = super(Scenario, self).journeys(request_tc, instance)

        if not request['debug']:
            self._remove_extra_journeys(response_tc.journeys, max_nb_journeys,
                                        request['clockwise'],
                                        timezone=instance.timezone)

        max_duration = self._find_max_duration(response_tc.journeys, instance, request['clockwise'])
        if max_duration:
            #we find the max_duration with the pure tc call, so we use it for the alternatives
            request_alternative['max_duration'] = int(max_duration)

        if not all([is_admin(request['origin']), is_admin(request['destination'])]):
            logger.debug('journeys with alternative mode')
            response_alternative = self._get_alternatives(request_alternative, instance, response_tc.journeys)
            logger.debug('merge and sort responses')
            self.merge_response(response_tc, response_alternative)
        else:
            logger.debug('don\'t search alternative for admin to admin journeys')
            self._remove_non_pt_walk(response_tc.journeys)

        for journey in response_tc.journeys:
            if journey.type == 'best':
                journey.type = 'rapid'
        self._custom_sort_journeys(response_tc, clockwise=request['clockwise'], timezone=instance.timezone)
        self.choose_best(response_tc)
        for journey in response_tc.journeys:
            if is_alternative(journey):
                journey.tags.append('alternative')
            if is_pure_tc(journey):
                #we need this for the pagination
                journey.tags.append('is_pure_tc')

        # for destineo, we use a custom pagination
        response_tc.ClearField(str('links'))
        self._compute_pagination_links(response_tc, instance, request['clockwise'])

        return response_tc

    def _get_alternatives(self, args, instance, tc_journeys):
        logger = logging.getLogger(__name__)
        response_alternative = super(Scenario, self).journeys(args, instance)
        #we don't want to choose an alternative already in the pure_pt solutions, it can happens since bss allow walking solutions
        self._remove_already_present_journey(response_alternative.journeys, tc_journeys)

        if not args['debug']:
            # in debug we don't filter
            self._choose_best_alternatives(response_alternative.journeys)
        return response_alternative

    def _remove_already_present_journey(self, alternative_journeys, tc_journeys):
        logger = logging.getLogger(__name__)
        to_delete = []
        for idx, journey in enumerate(alternative_journeys):
            for journey_tc in tc_journeys:
                if are_equals(journey, journey_tc):
                    to_delete.append(idx)
        logger.debug('remove %s alternative journey already present in TC response: %s',
                len(to_delete), [alternative_journeys[i].type for i in to_delete])
        to_delete.sort(reverse=True)
        for idx in to_delete:
            del alternative_journeys[idx]


    def _remove_extra_journeys(self, journeys, max_nb_journeys, clockwise, timezone):
        """
        for destineo we want to filter certain journeys

        - we want at most 'max_nb_journeys', but we always want to keep the non_pt_walk

        - we don't want 2 alternatives using the same buses but with different boarding stations

        for similar journeys, we want to pick up:
            - the earliest one (for clockwise, else tardiest)
            - the one that leave the tardiest (for clockwise, else earliest)
            - the one with the less fallback (we know it's walking)
        """
        to_delete = []

        def same_vjs(j):
            #same departure date and vjs
            journey_dt = datetime.utcfromtimestamp(j.departure_date_time)
            journey_date = pytz.utc.localize(journey_dt).astimezone(pytz.timezone(timezone)).date()
            yield journey_date
            for s in j.sections:
                yield s.uris.vehicle_journey

        def get_journey_to_remove(idx_j1, j1, idx_j2, j2):
            if clockwise:
                if j1.arrival_date_time != j2.arrival_date_time:
                    return idx_j1 if j1.arrival_date_time > j2.arrival_date_time else idx_j2
                if j1.departure_date_time != j2.departure_date_time:
                    return idx_j1 if j1.departure_date_time < j2.departure_date_time else idx_j2
            else:
                if j1.departure_date_time != j2.departure_date_time:
                    return idx_j1 if j1.departure_date_time < j2.departure_date_time else idx_j2
                if j1.arrival_date_time != j2.arrival_date_time:
                    return idx_j1 if j1.arrival_date_time > j2.arrival_date_time else idx_j2

            return idx_j1 if helpers.walking_duration(j1) > helpers.walking_duration(j2) else idx_j2

        for (idx1, j1), (idx2, j2) in itertools.combinations(enumerate(journeys), 2):
            if idx1 in to_delete or idx2 in to_delete:
                continue

            if not compare(j1, j2, same_vjs):
                continue

            to_delete.append(get_journey_to_remove(idx1, j1, idx2, j2))

        if max_nb_journeys:
            count = 0
            for idx, journey in enumerate(journeys):
                if idx in to_delete:
                    continue
                if journey.type == 'non_pt_walk':
                    continue
                if count >= max_nb_journeys:
                    to_delete.append(idx)
                count += 1

        to_delete.sort(reverse=True)
        logger = logging.getLogger(__name__)
        logger.debug('remove %s extra journeys: %s', len(to_delete), [journeys[i].type for i in to_delete])
        for idx in to_delete:
            del journeys[idx]


    def _remove_non_pt_walk(self, journeys):
        nb_deleted = pb_del_if(journeys, lambda j: j.type == 'non_pt_walk')
        logger = logging.getLogger(__name__)
        logger.debug('remove %s non_pt_walk journey', nb_deleted)

    def _choose_best_alternatives(self, journeys):
        to_keep = []
        mapping = defaultdict(list)
        best = None
        functors = OrderedDict([
            ('bss_walking', has_bss_first_and_walking_last),
            ('walking_bss', has_walking_first_and_bss_last),
            ('bss_bss', has_bss_first_and_bss_last),
            ('bike_walking', has_bike_first_and_walking_last),
            ('bike_bss', has_bike_first_and_bss_last),
            ('car', has_car_and_tc)
        ])
        for idx, journey in enumerate(journeys):
            if journey.type in non_pt_types:
                to_keep.append(idx)
            for key, func in functors.items():
                if func(journey):
                    mapping[key].append(journey)

        for key, _ in functors.items():
            if not best and mapping[key]:
                best = min(mapping[key], key=attrgetter('duration'))
                to_keep.append(indexOf(journeys, best))

        logger = logging.getLogger(__name__)
        logger.debug('from alternatives we keep: %s', [journeys[i].type for i in to_keep])
        to_delete = list(set(range(len(journeys))) - set(to_keep))
        to_delete.sort(reverse=True)
        for idx in to_delete:
            del journeys[idx]

    def _custom_sort_journeys(self, response, timezone, clockwise=True):
        if len(response.journeys) > 1:
            response.journeys.sort(DestineoJourneySorter(clockwise, timezone))

    @staticmethod
    def filter_journeys(journeys):
        section_is_pt = lambda section: section.stop_date_times
        filter_journey = lambda journey: any(section_is_pt(section) for section in journey.sections)
        filter_journey_pure_tc = lambda journey: 'is_pure_tc' in journey.tags

        list_journeys = filter(filter_journey_pure_tc, journeys)
        if not list_journeys:
            #if there is no pure tc journeys, we consider all journeys with TC
            list_journeys = filter(filter_journey, journeys)
        return list_journeys

    def _add_next_link(self, resp, params, clockwise):
        journeys = self.filter_journeys(resp.journeys)
        if not journeys:
            return None
        next_journey = max(journeys, key=lambda j: j.departure_date_time)
        params['datetime'] = timestamp_to_str(next_journey.departure_date_time + 60)
        params['datetime_represents'] = 'departure'
        add_link(resp, rel='next', **params)

    def _add_prev_link(self, resp, params, clockwise):
        journeys = self.filter_journeys(resp.journeys)
        if not journeys:
            return None
        next_journey = max(journeys, key=lambda j: j.arrival_date_time)
        params['datetime'] = timestamp_to_str(next_journey.arrival_date_time - 60)
        params['datetime_represents'] = 'arrival'
        add_link(resp, rel='prev', **params)

