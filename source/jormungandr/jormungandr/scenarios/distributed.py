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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, unicode_literals, division

try:
    from typing import Dict, Text, Any, Tuple
except ImportError:
    pass
import logging, operator
from jormungandr.scenarios import new_default
from jormungandr import app
from jormungandr.utils import PeriodExtremity, get_pt_object_coord
from jormungandr.street_network.street_network import StreetNetworkPathType
from jormungandr.scenarios.helper_classes import *
from jormungandr.scenarios.helper_classes.complete_pt_journey import (
    wait_and_get_pt_journeys,
    wait_and_build_crowflies,
    get_journeys_to_complete,
)
from jormungandr.street_network.utils import crowfly_distance_between
from jormungandr.scenarios.utils import (
    fill_uris,
    switch_back_to_ridesharing,
    updated_common_journey_request_with_default,
)
from jormungandr.new_relic import record_custom_parameter
from navitiacommon import type_pb2
from flask_restful import abort
from .helper_classes.helper_utils import timed_logger


class PartialResponseContext(object):
    requested_orig = None
    requested_dest = None
    requested_orig_obj = None
    requested_dest_obj = None
    streetnetwork_path_pool = None
    orig_fallback_durations_pool = None
    dest_fallback_durations_pool = None
    partial_response_is_empty = True
    orig_places_free_access = None
    dest_places_free_access = None
    # Map of journeys's internal id to fallback modes.
    journeys_to_modes = dict()  # type: Dict[Text, Tuple[Text, Text]]


