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
from jormungandr.scenarios import default
import copy
import logging
from jormungandr.scenarios.utils import JourneySorter
from navitiacommon import response_pb2
from operator import itemgetter
from datetime import datetime, timedelta

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
            #if we take a bike before renting a bss then we have a bike mode and not a bss
            has_car = True
        if section.type == response_pb2.PUBLIC_TRANSPORT:
            has_tc = True

    return has_tc and has_car

def is_non_pt_bss(journey):
    return journey.type == 'non_pt_bss'

def is_non_pt_walk(journey):
    return journey.type == 'non_pt_walk'

def is_non_pt_bike(journey):
    return journey.type == 'non_pt_bike'

def is_alternative(journey):
    return is_non_pt_bike(journey) or is_non_pt_walk(journey)

def bike_duration(journey):
    duration = 0
    in_bss = False
    for section in journey.sections:
        if section.type == response_pb2.BSS_RENT:
            in_bss = True
        if section.type == response_pb2.BSS_PUT_BACK:
            in_bss = False
        if section.type in (response_pb2.STREET_NETWORK, response_pb2.CROW_FLY) \
                and section.street_network.mode == response_pb2.Bike \
                and not in_bss:
            duration = duration + section.duration

    return duration

def car_duration(journey):
    duration = 0
    for section in journey.sections:
        if section.type in (response_pb2.STREET_NETWORK, response_pb2.CROW_FLY) \
                and section.street_network.mode == response_pb2.Car:
            duration = duration + section.duration

    return duration

def tc_duration(journey):
    duration = 0
    for section in journey.sections:
        if section.type == response_pb2.PUBLIC_TRANSPORT:
            duration = duration + section.duration

    return duration

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
    def __init__(self, clockwise):
        super(DestineoJourneySorter, self).__init__(clockwise)

    def sort_by_time(self, j1, j2):
        # we don't care about the clockwise for destineo
        if j1.departure_date_time != j2.departure_date_time:
            return -1 if j1.departure_date_time < j2.departure_date_time else 1

        return self.sort_by_duration_and_transfert(j1, j2)

    def __call__(self, j1, j2):
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

        request_alternative = copy.deepcopy(request)
        request_alternative['min_nb_journeys'] = None
        request_alternative['max_nb_journeys'] = None
        logger.debug('journeys only on TC')
        response_tc = super(Scenario, self).journeys(request_tc, instance)

        logger.debug('journeys with alternative mode')
        response_alternative = super(Scenario, self).journeys(request_alternative, instance)

        logger.debug('merge and sort reponses')

        self._remove_car_if_possible(response_alternative)
        self._remove_not_long_enough_fallback(response_alternative.journeys, instance)
        self._remove_not_long_enough_tc_with_fallback(response_alternative.journeys, instance)

        self.merge_response(response_tc, response_alternative)
        for journey in response_tc.journeys:
            if journey.type == 'best':
                journey.type = 'rapid'
        self._custom_sort_journeys(response_tc, request['clockwise'])
        self.choose_best(response_tc)
        for journey in response_tc.journeys:
            if is_alternative(journey):
                journey.tags.append('alternative')
            if is_pure_tc(journey):
                #we need this for the pagination
                journey.tags.append('is_pure_tc')

        return response_tc


    def _remove_not_long_enough_fallback(self, journeys, instance):
        to_delete = []
        for idx, journey in enumerate(journeys):
            if journey.type in non_pt_types:
                continue
            bike_dur = bike_duration(journey)
            car_dur = car_duration(journey)
            if bike_dur and bike_dur < instance.destineo_min_bike:
                to_delete.append(idx)
            elif car_dur and car_dur < instance.destineo_min_car:
                to_delete.append(idx)

        logger = logging.getLogger(__name__)
        logger.debug('remove %s journey with not enough fallback duration: %s',
                len(to_delete), [journeys[i].type for i in to_delete])
        to_delete.sort(reverse=True)
        for idx in to_delete:
            del journeys[idx]


    def _remove_not_long_enough_tc_with_fallback(self, journeys, instance):
        to_delete = []
        for idx, journey in enumerate(journeys):
            if journey.type in non_pt_types:
                continue
            bike_dur = bike_duration(journey)
            car_dur = car_duration(journey)
            tc_dur = tc_duration(journey)
            if bike_dur and tc_dur < instance.destineo_min_tc_with_bike:
                to_delete.append(idx)
            elif car_dur and tc_dur < instance.destineo_min_tc_with_car:
                to_delete.append(idx)

        logger = logging.getLogger(__name__)
        logger.debug('remove %s journey with not enough tc duration: %s',
                len(to_delete), [journeys[i].type for i in to_delete])
        to_delete.sort(reverse=True)
        for idx in to_delete:
            del journeys[idx]

    def _remove_car_if_possible(self, response):
        fallback_journeys = [journey for journey in response.journeys if journey.type not in non_pt_types]
        if len(fallback_journeys) > 1:
            to_delete = [idx for idx, j in enumerate(response.journeys) if j.type == 'car']
            for idx in to_delete:
                del response.journeys[idx]

    def _custom_sort_journeys(self, response, clockwise=True):
        if len(response.journeys) > 1:
            response.journeys.sort(DestineoJourneySorter(clockwise=clockwise))

    def filter_journeys(self, journeys):
        section_is_pt = lambda section: section['type'] == "public_transport"\
                           or section['type'] == "on_demand_transport"
        filter_journey = lambda journey: 'arrival_date_time' in journey and\
                             journey['arrival_date_time'] != '' and\
                             "sections" in journey and\
                             any(section_is_pt(section) for section in journey['sections'])
        filter_journey_pure_tc = lambda journey: 'is_pure_tc' in journey['tags']

        list_journeys = filter(filter_journey_pure_tc, journeys)
        if not list_journeys:
            #if there is no pure tc journeys, we consider all journeys with TC
            list_journeys = filter(filter_journey, journeys)
        return list_journeys


    def extremes(self, resp):
        logger = logging.getLogger(__name__)
        logger.debug('calling extremes for destineo')
        if 'journeys' not in resp:
            return (None, None)
        list_journeys = self.filter_journeys(resp['journeys'])
        if not list_journeys:
            return (None, None)
        prev_journey = min(list_journeys, key=itemgetter('arrival_date_time'))
        next_journey = max(list_journeys, key=itemgetter('departure_date_time'))
        f_datetime = "%Y%m%dT%H%M%S"
        f_departure = datetime.strptime(next_journey['departure_date_time'], f_datetime)
        f_arrival = datetime.strptime(prev_journey['arrival_date_time'], f_datetime)
        datetime_after = f_departure + timedelta(minutes=1)
        datetime_before = f_arrival - timedelta(minutes=1)

        return (datetime_before, datetime_after)
