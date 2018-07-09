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
import logging
import itertools
import datetime
import abc
import six
from jormungandr.scenarios.utils import compare, get_or_default
from navitiacommon import response_pb2
from jormungandr.utils import pb_del_if, ComposedFilter, portable_min


def delete_journeys(responses, request):

    if request.get('debug', False):
        return

    nb_deleted = 0
    for r in responses:
        nb_deleted += pb_del_if(r.journeys, lambda j: to_be_deleted(j))

    if nb_deleted:
        logging.getLogger(__name__).info('filtering {} journeys'.format(nb_deleted))


def to_be_deleted(journey):
    return 'to_delete' in journey.tags


def mark_as_dead(journey, is_debug, *reasons):
    journey.tags.append('to_delete')
    if is_debug:
        journey.tags.extend(('deleted_because_' + reason for reason in reasons))


@six.add_metaclass(abc.ABCMeta)
class SingleJourneyFilter(object):
    """
    Interface to implement for filters applied to a single journey (no comparison)
    """

    @abc.abstractmethod
    def filter_func(self, journey):
        """
        :return: True if the journey is valid, False otherwise
        """
        pass

    @abc.abstractproperty
    def message(self):
        """
        :attribute: a one-word, snake_case unicode to be used to explain why a journey is filtered
        """
        pass


def filter_wrapper(filter_obj=None, is_debug=False):
    """
    Wraps a SingleJourneyFilter instance to automatically deal with debug-mode, logging and tagging

    If filtered, the journey is tagged 'to_delete' (regardless of the debug-mode)
    In debug-mode, we deactivate filtering, only add a tag with the reason why it's deleted

    The main purpose of debug mode is to have all journeys generated (even the ones that should be filtered)
    and log ALL the reasons why they are filtered (if they are)

    :param filter_obj: a SingleJourneyFilter to be wrapped (using its message and filter_func attributes)
    :param is_debug: True if we are in debug-mode
    :return: a function to be called on a journey, returning True or False,tagging it if it's deleted
    """
    filter_func = filter_obj.filter_func
    message = filter_obj.message

    def wrapped_filter(journey):
        logger = logging.getLogger(__name__)
        res = filter_func(journey)
        if not res:
            logger.debug("We delete: {}, cause: {}".format(journey.internal_id, message))
            mark_as_dead(journey, is_debug, message)
        return res or is_debug

    return wrapped_filter


def filter_journeys(responses, instance, request):
    """
    Filter by side effect the list of pb responses's journeys

    """
    is_debug = request.get('debug', False)

    #DEBUG
    if is_debug:
        logger = logging.getLogger(__name__)
        logger.debug("All journeys:")
        [_debug_journey(j) for j in get_all_journeys(responses)]
        logger.debug("Qualified journeys:")
        [_debug_journey(j) for j in get_qualified_journeys(responses)]

    # build filters
    min_bike = request.get('_min_bike', None)
    min_car = request.get('_min_car', None)
    orig_modes = request.get('origin_mode', [])
    dest_modes = request.get('destination_mode', [])
    min_nb_transfers = request.get('min_nb_transfers', 0)

    filters = [FilterTooShortHeavyJourneys(min_bike=min_bike, min_car=min_car,
                                           orig_modes=orig_modes, dest_modes=dest_modes),
               FilterTooLongWaiting(),
               FilterMinTransfers(min_nb_transfers=min_nb_transfers)]

    # we add more filters in some special cases

    max_successive = request.get('_max_successive_physical_mode', 0)
    if max_successive != 0:
        limited_mode_id = instance.successive_physical_mode_to_limit_id # typically : physical_mode:bus
        filters.append(FilterMaxSuccessivePhysicalMode(successive_physical_mode_to_limit_id=limited_mode_id,
                                                       max_successive_physical_mode=max_successive))

    dp = request.get('direct_path', 'indifferent')
    if dp != 'indifferent':
        filters.append(FilterDirectPath(dp=dp))


    # compose filters

    composed_filter = ComposedFilter()
    for f in filters:
        composed_filter.add_filter(filter_wrapper(is_debug=is_debug, filter_obj=f))

    journey_generator = get_qualified_journeys
    if is_debug:
        journey_generator = get_all_journeys

    return composed_filter.compose_filters()(journey_generator(responses))