class Distributed(object):
    @staticmethod
    def _map_journeys_to_modes(pt_journey_elements):
        # type: (Any) -> Dict[Text, Tuple[Text, Text]]
        """
        Map the journeys's internal id (for each pt_journey_elements) with its fallback modes

        :param pt_journey_elements: the journeys to map
        :return: a map: ID -> (departure mode, arrival mode)
        """
        return {
            pt_j.internal_id: (e.dep_mode, e.arr_mode)
            for e in pt_journey_elements
            for pt_j in e.pt_journeys.journeys
        }

    def _compute_journeys(self, future_manager, request, instance, krakens_call, context, request_type):
        """
        For all krakens_call, call the kraken and aggregate the responses

        Note: the responses will only attach a crowfly section as a fallback. Street network will be
        done when calling finalise_journeys

        return the list of all responses
        """

        logger = logging.getLogger(__name__)
        logger.debug('request datetime: %s', request['datetime'])
        request_id = request["request_id"]
        logger.debug("request_id : {}".format(request_id))

        requested_dep_modes_with_pt = {
            mode for mode, _, direct_path_type in krakens_call if direct_path_type != "only"
        }
        requested_arr_modes_with_pt = {
            mode for _, mode, direct_path_type in krakens_call if direct_path_type != "only"
        }

        # These are the modes in first_section_modes[] and direct_path_modes[]
        # We need to compute direct_paths for them either because we requested it with direct_path_modes[]
        # Or because we need them to optimize the pt_journey computation
        requested_direct_path_modes = {
            mode for mode, _, direct_path_type in krakens_call if direct_path_type == "only"
        }
        requested_direct_path_modes.update(requested_dep_modes_with_pt)

        if context.partial_response_is_empty:
            logger.debug('requesting places by uri orig: %s dest %s', request['origin'], request['destination'])

            context.requested_orig = PlaceByUri(
                future_manager=future_manager,
                instance=instance,
                uri=request['origin'],
                request_id="{}_place_origin".format(request_id),
            )
            context.requested_dest = PlaceByUri(
                future_manager=future_manager,
                instance=instance,
                uri=request['destination'],
                request_id="{}_place_dest".format(request_id),
            )

            context.requested_orig_obj = get_entry_point_or_raise(context.requested_orig, request['origin'])
            context.requested_dest_obj = get_entry_point_or_raise(context.requested_dest, request['destination'])

            context.streetnetwork_path_pool = StreetNetworkPathPool(
                future_manager=future_manager, instance=instance
            )

            period_extremity = PeriodExtremity(request['datetime'], request['clockwise'])

            for mode in requested_direct_path_modes:
                context.streetnetwork_path_pool.add_async_request(
                    requested_orig_obj=context.requested_orig_obj,
                    requested_dest_obj=context.requested_dest_obj,
                    mode=mode,
                    period_extremity=period_extremity,
                    request=request,
                    streetnetwork_path_type=StreetNetworkPathType.DIRECT,
                    request_id="{}_direct_path_mode_{}".format(request_id, mode),
                )

            # if public transport is not requested, there is no need to continue,
            # we return all direct path without pt
            if request['max_duration'] == 0 or request.get('direct_path', "") == "only":
                res = [
                    context.streetnetwork_path_pool.wait_and_get(
                        requested_orig_obj=context.requested_orig_obj,
                        requested_dest_obj=context.requested_dest_obj,
                        mode=mode,
                        request=request,
                        period_extremity=period_extremity,
                        streetnetwork_path_type=StreetNetworkPathType.DIRECT,
                    )
                    for mode in requested_direct_path_modes
                ]
                # add SN feed publishers
                context.streetnetwork_path_pool.add_feed_publishers(request, requested_direct_path_modes, res)
                return res

            # We'd like to get the duration of a direct path to do some optimizations in ProximitiesByCrowflyPool and
            # FallbackDurationsPool.
            # Note :direct_paths_by_mode is a dict of mode vs future of a direct paths, this line is not blocking
            context.direct_paths_by_mode = context.streetnetwork_path_pool.get_all_direct_paths()

            crowfly_distance = crowfly_distance_between(
                get_pt_object_coord(context.requested_orig_obj), get_pt_object_coord(context.requested_dest_obj)
            )
            context.orig_proximities_by_crowfly = ProximitiesByCrowflyPool(
                future_manager=future_manager,
                instance=instance,
                requested_place_obj=context.requested_orig_obj,
                modes=requested_dep_modes_with_pt,
                request=request,
                direct_paths_by_mode=context.direct_paths_by_mode,
                max_nb_crowfly_by_mode=request['max_nb_crowfly_by_mode'],
                request_id="{}_crowfly_orig".format(request_id),
                o_d_crowfly_distance=crowfly_distance,
            )

            context.dest_proximities_by_crowfly = ProximitiesByCrowflyPool(
                future_manager=future_manager,
                instance=instance,
                requested_place_obj=context.requested_dest_obj,
                modes=requested_arr_modes_with_pt,
                request=request,
                direct_paths_by_mode=context.direct_paths_by_mode,
                max_nb_crowfly_by_mode=request['max_nb_crowfly_by_mode'],
                request_id="{}_crowfly_dest".format(request_id),
                o_d_crowfly_distance=crowfly_distance,
            )

            context.orig_places_free_access = PlacesFreeAccess(
                future_manager=future_manager,
                instance=instance,
                requested_place_obj=context.requested_orig_obj,
                request_id="{}_places_free_access_orig".format(request_id),
            )
            context.dest_places_free_access = PlacesFreeAccess(
                future_manager=future_manager,
                instance=instance,
                requested_place_obj=context.requested_dest_obj,
                request_id="{}_places_free_access_dest".format(request_id),
            )

            context.orig_fallback_durations_pool = FallbackDurationsPool(
                future_manager=future_manager,
                instance=instance,
                requested_place_obj=context.requested_orig_obj,
                modes=requested_dep_modes_with_pt,
                proximities_by_crowfly_pool=context.orig_proximities_by_crowfly,
                places_free_access=context.orig_places_free_access,
                direct_paths_by_mode=context.direct_paths_by_mode,
                request=request,
                direct_path_type=StreetNetworkPathType.BEGINNING_FALLBACK,
                request_id="{}_fallback_orig".format(request_id),
            )

            context.dest_fallback_durations_pool = FallbackDurationsPool(
                future_manager=future_manager,
                instance=instance,
                requested_place_obj=context.requested_dest_obj,
                modes=requested_arr_modes_with_pt,
                proximities_by_crowfly_pool=context.dest_proximities_by_crowfly,
                places_free_access=context.dest_places_free_access,
                direct_paths_by_mode=context.direct_paths_by_mode,
                request=request,
                direct_path_type=StreetNetworkPathType.ENDING_FALLBACK,
                request_id="{}_fallback_dest".format(request_id),
            )

        pt_journey_pool = PtJourneyPool(
            future_manager=future_manager,
            instance=instance,
            requested_orig_obj=context.requested_orig_obj,
            requested_dest_obj=context.requested_dest_obj,
            streetnetwork_path_pool=context.streetnetwork_path_pool,
            krakens_call=krakens_call,
            orig_fallback_durations_pool=context.orig_fallback_durations_pool,
            dest_fallback_durations_pool=context.dest_fallback_durations_pool,
            request=request,
            request_type=request_type,
            request_id="{}_ptjourney".format(request_id),
        )

        pt_journey_elements = wait_and_build_crowflies(
            requested_orig_obj=context.requested_orig_obj,
            requested_dest_obj=context.requested_dest_obj,
            pt_journey_pool=pt_journey_pool,
            has_valid_direct_paths=context.streetnetwork_path_pool.has_valid_direct_paths(),
            orig_places_free_access=context.orig_places_free_access,
            dest_places_free_acces=context.dest_places_free_access,
            orig_fallback_durations_pool=context.orig_fallback_durations_pool,
            dest_fallback_durations_pool=context.dest_fallback_durations_pool,
        )

        context.journeys_to_modes.update(self._map_journeys_to_modes(pt_journey_elements))

        # At the stage, all types of journeys have been computed thus we build the final result here
        res = []
        if context.partial_response_is_empty:
            for mode in requested_direct_path_modes:
                dp = context.direct_paths_by_mode.get(mode).wait_and_get()
                if getattr(dp, "journeys", None):
                    res.append(dp)

        # pt_journeys may contain None and res must be a list of protobuf journey
        res.extend((j.pt_journeys for j in pt_journey_elements if j.pt_journeys))

        check_final_results_or_raise(
            res, context.orig_fallback_durations_pool, context.dest_fallback_durations_pool
        )

        for r in res:
            fill_uris(r)

        context.partial_response_is_empty = False

        # add SN feed publishers
        context.streetnetwork_path_pool.add_feed_publishers(request, requested_direct_path_modes, res)

        return res

    def finalise_journeys(self, future_manager, request, responses, context, instance, is_debug, request_id):
        """
        Update responses that contains filtered journeys with their proper streetnetwork fallback sections.
        Fallbacks will only be computed for journeys not tagged as 'to_delete'
        """

        streetnetwork_path_pool = StreetNetworkPathPool(future_manager=future_manager, instance=instance)

        journeys_to_complete = get_journeys_to_complete(responses, context, is_debug)
        wait_and_complete_pt_journey(
            requested_orig_obj=context.requested_orig_obj,
            requested_dest_obj=context.requested_dest_obj,
            streetnetwork_path_pool=streetnetwork_path_pool,
            orig_places_free_access=context.orig_places_free_access,
            dest_places_free_access=context.dest_places_free_access,
            orig_fallback_durations_pool=context.orig_fallback_durations_pool,
            dest_fallback_durations_pool=context.dest_fallback_durations_pool,
            request=request,
            journeys=journeys_to_complete,
            request_id="{}_complete_pt_journey".format(request_id),
        )

    def _compute_isochrone_common(self, future_manager, request, instance, krakens_call, request_type):
        logger = logging.getLogger(__name__)
        logger.debug('request datetime: %s', request['datetime'])

        isochrone_center = request['origin'] or request['destination']

        mode_getter = operator.itemgetter(0 if request['origin'] else 1)
        requested_modes = {mode_getter(call) for call in krakens_call}

        logger.debug('requesting places by uri orig: %s', isochrone_center)
        request_id = request.get("request_id", None)

        requested_orig = PlaceByUri(
            future_manager=future_manager, instance=instance, uri=isochrone_center, request_id=request_id
        )

        requested_obj = get_entry_point_or_raise(requested_orig, isochrone_center)

        direct_paths_by_mode = {}

        proximities_by_crowfly = ProximitiesByCrowflyPool(
            future_manager=future_manager,
            instance=instance,
            requested_place_obj=requested_obj,
            modes=requested_modes,
            request=request,
            direct_paths_by_mode=direct_paths_by_mode,
            max_nb_crowfly_by_mode=request.get('max_nb_crowfly_by_mode', {}),
            request_id=request_id,
            o_d_crowfly_distance=None,
        )

        places_free_access = PlacesFreeAccess(
            future_manager=future_manager,
            instance=instance,
            requested_place_obj=requested_obj,
            request_id=request_id,
        )

        direct_path_type = (
            StreetNetworkPathType.BEGINNING_FALLBACK
            if request['origin']
            else StreetNetworkPathType.ENDING_FALLBACK
        )

        fallback_durations_pool = FallbackDurationsPool(
            future_manager=future_manager,
            instance=instance,
            requested_place_obj=requested_obj,
            modes=requested_modes,
            proximities_by_crowfly_pool=proximities_by_crowfly,
            places_free_access=places_free_access,
            direct_paths_by_mode=direct_paths_by_mode,
            request=request,
            request_id=request_id,
            direct_path_type=direct_path_type,
        )

        # We don't need requested_orig_obj or requested_dest_obj for isochrone
        pt_journey_args = {
            "future_manager": future_manager,
            "instance": instance,
            "requested_orig_obj": None,
            "requested_dest_obj": None,
            "streetnetwork_path_pool": None,
            "krakens_call": krakens_call,
            "request": request,
            "request_type": request_type,
            "request_id": request_id,
            "isochrone_center": isochrone_center,
        }
        if request['origin']:
            pt_journey_args.update(
                {"orig_fallback_durations_pool": fallback_durations_pool, "dest_fallback_durations_pool": None}
            )
        else:
            pt_journey_args.update(
                {"orig_fallback_durations_pool": None, "dest_fallback_durations_pool": fallback_durations_pool}
            )

        pt_journey_pool = PtJourneyPool(**pt_journey_args)

        res = []
        for (dep_mode, arr_mode, future_pt_journey) in pt_journey_pool:
            logger.debug("waiting for pt journey starts with %s and ends with %s", dep_mode, arr_mode)
            pt_journeys = wait_and_get_pt_journeys(future_pt_journey, False)
            if pt_journeys:
                res.append(pt_journeys)

            for r in res:
                fill_uris(r)

        # Graphical isochrone returns one response, not a list of responses
        if request_type == type_pb2.graphical_isochrone:
            return res[0]

        return res


