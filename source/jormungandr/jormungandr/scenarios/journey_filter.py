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
import logging
import itertools
import datetime
import abc
import six
from jormungandr.scenarios.utils import compare, get_or_default
from navitiacommon import response_pb2
from jormungandr.utils import pb_del_if, ComposedFilter, portable_min
from jormungandr.fallback_modes import FallbackModes
from jormungandr.scenarios.qualifier import get_ASAP_journey


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

    # DEBUG
    if is_debug:
        logger = logging.getLogger(__name__)
        logger.debug("All journeys:")
        [_debug_journey(j) for j in get_all_journeys(responses)]
        logger.debug("Qualified journeys:")
        [_debug_journey(j) for j in get_qualified_journeys(responses)]

    # build filters
    min_bike = request.get('_min_bike', None)
    min_car = request.get('_min_car', None)
    min_taxi = request.get('_min_taxi', None)
    min_ridesharing = request.get('_min_ridesharing', None)
    orig_modes = request.get('origin_mode', [])
    dest_modes = request.get('destination_mode', [])
    min_nb_transfers = request.get('min_nb_transfers', 0)
    max_waiting_duration = request.get('max_waiting_duration')
    if not max_waiting_duration:
        max_waiting_duration = instance.max_waiting_duration

    filters = [
        FilterTooShortHeavyJourneys(
            min_bike=min_bike, min_car=min_car, min_taxi=min_taxi, min_ridesharing=min_ridesharing
        ),
        FilterTooLongWaiting(max_waiting_duration=max_waiting_duration),
        FilterMinTransfers(min_nb_transfers=min_nb_transfers),
    ]

    # TODO: we should handle this better....
    if (request.get('_override_scenario') or instance._scenario_name) == 'distributed':
        filters.append(FilterTooLongDirectPath(instance=instance, request=request))

    # we add more filters in some special cases

    max_successive = request.get('_max_successive_physical_mode', 0)
    if max_successive != 0:
        limited_mode_id = instance.successive_physical_mode_to_limit_id  # typically : physical_mode:bus
        filters.append(
            FilterMaxSuccessivePhysicalMode(
                successive_physical_mode_to_limit_id=limited_mode_id, max_successive_physical_mode=max_successive
            )
        )

    dp = request.get('direct_path', 'indifferent')
    if dp != 'indifferent':
        filters.append(FilterDirectPath(dp=dp))

    dp_mode = request.get('direct_path_mode', [])
    if dp_mode:
        filters.append(FilterDirectPathMode(dp_mode))

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

    def __init__(self, min_bike=None, min_car=None, min_taxi=None, min_ridesharing=None):
        self.min_bike = min_bike
        self.min_car = min_car
        self.min_taxi = min_taxi
        self.min_ridesharing = min_ridesharing

    def filter_func(self, journey):
        """
        We filter the journeys that use an "heavy" mode as fallback for a short time.
        Typically you don't take your car for only 2 minutes.
        Heavy fallback modes are Bike and Car, BSS is not considered as one.
        We also filter too short direct path except for Bike
        """

        def _exceed_min_duration(min_duration, total_duration):
            return total_duration < min_duration

        def _is_bike_direct_path(journey):
            return len(journey.sections) == 1 and journey.sections[0].street_network.mode == response_pb2.Bike

        # We do not filter direct_path bike journeys
        if _is_bike_direct_path(journey=journey):
            return True

        on_bss = False
        for s in journey.sections:
            if s.type == response_pb2.BSS_RENT:
                on_bss = True
            elif s.type == response_pb2.BSS_PUT_BACK:
                on_bss = False
            elif s.type != response_pb2.STREET_NETWORK:
                continue

            min_mode = None
            total_duration = 0
            if s.street_network.mode == response_pb2.Car:
                min_mode = self.min_car
                total_duration = journey.durations.car
            elif s.street_network.mode == response_pb2.Taxi:
                min_mode = self.min_taxi
                total_duration = journey.durations.taxi
            elif s.street_network.mode == response_pb2.Ridesharing:
                min_mode = self.min_ridesharing
                total_duration = journey.durations.ridesharing
            elif s.street_network.mode == response_pb2.Bike:
                min_mode = self.min_bike
                total_duration = journey.durations.bike
            else:
                continue

            if (
                not on_bss
                and min_mode is not None
                and _exceed_min_duration(min_duration=min_mode, total_duration=total_duration)
            ):
                return False

        return True


