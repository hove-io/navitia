# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
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
import navitiacommon.type_pb2 as type_pb2
import navitiacommon.response_pb2 as response_pb2
import navitiacommon.request_pb2 as request_pb2
from future.moves.itertools import zip_longest
from jormungandr.fallback_modes import FallbackModes
import requests as requests
import re
from collections import defaultdict
from string import Formatter
from copy import deepcopy
import six

places_type = {
    'stop_area': type_pb2.STOP_AREA,
    'stop_point': type_pb2.STOP_POINT,
    'address': type_pb2.ADDRESS,
    'poi': type_pb2.POI,
    'administrative_region': type_pb2.ADMINISTRATIVE_REGION,
}

free_floatings_type = ['BIKE', 'SCOOTER', 'MOTORSCOOTER', 'STATION', 'CAR', 'OTHER']


pt_object_type = {
    'network': type_pb2.NETWORK,
    'commercial_mode': type_pb2.COMMERCIAL_MODE,
    'line': type_pb2.LINE,
    'route': type_pb2.ROUTE,
    'stop_area': type_pb2.STOP_AREA,
    'stop_point': type_pb2.STOP_POINT,
    'line_group': type_pb2.LINE_GROUP,
}

PSEUDO_DURATION_FACTORS = ((1, -1, 'departure_date_time'), (-1, 1, 'arrival_date_time'))

mode_weight = {
    FallbackModes.taxi: 6,
    FallbackModes.ridesharing: 5,
    FallbackModes.car: 4,
    FallbackModes.car_no_park: 4,
    FallbackModes.bike: 3,
    FallbackModes.bss: 2,
    FallbackModes.walking: 1,
}


def compare(obj1, obj2, compare_generator):
    """
    To decide that 2 objects are equals, we loop through all values of the
    compare_generator and stop at the first non equals value

    Note: the fillvalue is the value used when a generator is consumed
    (if the 2 generators does not return the same number of elt).
    by setting it to object(), we ensure that it will be !=
    from any values returned by the other generator
    """
    return all(
        a == b for a, b in zip_longest(compare_generator(obj1), compare_generator(obj2), fillvalue=object())
    )


class JourneySorter(object):
    """
    abstract class for journey sorter
    """

    def __init__(self, clockwise):
        self.clockwise = clockwise

    def __call__(self, j1, j2):
        raise NotImplementedError()

    def sort_by_duration_and_transfert(self, j1, j2):
        """
        we compare the duration, hence it will indirectly compare
        the departure for clockwise, and the arrival for not clockwise
        """
        if j1.duration != j2.duration:
            return j1.duration - j2.duration

        if j1.nb_transfers != j2.nb_transfers:
            return j1.nb_transfers - j2.nb_transfers

        # afterward we compare the non pt duration
        non_pt_duration_j1 = non_pt_duration_j2 = None
        for journey in [j1, j2]:
            non_pt_duration = 0
            for section in journey.sections:
                if section.type != response_pb2.PUBLIC_TRANSPORT:
                    non_pt_duration += section.duration
            if non_pt_duration_j1 is None:
                non_pt_duration_j1 = non_pt_duration
            else:
                non_pt_duration_j2 = non_pt_duration
        return non_pt_duration_j1 - non_pt_duration_j2


class ArrivalJourneySorter(JourneySorter):
    """
    Journey comparator for sort, on clockwise the sort is done by arrival time

    the comparison is different if the query is for clockwise search or not
    """

    def __init__(self, clockwise):
        super(ArrivalJourneySorter, self).__init__(clockwise)

    def __call__(self, j1, j2):
        if self.clockwise:
            # for clockwise query, we want to sort first on the arrival time
            if j1.arrival_date_time != j2.arrival_date_time:
                return -1 if j1.arrival_date_time < j2.arrival_date_time else 1
        else:
            # for non clockwise the first sort is done on departure
            if j1.departure_date_time != j2.departure_date_time:
                return -1 if j1.departure_date_time > j2.departure_date_time else 1

        return self.sort_by_duration_and_transfert(j1, j2)


class DepartureJourneySorter(JourneySorter):
    """
    Journey comparator for sort, on clockwise the sort is done by departure time
    the comparison is different if the query is for clockwise search or not
    """

    def __init__(self, clockwise):
        super(DepartureJourneySorter, self).__init__(clockwise)

    def __call__(self, j1, j2):

        if self.clockwise:
            # for clockwise query, we want to sort first on the departure time
            if j1.departure_date_time != j2.departure_date_time:
                return -1 if j1.departure_date_time < j2.departure_date_time else 1
        else:
            # for non clockwise the first sort is done on arrival
            if j1.arrival_date_time != j2.arrival_date_time:
                return -1 if j1.arrival_date_time > j2.arrival_date_time else 1

        return self.sort_by_duration_and_transfert(j1, j2)