class Scenario(new_default.Scenario):
    def __init__(self):
        super(Scenario, self).__init__()
        self._scenario = Distributed()
        self.greenlet_pool_size = app.config.get('GREENLET_POOL_SIZE', 8)

    def get_context(self):
        return PartialResponseContext()

    def call_kraken(self, request_type, request, instance, krakens_call, request_id, context):
        record_custom_parameter('scenario', 'distributed')
        logger = logging.getLogger(__name__)
        logger.warning("using experimental scenario!!")
        """
        All spawned futures must be started(if they're not yet started) when leaving the scope.

        We do this to prevent the programme from being blocked in case where some un-started futures may hold
        threading locks. If we leave the scope without cleaning these futures, they may hold locks forever.

        Note that the cleaning process depends on the implementation of futures.
        """
        try:
            with FutureManager(self.greenlet_pool_size) as future_manager, timed_logger(
                logger, 'call_kraken', request_id
            ):
                if request_type == type_pb2.ISOCHRONE:
                    return self._scenario._compute_isochrone_common(
                        future_manager, request, instance, krakens_call, type_pb2.ISOCHRONE
                    )
                elif request_type == type_pb2.PLANNER:
                    return self._scenario._compute_journeys(
                        future_manager, request, instance, krakens_call, context, type_pb2.PLANNER
                    )
                else:
                    abort(400, message="This type of request is not supported with distributed")
        except PtException as e:
            logger.exception('')
            return [e.get()]
        except EntryPointException as e:
            logger.exception('')
            return [e.get()]

    def finalise_journeys(self, request, responses, context, instance, is_debug, request_id):
        logger = logging.getLogger(__name__)

        try:
            with FutureManager(self.greenlet_pool_size) as future_manager, timed_logger(
                logger, 'finalise_journeys', "{}_finalise_journeys".format(request_id)
            ):
                self._scenario.finalise_journeys(
                    future_manager, request, responses, context, instance, is_debug, request_id
                )

                from jormungandr.scenarios import journey_filter

                # At this point, we should have every details on the journeys.
                # We refilter again(again and again...)
                journey_filter.filter_detailed_journeys(responses, request)

        except Exception as e:
            logger.exception('')
            final_e = FinaliseException(e)
            return [final_e.get()]

    def graphical_isochrones(self, request, instance):
        logger = logging.getLogger(__name__)
        logger.warning("using experimental scenario!!")
        """
        All spawned futures must be started(if they're not yet started) when leaving the scope.

        We do this to prevent the programme from being blocked in case where some un-started futures may hold
        threading locks. If we leave the scope without cleaning these futures, they may hold locks forever.

        Note that the cleaning process depends on the implementation of futures.
        """
        updated_common_journey_request_with_default(request, instance)
        if request.get("max_duration") is None:
            request["max_duration"] = max(request["boundary_duration[]"], key=int)
        if request.get('additional_time_after_first_section_taxi') is None:
            request[
                'additional_time_after_first_section_taxi'
            ] = instance.additional_time_after_first_section_taxi
        if request.get('additional_time_before_last_section_taxi') is None:
            request[
                'additional_time_before_last_section_taxi'
            ] = instance.additional_time_after_first_section_taxi

        krakens_call = set({(request["origin_mode"][0], request["destination_mode"][0], "indifferent")})
        try:
            with FutureManager(self.greenlet_pool_size) as future_manager:
                return self._scenario._compute_isochrone_common(
                    future_manager, request, instance, krakens_call, type_pb2.graphical_isochrone
                )
        except PtException as e:
            logger.exception('')
            return e.get()
        except EntryPointException as e:
            logger.exception('')
            return e.get()