class FilterTooLongWaiting(SingleJourneyFilter):

    message = 'too_long_waiting'

    def __init__(self, max_waiting_duration=4 * 60 * 60):
        self.max_waiting_duration = max_waiting_duration

    def filter_func(self, journey):
        """
        filter journeys with a too long section of type waiting
        """

        # early returns

        # if there is no transfer it won't have any waiting sections
        if journey.nb_transfers == 0:
            return True

        if journey.duration < self.max_waiting_duration:
            return True

        for s in journey.sections:
            if s.type != response_pb2.WAITING:
                continue
            if s.duration < self.max_waiting_duration:
                continue
            return False
        return True


class FilterMaxSuccessivePhysicalMode(SingleJourneyFilter):

    message = 'too_much_successive_section_same_physical_mode'

    def __init__(self, successive_physical_mode_to_limit_id='physical_mode:Bus', max_successive_physical_mode=3):
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
        eliminates journeys that are not matching direct path parameter (none, only, only_with_alternatives or indifferent)
        """
        if self.dp == 'none' and 'non_pt' in journey.tags:
            return False
        elif self.dp in ('only', 'only_with_alternatives') and 'non_pt' not in journey.tags:
            return False
        return True


class FilterDirectPathMode(SingleJourneyFilter):

    message = 'direct_path_mode_parameter'

    def __init__(self, dp_mode):
        self.dp_mode = [] if dp_mode is None else dp_mode

    def filter_func(self, journey):
        """
        eliminates journeys that are not matching direct path mode parameter
        """

        is_dp = 'non_pt' in journey.tags
        is_in_direct_path_mode_list = any(mode in self.dp_mode for mode in journey.tags)

        # if direct_path of a mode which is not in direct_path_mode[]
        if is_dp and not is_in_direct_path_mode_list:
            return False

        return True


class FilterTooLongDirectPath(SingleJourneyFilter):

    message = 'too_long_direct_path'

    def __init__(self, instance, request):
        self.instance = instance
        self.request = request
        self.logger = logging.getLogger(__name__)

    def _get_mode_of_journey(self, journey):
        mode = FallbackModes.modes_str() & set(journey.tags)
        if len(mode) != 1:
            self.logger.error('Cannot determine the mode of direct path: {}'.format(mode))
            return None

        return next(iter(mode))

    def filter_func(self, journey):
        """
        eliminates too long direct_path journey
        """
        # we filter only direct path
        if 'non_pt' not in journey.tags:
            return True

        direct_path_mode = self._get_mode_of_journey(journey)

        attr_name = 'max_{}_direct_path_duration'.format(direct_path_mode)
        max_duration = self.request[attr_name]
        return max_duration > journey.duration


def get_best_pt_journey_connections(journeys, request):
    """
    Returns the nb of connection of the best pt_journey
    Returns None if journeys empty
    """
    if not journeys:
        return None
    best = get_ASAP_journey((j for j in journeys if 'non_pt' not in j.tags), request)
    return get_nb_connections(best) if best else None


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
    is_park_section = lambda section: section.type in {
        response_pb2.PARK,
        response_pb2.LEAVE_PARKING,
        response_pb2.BSS_PUT_BACK,
        response_pb2.BSS_RENT,
    }

    s = journey.sections[idx_section]
    if (
        s.type == response_pb2.STREET_NETWORK
        and s.street_network.mode == response_pb2.Walking
        and (
            (idx_section - 1 >= 0 and is_park_section(journey.sections[idx_section - 1]))
            or (idx_section + 1 < len(journey.sections) and is_park_section(journey.sections[idx_section + 1]))
        )
    ):
        return True
    return False


def similar_pt_section_vj(section):
    return 'pt:%s' % section.pt_display_informations.uris.vehicle_journey


def similar_pt_section_line(section):
    return "pt:{}".format(section.pt_display_informations.uris.line)


def _sn_functor(s):
    return 'sn:{}'.format(s.street_network.mode)


def _crow_fly_sn_functor(_s):
    return "crow_fly"


def similar_journeys_generator(journey, pt_functor, sn_functor=_sn_functor, crow_fly_functor=_sn_functor):
    def _similar_non_pt():
        for s in journey.sections:
            yield "sn:{} type:{} tags:{}".format(s.street_network.mode, s.type, journey.tags)

    def _similar_pt():
        for idx, s in enumerate(journey.sections):
            if s.type == response_pb2.PUBLIC_TRANSPORT:
                yield pt_functor(s)
            elif s.type == response_pb2.STREET_NETWORK and is_walk_after_parking(journey, idx):
                continue
            elif s.type is response_pb2.STREET_NETWORK:
                yield sn_functor(s)
            elif s.type is response_pb2.CROW_FLY:
                yield crow_fly_functor(s)

    if 'non_pt' in journey.tags:
        return _similar_non_pt()

    return _similar_pt()


def detailed_pt_section_vj(section):
    return 'pt:{vj} dep:{dep} arr:{arr}'.format(
        vj=section.pt_display_informations.uris.vehicle_journey,
        dep=section.begin_date_time,
        arr=section.end_date_time,
    )


def bss_walking_sn_functor(s):
    mode = s.street_network.mode
    return "d: {} m: {}".format(
        s.duration,
        FallbackModes.walking.value if mode in (FallbackModes.walking.value, FallbackModes.bss.value) else mode,
    )


def similar_bss_walking_vj_generator(journey):
    for v in similar_journeys_generator(journey, detailed_pt_section_vj, bss_walking_sn_functor):
        yield v


def similar_journeys_vj_generator(journey):
    for v in similar_journeys_generator(journey, similar_pt_section_vj):
        yield v


def similar_journeys_line_generator(journey):
    for v in similar_journeys_generator(journey, similar_pt_section_line):
        yield v


def similar_journeys_line_and_crowfly_generator(journey):
    for v in similar_journeys_generator(journey, similar_pt_section_line, crow_fly_functor=_crow_fly_sn_functor):
        yield v


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


def end_fallback_duration(journey):
    for section in reversed(journey.sections):
        if section.type in (response_pb2.STREET_NETWORK, response_pb2.CROW_FLY):
            return section.duration

    return 0


def start_fallback_duration(journey):
    for section in journey.sections:
        if section.type in (response_pb2.STREET_NETWORK, response_pb2.CROW_FLY):
            return section.duration

    return 0


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
            sections.append(
                u"{line} ({vj})".format(
                    line=s.pt_display_informations.uris.line, vj=s.pt_display_informations.uris.vehicle_journey
                )
            )
        elif s.type == response_pb2.STREET_NETWORK:
            sections.append(shorten(response_pb2.StreetNetworkMode.Name(s.street_network.mode)))
        else:
            sections.append(shorten(response_pb2.SectionType.Name(s.type)))

    def _datetime_to_str(ts):
        # DEBUG, no tz handling
        dt = datetime.datetime.utcfromtimestamp(ts)
        return dt.strftime("%dT%H:%M:%S")

    logger.debug(
        u"journey {id}: {dep} -> {arr} - {duration} ({fallback} map) | ({sec})".format(
            id=journey.internal_id,
            dep=_datetime_to_str(journey.departure_date_time),
            arr=_datetime_to_str(journey.arrival_date_time),
            duration=datetime.timedelta(seconds=journey.duration),
            fallback=datetime.timedelta(seconds=fallback_duration(journey)),
            sec=" - ".join(sections),
        )
    )


def get_qualified_journeys(responses):
    """
    :param responses: protobuf
    :return: generator of journeys
    """
    return (j for r in responses if r is not None for j in r.journeys if not to_be_deleted(j))


def nb_qualifed_journeys(responses):
    return sum(1 for j in get_qualified_journeys(responses))


def get_all_journeys(responses):
    """
    :param responses: protobuf
    :return: generator of journeys
    """
    return (j for r in responses if r is not None for j in r.journeys)


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

    filter_odt = get_or_default(request, '_filter_odt_journeys', False)
    if filter_odt:
        journeys = journey_generator(response_list)
        filter_odt_journeys(journeys, request)

    # we filter journeys having too much connections compared to minimum
    journeys = journey_generator(response_list)
    _filter_too_much_connections(journeys, instance, request)

    origin_mode = get_or_default(request, 'origin_mode', [])
    if origin_mode == ['car']:
        journeys = journey_generator(response_list)
        filter_non_car_tagged_journey(journeys, request)


def filter_non_car_tagged_journey(journeys, request):
    is_debug = request.get('debug', False)

    for j in journeys:
        if not j.tags or "car" in j.tags:
            continue
        mark_as_dead(
            j,
            is_debug,
            'non_car_tagged_journey_filtered',
        )


def apply_final_journey_filters_post_finalize(response_list, request):
    """
    Final pass: Filter by side effect the list of pb responses's journeys
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
        _filter_similar_line_and_crowfly_journeys(journey_pairs_pool, request)