class FilterTooShortHeavyJourneys(SingleJourneyFilter):

    message = 'too_short_heavy_mode_fallback'

    def __init__(self, min_bike=None, min_car=None, orig_modes=None, dest_modes=None):
        self.min_bike = min_bike
        self.min_car = min_car
        self.orig_modes = [] if orig_modes is None else orig_modes
        self.dest_modes = [] if dest_modes is None else dest_modes

    def filter_func(self, journey):
        """
        We filter the journeys that use an "heavy" mode as fallback for a short time.
        Typically you don't take your car for only 2 minutes.
        Heavy fallback modes are Bike and Car, BSS is not considered as one.
        """

        def _exceed_min_duration(current_section, journey, min_duration, orig_modes=None, dest_modes=None):
            orig_modes=[] if orig_modes is None else orig_modes
            dest_modes=[] if dest_modes is None else dest_modes

            if current_section == journey.sections[0]:
                return 'walking' in orig_modes and current_section.duration < min_duration
            return 'walking' in dest_modes and current_section.duration < min_duration

        on_bss = False
        for s in journey.sections:
            if s.type == response_pb2.BSS_RENT:
                on_bss = True
            elif s.type == response_pb2.BSS_PUT_BACK:
                on_bss = False
            elif s.type != response_pb2.STREET_NETWORK:
                continue

            if s.street_network.mode == response_pb2.Car \
                    and self.min_car is not None \
                    and _exceed_min_duration(s, journey, min_duration=self.min_car,
                                             orig_modes=self.orig_modes, dest_modes=self.dest_modes):
                return False
            if not on_bss \
                    and s.street_network.mode == response_pb2.Bike \
                    and self.min_bike is not None \
                    and _exceed_min_duration(s, journey, min_duration=self.min_bike,
                                             orig_modes=self.orig_modes, dest_modes=self.dest_modes):
                return False
        return True


class FilterTooLongWaiting(SingleJourneyFilter):

    message = 'too_long_waiting'

    def filter_func(self, journey):
        """
        filter journeys with a too long section of type waiting
        """

        # early returns

        # if there is no transfer it won't have any waiting sections
        if journey.nb_transfers == 0:
            return True
        # if the total duration is smaller that the max_waiting, no need to check
        max_waiting = 4 * 60 * 60
        if journey.duration < max_waiting:
            return True

        for s in journey.sections:
            if s.type != response_pb2.WAITING:
                continue
            if s.duration < max_waiting:
                continue
            return False
        return True


class FilterMaxSuccessivePhysicalMode(SingleJourneyFilter):

    message = 'too_much_successive_section_same_physical_mode'

    def __init__(self,
                 successive_physical_mode_to_limit_id='physical_mode:Bus',
                 max_successive_physical_mode=3):
        self.successive_physical_mode_to_limit_id = successive_physical_mode_to_limit_id
        self.max_successive_physical_mode = max_successive_physical_mode

    def filter_func(self, journey):
        """
        eliminates journeys with specified public_transport.physical_mode more than
        _max_successive_physical_mode (used for STIF buses)
        """
        bus_count = 0
        for s in journey.sections:
            if s.type != response_pb2.PUBLIC_TRANSPORT:
                continue
            if s.pt_display_informations.uris.physical_mode == self.successive_physical_mode_to_limit_id:
                bus_count += 1
            else:
                if bus_count <= self.max_successive_physical_mode:
                    bus_count = 0

        if bus_count > self.max_successive_physical_mode:
            return False

        return True


class FilterMinTransfers(SingleJourneyFilter):

    message = 'not_enough_connections'

    def __init__(self, min_nb_transfers=0):
        self.min_nb_transfers = min_nb_transfers

    def filter_func(self, journey):
        """
        eliminates journeys with number of connections less than min_nb_transfers among journeys
        """
        if get_nb_connections(journey) < self.min_nb_transfers:
            return False
        return True


class FilterDirectPath(SingleJourneyFilter):

    message = 'direct_path_parameter'

    def __init__(self, dp='indifferent'):
        self.dp = dp

    def filter_func(self, journey):
        """
        eliminates journeys that are not matching direct path parameter (none, only or indifferent)
        """
        if self.dp == 'none' and 'non_pt' in journey.tags:
            return False
        elif self.dp == 'only' and 'non_pt' not in journey.tags:
            return False
        return True


def get_min_connections(journeys):
    """
    Returns min connection count among journeys
    Returns None if journeys empty
    """
    if not journeys:
        return None

    return portable_min((get_nb_connections(j) for j in journeys if not to_be_deleted(j)), default=0)


def get_nb_connections(journey):
    """
    Returns connections count in a journey
    """
    return journey.nb_transfers


def get_min_waiting(journey):
    """
    Returns min waiting time in a journey
    """
    return portable_min((s.duration for s in journey.sections if s.type == response_pb2.WAITING), default=0)