def build_pagination(request, resp):
    pagination = resp.pagination
    if pagination.totalResult > 0:
        query_args = ""
        for key, value in request.items():
            if key != "startPage":
                if isinstance(value, type([])):
                    for v in value:
                        query_args += key + "=" + six.text_type(v) + "&"
                else:
                    query_args += key + "=" + six.text_type(value) + "&"
        if pagination.startPage > 0:
            page = pagination.startPage - 1
            pagination.previousPage = query_args
            pagination.previousPage += "start_page=%i" % page
        last_id_page = (pagination.startPage + 1) * pagination.itemsPerPage
        if last_id_page < pagination.totalResult:
            page = pagination.startPage + 1
            pagination.nextPage = query_args + "start_page=%i" % page


journey_sorter = {'arrival_time': ArrivalJourneySorter, 'departure_time': DepartureJourneySorter}


def get_or_default(request, val, default):
    """
    Flask add all API param in the request dict with None by default,
    so here is a simple helper to get the default value is the key
    is not in the dict or if the value is None
    """
    val = request.get(val, default)
    return val if val is not None else default


def updated_common_journey_request_with_default(request, instance):
    if request['max_walking_duration_to_pt'] is None:
        request['max_walking_duration_to_pt'] = instance.max_walking_duration_to_pt

    if request['max_bike_duration_to_pt'] is None:
        request['max_bike_duration_to_pt'] = instance.max_bike_duration_to_pt

    if request['max_bss_duration_to_pt'] is None:
        request['max_bss_duration_to_pt'] = instance.max_bss_duration_to_pt

    if request['max_car_duration_to_pt'] is None:
        request['max_car_duration_to_pt'] = instance.max_car_duration_to_pt

    if request['max_car_no_park_duration_to_pt'] is None:
        request['max_car_no_park_duration_to_pt'] = instance.max_car_no_park_duration_to_pt

    if request['max_ridesharing_duration_to_pt'] is None:
        request['max_ridesharing_duration_to_pt'] = instance.max_ridesharing_duration_to_pt

    if request['max_taxi_duration_to_pt'] is None:
        request['max_taxi_duration_to_pt'] = instance.max_taxi_duration_to_pt

    if request['max_bike_direct_path_distance'] is None:
        request['max_bike_direct_path_distance'] = instance.max_bike_direct_path_distance

    if request['max_bss_direct_path_distance'] is None:
        request['max_bss_direct_path_distance'] = instance.max_bss_direct_path_distance

    if request['max_walking_direct_path_distance'] is None:
        request['max_walking_direct_path_distance'] = instance.max_walking_direct_path_distance

    if request['max_car_direct_path_distance'] is None:
        request['max_car_direct_path_distance'] = instance.max_car_direct_path_distance

    if request['max_car_no_park_direct_path_distance'] is None:
        request['max_car_no_park_direct_path_distance'] = instance.max_car_no_park_direct_path_distance

    if request['max_ridesharing_direct_path_distance'] is None:
        request['max_ridesharing_direct_path_distance'] = instance.max_ridesharing_direct_path_distance

    if request['max_taxi_direct_path_distance'] is None:
        request['max_taxi_direct_path_distance'] = instance.max_taxi_direct_path_distance

    if request['max_transfers'] is None:
        request['max_transfers'] = instance.max_nb_transfers

    if request['walking_speed'] is None:
        request['walking_speed'] = instance.walking_speed

    if request['bike_speed'] is None:
        request['bike_speed'] = instance.bike_speed

    if request['bss_speed'] is None:
        request['bss_speed'] = instance.bss_speed

    if request['car_speed'] is None:
        request['car_speed'] = instance.car_speed

    if request['car_no_park_speed'] is None:
        request['car_no_park_speed'] = instance.car_no_park_speed

    if request['ridesharing_speed'] is None:
        request['ridesharing_speed'] = instance.ridesharing_speed

    if request['taxi_speed'] is None:
        request['taxi_speed'] = instance.taxi_speed

    for attr in ('bss_rent_duration', 'bss_rent_penalty', 'bss_return_duration', 'bss_return_penalty'):
        attr_value = request.get(attr)
        request[attr] = getattr(instance, attr, None) if attr_value is None else attr_value


def updated_request_with_default(request, instance):
    updated_common_journey_request_with_default(request, instance)

    if request['_min_car'] is None:
        request['_min_car'] = instance.min_car

    if request['_min_bike'] is None:
        request['_min_bike'] = instance.min_bike

    if request['_min_taxi'] is None:
        request['_min_taxi'] = instance.min_taxi

    if request['_min_ridesharing'] is None:
        request['_min_ridesharing'] = instance.min_ridesharing