def replace_bss_tag(journeys):
    """
    replace the bss tag by walking, if there's no bss section in the journey
    """
    for j in journeys:
        if not j.tags or "bss" not in j.tags:
            continue
        has_bss = any((s.type == response_pb2.BSS_RENT for s in j.sections))
        if has_bss:
            continue
        j.tags.remove("bss")
        j.tags.append("walking")


def filter_detailed_journeys(responses, request):
    journey_generator = get_qualified_journeys
    if request.get('debug', False):
        journey_generator = get_all_journeys

    journeys = journey_generator(responses)

    min_bike = request.get('_min_bike', None)
    min_car = request.get('_min_car', None)
    min_taxi = request.get('_min_taxi', None)
    min_ridesharing = request.get('_min_ridesharing', None)
    orig_modes = request.get('origin_mode', [])
    dest_modes = request.get('destination_mode', [])

    too_heavy_journey_filter = FilterTooShortHeavyJourneys(
        min_bike=min_bike, min_car=min_car, min_taxi=min_taxi, min_ridesharing=min_ridesharing
    )
    f_wrapped = filter_wrapper(is_debug=request.get('debug', False), filter_obj=too_heavy_journey_filter)

    [f_wrapped(j) for j in journeys]

    it1, it2 = itertools.tee(journey_generator(responses))
    journey_pairs_pool = itertools.product(it1, it2)
    filter_similar_vj_journeys(journey_pairs_pool, request)

    replace_bss_tag(journey_generator(responses))