def is_walk_after_parking(journey, idx_section):
    """
    True if section at given index is a walking after/before parking car/bss, False otherwise
    """
    is_park_section = lambda section: section.type in {response_pb2.PARK,
                                                       response_pb2.LEAVE_PARKING,
                                                       response_pb2.BSS_PUT_BACK,
                                                       response_pb2.BSS_RENT}

    s = journey.sections[idx_section]
    if s.type == response_pb2.STREET_NETWORK and s.street_network.mode == response_pb2.Walking and \
        ((idx_section - 1 >= 0 and is_park_section(journey.sections[idx_section - 1])) or
         (idx_section + 1 < len(journey.sections) and is_park_section(journey.sections[idx_section + 1]))):
        return True
    return False


def similar_journeys_vj_generator(journey):
    for idx, s in enumerate(journey.sections):
        # special case, we don't want to consider the walking section after/before parking a car
        # so CAR / PARK / WALK / PT is equivalent to CAR / PARK / PT
        if is_walk_after_parking(journey, idx):
            continue

        if s.type == response_pb2.PUBLIC_TRANSPORT:
            yield "pt:{}".format(s.pt_display_informations.uris.vehicle_journey)
        elif s.type == response_pb2.STREET_NETWORK:
            yield "sn:{}".format(s.street_network.mode)


def similar_journeys_line_generator(journey):
    for idx, s in enumerate(journey.sections):
        # special case, we don't want to consider the walking section after/before parking a car
        # so CAR / PARK / WALK / PT is equivalent to CAR / PARK / PT
        if is_walk_after_parking(journey, idx):
            continue

        if s.type == response_pb2.PUBLIC_TRANSPORT:
            yield "pt:{}".format(s.pt_display_informations.uris.line)
        elif s.type == response_pb2.STREET_NETWORK:
            yield "sn:{}".format(s.street_network.mode)


def shared_section_generator(journey):
    """
    Definition of journeys with a shared section:
    - same stop point of departure and arrival
    - same number of sections in the journey
    """

    # Early return: test if the journeys have the same number of sections
    yield len(journey.sections)

    # Compare each section of the journey with the criteria in the function description
    for s in journey.sections:
        if s.type == response_pb2.PUBLIC_TRANSPORT:
            yield "origin:{}/dest:{}".format(s.origin.uri, s.destination.uri)


def fallback_duration(journey):
    duration = 0
    for section in journey.sections:
        if section.type in (response_pb2.STREET_NETWORK, response_pb2.CROW_FLY):
            duration += section.duration

    return duration


def _debug_journey(journey):
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
    for s in journey.sections:
        if s.type == response_pb2.PUBLIC_TRANSPORT:
            sections.append(u"{line} ({vj})".format(line=s.pt_display_informations.uris.line,
                                                    vj=s.pt_display_informations.uris.vehicle_journey))
        elif s.type == response_pb2.STREET_NETWORK:
            sections.append(shorten(response_pb2.StreetNetworkMode.Name(s.street_network.mode)))
        else:
            sections.append(shorten(response_pb2.SectionType.Name(s.type)))

    def _datetime_to_str(ts):
        #DEBUG, no tz handling
        dt = datetime.datetime.utcfromtimestamp(ts)
        return dt.strftime("%dT%H:%M:%S")

    logger.debug(u"journey {id}: {dep} -> {arr} - {duration} ({fallback} map) | ({sec})".format(
        id=journey.internal_id,
        dep=_datetime_to_str(journey.departure_date_time),
        arr=_datetime_to_str(journey.arrival_date_time),
        duration=datetime.timedelta(seconds=journey.duration),
        fallback=datetime.timedelta(seconds=fallback_duration(journey)),
        sec=" - ".join(sections)))


def get_qualified_journeys(responses):
    """
    :param responses: protobuf
    :return: generator of journeys
    """
    return (j for r in responses for j in r.journeys if not to_be_deleted(j))


def get_all_journeys(responses):
    """
    :param responses: protobuf
    :return: generator of journeys
    """
    return (j for r in responses for j in r.journeys)


def apply_final_journey_filters(response_list, instance, request):
    """
    Final pass: Filter by side effect the list of pb responses's journeys

    Nota: All filters below are applied only once, after all calls to kraken are done
    """
    is_debug = request.get('debug', False)
    journey_generator = get_qualified_journeys
    if is_debug:
        journey_generator = get_all_journeys

    # we remove similar journeys (same lines and same succession of stop_points)
    final_line_filter = get_or_default(request, '_final_line_filter', False)
    if final_line_filter:
        journeys = journey_generator(response_list)
        journey_pairs_pool = itertools.combinations(journeys, 2)
        _filter_similar_line_journeys(journey_pairs_pool, request)

    # we filter journeys having "shared sections" (same succession of stop_points + custom rules)
    no_shared_section = get_or_default(request, 'no_shared_section', False)
    if no_shared_section:
        journeys = journey_generator(response_list)
        journey_pairs_pool = itertools.combinations(journeys, 2)
        filter_shared_sections_journeys(journey_pairs_pool, request)

    # we filter journeys having too much connections compared to minimum
    journeys = journey_generator(response_list)
    _filter_too_much_connections(journeys, instance, request)