def change_ids(new_journeys, response_index):
    """
    we have to change some id's on the response not to have id's collision between response

    we need to change the fare id , the section id and the fare ref in the journey
    """
    # we need to change the fare id, the section id and the fare ref in the journey
    def _change_id(old_id):
        return '{id}_{response_index}'.format(id=old_id, response_index=response_index)

    for ticket in new_journeys.tickets:
        ticket.id = _change_id(ticket.id)
        for i, _ in enumerate(ticket.section_id):
            ticket.section_id[i] = _change_id(ticket.section_id[i])

    for count, new_journey in enumerate(new_journeys.journeys):
        for i in range(len(new_journey.fare.ticket_id)):
            # the ticket_id inside of journeys must be the same as the one at the root of journey
            new_journey.fare.ticket_id[i] = _change_id(new_journey.fare.ticket_id[i])

        for section in new_journey.sections:
            section.id = _change_id(section.id)


def fill_uris(resp):
    if not resp:
        return
    for journey in resp.journeys:
        for section in journey.sections:
            if section.type != response_pb2.PUBLIC_TRANSPORT:
                continue
            if section.HasField(str("pt_display_informations")):
                uris = section.uris
                pt_infos = section.pt_display_informations
                uris.vehicle_journey = pt_infos.uris.vehicle_journey
                uris.line = pt_infos.uris.line
                uris.route = pt_infos.uris.route
                uris.commercial_mode = pt_infos.uris.commercial_mode
                uris.physical_mode = pt_infos.uris.physical_mode
                uris.network = pt_infos.uris.network


def get_pseudo_duration(journey, requested_dt, is_clockwise):
    f1, f2, attr = PSEUDO_DURATION_FACTORS[is_clockwise]
    return f1 * requested_dt + f2 * getattr(journey, attr)


def gen_all_combin(n, t):
    """
    :param n: number of elements in the whole set
    :param t: number of choices
    :return: iterator

    This function is used to generate all possible unordered combinations
    Combination = {c_1, c_2, ..., c_t |  all c_t belongs to S }
    where S is a set whose card(S) = n, c_t are indexes of elements in S (c as choice)

    Note that when n <= t, we consider there is only one possible combination.

    The function is a implementation of the algorithm L from the book of DONALD E.KNUTH's
    <The art of computer programming> Section7.2.1.3

    Example:
    Given 4 elements, list all possible unordered combinations when choosing 3 elements
    >>> list(gen_all_combin(4, 3))
    [[0, 1, 2], [0, 1, 3], [0, 2, 3], [1, 2, 3]]

    >>> list(gen_all_combin(3, 3))
    [[0, 1, 2]]

    We assume that it's also valid when n < t
    >>> list(gen_all_combin(3, 4))
    [[0, 1, 2]]

    """
    import numpy as np

    if n <= t:
        """
        nothing to do when n <= t, there is only one possible combination
        """
        yield list(range(n))
        return
    # c is an array of choices
    c = np.ones(t + 2, dtype=int).tolist()
    # init
    for i in range(1, t + 1):
        c[i - 1] = i - 1
    c[t] = n
    c[t + 1] = 0
    j = 0
    while j < t:
        yield c[0:-2]
        j = 0
        while (c[j] + 1) == c[j + 1]:
            c[j] = j
            j += 1
        c[j] += 1


def add_link(resp, rel, **kwargs):
    """create and add a protobuf link to a protobuff response"""
    link = resp.links.add(rel=rel, is_templated=False, ressource_name='journeys')
    for k, v in kwargs.items():
        if k is None or v is None:
            continue
        args = link.kwargs.add(key=k)
        if type(v) is list:
            args.values.extend(v)
        else:
            args.values.extend([v])


def _is_fake_car_section(section):
    """
    This function tests if the section is a fake car section
    """
    return (
        section.type == response_pb2.STREET_NETWORK or section.type == response_pb2.CROW_FLY
    ) and section.street_network.mode == response_pb2.Car


def switch_back_to_ridesharing(response, is_first_section):
    """
    :param response: a pb_response returned by kraken
    :param is_first_section: a bool indicates that if the first_section or last_section is a ridesharing section
                             True if the first_section is, False if the last_section is
    """
    for journey in response.journeys:
        if len(journey.sections) == 0:
            continue
        section_idx = 0 if is_first_section else -1
        section = journey.sections[section_idx]
        if _is_fake_car_section(section):
            section.street_network.mode = response_pb2.Ridesharing
            journey.durations.ridesharing += section.duration
            journey.durations.car -= section.duration
            journey.distances.ridesharing += section.length
            journey.distances.car -= section.length


def nCr(n, r):
    """
    Classic combination operator but accept r > n
    :param n: objects
    :param r: sample
    :return: answer
    >>> nCr(10, 11)
    1
    >>> nCr(5, 2)
    10
    """
    if n <= r:
        """
        We assume that it's valid when n <= r
        """
        return 1
    import math

    f = math.factorial
    return int(f(n) / f(r) / f(n - r))