def _get_worst_similar(j1, j2, request):
    """
    Decide which is the worst journey between 2 similar journeys.

    The choice is made on:
     - crow_fly fallback
     - asap
     - duration
     - more fallback
     - more connection
     - smaller value among min waiting duration
     - more constrained fallback mode : all else being equal,
            it's better to know that you can do it by bike for 'bike_in_pt' tag
            (and traveler presumes he can do it walking too, as the practical case is 0s fallback)
    """

    def get_mode_rank_crow_fly(section):
        mode_rank = {
            response_pb2.Car: 0,
            response_pb2.Taxi: 1,
            response_pb2.Bike: 2,
            response_pb2.Bss: 3,
            response_pb2.Walking: 4,
        }
        return mode_rank.get(section.street_network.mode)

    def is_fallback_crow_fly(section):
        return section.type == response_pb2.CROW_FLY

    for idx in [0, -1]:
        s1 = j1.sections[idx]
        s2 = j2.sections[idx]
        if (
            is_fallback_crow_fly(s1)
            and is_fallback_crow_fly(s2)
            and s1.street_network.mode != s2.street_network.mode
        ):
            return j1 if get_mode_rank_crow_fly(s1) < get_mode_rank_crow_fly(s2) else j2

    if request.get('clockwise', True):

        # we dont want to arrive a few seconds earlier if it means walking more
        # so we consider that arriving 1s earlier is better if we walk at most 1s more
        # hence we compare arrival_time + walking_time instead of just arrival time
        j1_penalized_arrival = j1.arrival_date_time + end_fallback_duration(j1)
        j2_penalized_arrival = j2.arrival_date_time + end_fallback_duration(j2)
        if j1_penalized_arrival != j2_penalized_arrival:
            return j1 if j1_penalized_arrival > j2_penalized_arrival else j2

        # arrival times are the same, let's look at departure times

        # we dont want to depart a few seconds later if it means walking more
        # so we consider that departing 1s later is better if we walk at most 1s more
        # hence we compare departure_time - walking_time instead of just departure time
        j1_penalized_departure = j1.departure_date_time - start_fallback_duration(j1)
        j2_penalized_departure = j2.departure_date_time - start_fallback_duration(j2)
        if j1_penalized_departure != j2_penalized_departure:
            return j1 if j1_penalized_departure < j2_penalized_departure else j2
    else:

        j1_penalized_departure = j1.departure_date_time - start_fallback_duration(j1)
        j2_penalized_departure = j2.departure_date_time - start_fallback_duration(j2)
        if j1_penalized_departure != j2_penalized_departure:
            return j1 if j1_penalized_departure < j2_penalized_departure else j2

        # departure times are the same, let's look at arrival times
        j1_penalized_arrival = j1.arrival_date_time + end_fallback_duration(j1)
        j2_penalized_arrival = j2.arrival_date_time + end_fallback_duration(j2)
        if j1_penalized_arrival != j2_penalized_arrival:
            return j1 if j1_penalized_arrival > j2_penalized_arrival else j2

    if j1.duration != j2.duration:
        return j1 if j1.duration > j2.duration else j2

    if fallback_duration(j1) != fallback_duration(j2):
        return j1 if fallback_duration(j1) > fallback_duration(j2) else j2

    if get_nb_connections(j1) != get_nb_connections(j2):
        return j1 if get_nb_connections(j1) > get_nb_connections(j2) else j2

    if get_min_waiting(j1) != get_min_waiting(j2):
        return j1 if get_min_waiting(j1) < get_min_waiting(j2) else j2

    def get_mode_rank(section):
        mode_rank = {response_pb2.Car: 0, response_pb2.Bike: 1, response_pb2.Walking: 3, response_pb2.Bss: 4}
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
    _filter_similar_journeys(
        journey_pairs_pool, request, similar_journeys_vj_generator, similar_bss_walking_vj_generator
    )