def _get_worst_similar(j1, j2, request):
    """
    Decide which is the worst journey between 2 similar journeys.

    The choice is made on:
     - asap
     - duration
     - more fallback
     - more connection
     - smaller value among min waiting duration
     - more constrained fallback mode : all else being equal,
            it's better to know that you can do it by bike for 'bike_in_pt' tag
            (and traveler presumes he can do it walking too, as the practical case is 0s fallback)
    """
    if request.get('clockwise', True):
        if j1.arrival_date_time != j2.arrival_date_time:
            return j1 if j1.arrival_date_time > j2.arrival_date_time else j2
    else:
        if j1.departure_date_time != j2.departure_date_time:
            return j1 if j1.departure_date_time < j2.departure_date_time else j2

    if j1.duration != j2.duration:
        return j1 if j1.duration > j2.duration else j2

    if fallback_duration(j1) != fallback_duration(j2):
        return j1 if fallback_duration(j1) > fallback_duration(j2) else j2

    if get_nb_connections(j1) != get_nb_connections(j2):
        return j1 if get_nb_connections(j1) > get_nb_connections(j2) else j2

    if get_min_waiting(j1) != get_min_waiting(j2):
        return j1 if get_min_waiting(j1) < get_min_waiting(j2) else j2


    def get_mode_rank(section):
        mode_rank = {response_pb2.Car: 0,
                     response_pb2.Bike: 1,
                     response_pb2.Walking: 2}
        return mode_rank.get(section.street_network.mode)

    def is_fallback(section):
        return section.type == response_pb2.CROW_FLY or section.type != response_pb2.STREET_NETWORK

    s1 = j1.sections[0]
    s2 = j2.sections[0]
    if is_fallback(s1) and is_fallback(s2) and s1.street_network.mode != s2.street_network.mode:
        return j1 if get_mode_rank(s1) > get_mode_rank(s2) else j2

    s1 = j1.sections[-1]
    s2 = j2.sections[-1]
    if is_fallback(s1) and is_fallback(s2) and s1.street_network.mode != s2.street_network.mode:
        return j1 if get_mode_rank(s1) > get_mode_rank(s2) else j2

    return j2


def filter_similar_vj_journeys(journey_pairs_pool, request):
    _filter_similar_journeys(journey_pairs_pool, request, similar_journeys_vj_generator)


def _filter_similar_line_journeys(journey_pairs_pool, request):
    _filter_similar_journeys(journey_pairs_pool, request, similar_journeys_line_generator)


def filter_shared_sections_journeys(journey_pairs_pool, request):
    _filter_similar_journeys(journey_pairs_pool, request, shared_section_generator)


def _filter_similar_journeys(journey_pairs_pool, request, similar_journey_generator):
    """
    Compare journeys 2 by 2.
    The given generator tells which part of journeys are compared.
    In case of similar journeys, the function '_get_worst_similar_vjs' decides which one to delete.
    """

    logger = logging.getLogger(__name__)
    is_debug = request.get('debug', False)
    for j1, j2 in journey_pairs_pool:
        if to_be_deleted(j1) or to_be_deleted(j2):
            continue
        if compare(j1, j2, similar_journey_generator):
            # After comparison, if the 2 journeys are similar, the worst one must be eliminated
            worst = _get_worst_similar(j1, j2, request)
            logger.debug("the journeys {}, {} are similar, we delete {}".format(j1.internal_id,
                                                                                j2.internal_id,
                                                                                worst.internal_id))

            mark_as_dead(worst, is_debug, 'duplicate_journey', 'similar_to_{other}'
                          .format(other=j1.internal_id if worst == j2 else j2.internal_id))


def _filter_too_much_connections(journeys, instance, request):
    """
    eliminates journeys with a number of connections strictly superior to the
    minimum number of connections among all journeys + _max_additional_connections
    """
    logger = logging.getLogger(__name__)
    max_additional_connections = get_or_default(request, '_max_additional_connections',
                                                instance.max_additional_connections)
    import itertools
    it1, it2 = itertools.tee(journeys, 2)
    min_connections = get_min_connections(it1)
    is_debug = request.get('debug', False)
    if min_connections is not None:
        max_connections_allowed = max_additional_connections + min_connections
        for j in it2:
            if get_nb_connections(j) > max_connections_allowed:
                logger.debug("the journey {} has a too much connections, we delete it".format(j.internal_id))
                mark_as_dead(j, is_debug, "too_much_connections")