def include_poi_access_points(request, pt_object, mode):
    return (
        request.get("_poi_access_points", False)
        and pt_object.embedded_type == type_pb2.POI
        and mode
        in [
            FallbackModes.walking.name,
            FallbackModes.bss.name,
        ]
        and pt_object.poi.children
    )


def get_impact_uris_for_poi(response, poi):
    impact_uris = set()
    if response is None:
        return impact_uris
    for impact in response.impacts:
        for object in impact.impacted_objects:
            if object.pt_object.embedded_type == type_pb2.POI and object.pt_object.uri == poi.uri:
                impact_uris.add(impact.uri)
                object.pt_object.poi.CopyFrom(poi)

    return impact_uris


def fill_disruptions_on_pois(instance, response):
    if not response.pois:
        return

    # add all poi_ids as parameters
    poi_uris = set()
    for poi in response.pois:
        poi_uris.add(poi.uri)

    # calling loki with api poi_disruptions
    resp_poi = get_disruptions_on_poi(instance, poi_uris)

    # For each poi in the response add impact_uris from resp_poi
    # and copy object poi in impact.impacted_objects
    for poi in response.pois:
        impact_uris = get_impact_uris_for_poi(resp_poi, poi)
        for impact_uri in impact_uris:
            poi.impact_uris.append(impact_uri)

    # Add all impacts from resp_poi to the response
    add_disruptions(response, resp_poi)


def fill_disruptions_on_places_nearby(instance, response):
    if not response.places_nearby:
        return

    # Add all the poi uris in a list
    poi_uris = set()
    for place_nearby in response.places_nearby:
        if place_nearby.embedded_type == type_pb2.POI:
            poi_uris.add(place_nearby.uri)

    # calling loki with api poi_disruptions
    resp_poi = get_disruptions_on_poi(instance, poi_uris)

    # For each poi in the response add impact_uris from resp_poi
    # and copy object poi in impact.impacted_objects
    for place_nearby in response.places_nearby:
        if place_nearby.embedded_type == type_pb2.POI:
            impact_uris = get_impact_uris_for_poi(resp_poi, place_nearby.poi)
            for impact_uri in impact_uris:
                place_nearby.poi.impact_uris.append(impact_uri)

    # Add all impacts from resp_poi to the response
    add_disruptions(response, resp_poi)


def get_disruptions_on_poi(instance, uris, since_datetime=None, until_datetime=None):
    if not uris:
        return None
    try:
        pt_planner = instance.get_pt_planner("loki")
        req = request_pb2.Request()
        req.requested_api = type_pb2.poi_disruptions

        req.poi_disruptions.pois.extend(uris)
        if since_datetime:
            req.poi_disruptions.since_datetime = since_datetime
        if until_datetime:
            req.poi_disruptions.until_datetime = until_datetime

        # calling loki with api api_disruptions
        resp_poi = pt_planner.send_and_receive(req)
    except Exception:
        return None
    return resp_poi


def add_disruptions(pb_resp, pb_disruptions):
    if pb_disruptions is None:
        return
    pb_resp.impacts.extend(pb_disruptions.impacts)


def update_booking_rule_url_in_section(section):
    if section.type != response_pb2.ON_DEMAND_TRANSPORT:
        return

    booking_url = section.booking_rule.booking_url
    if not booking_url:
        return

    departure_datetime = section.begin_date_time
    from_name = section.origin.stop_point.name
    from_coord_lat = section.origin.stop_point.coord.lat
    from_coord_lon = section.origin.stop_point.coord.lon
    to_name = section.destination.stop_point.name
    to_coord_lat = section.destination.stop_point.coord.lat
    to_coord_lon = section.destination.stop_point.coord.lon

    # Get all placeholders present in booking_url and match with predefined placeholder variables. value of those
    # present in booking_url but absent in predefined placeholder variables will be replaced by N/A
    placeholders = re.findall(r"{(\w+)}", booking_url)

    placeholder_dict = defaultdict(lambda: 'N/A')
    fmtr = Formatter()

    for p in placeholders:
        if p == "departure_datetime":
            placeholder_dict[p] = departure_datetime
        elif p == "from_name":
            placeholder_dict[p] = from_name
        elif p == "from_coord_lat":
            placeholder_dict[p] = from_coord_lat
        elif p == "from_coord_lon":
            placeholder_dict[p] = from_coord_lon
        elif p == "to_name":
            placeholder_dict[p] = to_name
        elif p == "to_coord_lat":
            placeholder_dict[p] = to_coord_lat
        elif p == "to_coord_lon":
            placeholder_dict[p] = to_coord_lon

    section.booking_rule.booking_url = requests.utils.requote_uri(fmtr.vformat(booking_url, (), placeholder_dict))