def _filter_similar_line_journeys(journey_pairs_pool, request):
    _filter_similar_journeys(journey_pairs_pool, request, similar_journeys_line_generator)


def _filter_similar_line_and_crowfly_journeys(journey_pairs_pool, request):
    _filter_similar_journeys(journey_pairs_pool, request, similar_journeys_line_and_crowfly_generator)


def filter_shared_sections_journeys(journey_pairs_pool, request):
    _filter_similar_journeys(journey_pairs_pool, request, shared_section_generator)


def _filter_similar_journeys(journey_pairs_pool, request, *similar_journey_generators):
    """
    Compare journeys 2 by 2.
    The given generator tells which part of journeys are compared.
    In case of similar journeys, the function '_get_worst_similar_vjs' decides which one to delete.
    """

    logger = logging.getLogger(__name__)
    is_debug = request.get('debug', False)
    for j1, j2 in journey_pairs_pool:
        if j1 is j2:
            continue

        if to_be_deleted(j1) or to_be_deleted(j2):
            continue

        if any(compare(j1, j2, generator) for generator in similar_journey_generators):
            # After comparison, if the 2 journeys are similar, the worst one must be eliminated
            worst = _get_worst_similar(j1, j2, request)
            logger.debug(
                "the journeys {}, {} are similar, we delete {}".format(
                    j1.internal_id, j2.internal_id, worst.internal_id
                )
            )

            mark_as_dead(
                worst,
                is_debug,
                'duplicate_journey',
                'similar_to_{other}'.format(other=j1.internal_id if worst == j2 else j2.internal_id),
            )


def filter_odt_journeys(journeys, request):
    clockwise = request.get('clockwise', True)
    debug = request.get('debug', False)
    journeys_list = [j for j in journeys]
    if clockwise:
        return _filter_odt_journeys_clockwise(journeys_list, debug)
    else:
        return _filter_odt_journeys_counter_clockwise(journeys_list, debug)


def _filter_odt_journeys_clockwise(journeys, debug):
    """
    eliminates a journey that uses On Demand Transport if there is a public transport journey
    that arrive earlier
    """
    # let's find the earliest arrival time among public transport journeys
    earliest_arrival_pt_journey = portable_min(
        (j for j in journeys if _contains_pt_section(j) and not _contains_odt(j)),
        key=lambda j: j.arrival_date_time,
    )

    # no pt journey found, so there is nothing to filter
    if earliest_arrival_pt_journey is None:
        return

    # let's mark as dead all odt journeys that arrives after earliest_arrival_pt_journey
    for journey in journeys:
        if _contains_odt(journey) and journey.arrival_date_time >= earliest_arrival_pt_journey.arrival_date_time:
            mark_as_dead(
                journey,
                debug,
                'odt_arrives_after_{other}'.format(other=earliest_arrival_pt_journey.internal_id),
            )


def _filter_odt_journeys_counter_clockwise(journeys, debug):
    """
    eliminates a journey that uses On Demand Transport if there is a public transport journey
    that depart later
    """
    # let's find the latest departure time among public transport journeys
    latest_departure_pt_journey = portable_min(
        (j for j in journeys if _contains_pt_section(j) and not _contains_odt(j)),
        key=lambda j: -1 * j.departure_date_time,
    )

    # no pt journey found, so there is nothing to filter
    if latest_departure_pt_journey is None:
        return

    # let's mark as dead all odt journeys that depart before latest_departure_pt_journey
    for journey in journeys:
        if (
            _contains_odt(journey)
            and journey.departure_date_time <= latest_departure_pt_journey.departure_date_time
        ):
            mark_as_dead(
                journey,
                debug,
                'odt_departs_before_{other}'.format(other=latest_departure_pt_journey.internal_id),
            )


def _contains_pt_section(journey):
    return any(section.type == response_pb2.PUBLIC_TRANSPORT for section in journey.sections)


def _contains_odt(journey):
    for section in journey.sections:
        for info in section.additional_informations:
            if info in (
                response_pb2.ODT_WITH_ZONE,
                response_pb2.ODT_WITH_STOP_POINT,
                response_pb2.ODT_WITH_STOP_TIME,
            ):
                return True
    return False


def _filter_too_much_connections(journeys, instance, request):
    """
    eliminates journeys with a number of connections strictly superior to the
    the number of connections of the best pt_journey + _max_additional_connections
    """
    logger = logging.getLogger(__name__)
    max_additional_connections = get_or_default(
        request, '_max_additional_connections', instance.max_additional_connections
    )
    import itertools

    it1, it2 = itertools.tee(journeys, 2)
    best_pt_journey_connections = get_best_pt_journey_connections(it1, request)
    is_debug = request.get('debug', False)
    if best_pt_journey_connections is not None:
        max_connections_allowed = max_additional_connections + best_pt_journey_connections
        for j in it2:
            if get_nb_connections(j) > max_connections_allowed:
                logger.debug("the journey {} has a too much connections, we delete it".format(j.internal_id))
                mark_as_dead(j, is_debug, "too_much_connections")


def remove_excess_tickets(response):
    """
    Remove excess tickets
    """
    logger = logging.getLogger(__name__)

    fare_ticket_id_list = set()
    for j in response.journeys:
        for t_id in j.fare.ticket_id:
            fare_ticket_id_list.add(t_id)
        # ridesharing case
        for s in j.sections:
            for rj in s.ridesharing_journeys:
                for t_id in rj.fare.ticket_id:
                    fare_ticket_id_list.add(t_id)

    for t in reversed(response.tickets):
        if not t.id in fare_ticket_id_list:
            logger.debug('remove excess ticket id %s', t.id)
            response.tickets.remove(t)


def remove_excess_terminus(response):
    logger = logging.getLogger(__name__)
    terminus_section_ids = set()
    for j in response.journeys:
        for section in j.sections:
            for type_, value in section.pt_display_informations.uris.ListFields():
                if type_.name == "stop_area":
                    terminus_section_ids.add(value)
    for terminus in reversed(response.terminus):
        if not terminus.uri in terminus_section_ids:
            logger.debug('remove excess terminus uri %s', terminus.uri)
            response.terminus.remove(terminus)
