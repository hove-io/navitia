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
from jormungandr.scenarios import new_default
from jormungandr.utils import PeriodExtremity
from jormungandr.street_network.street_network import StreetNetworkPathType
from jormungandr.scenarios.helper_classes import *
from jormungandr.scenarios.utils import fill_uris, switch_back_to_ridesharing
from flask import g
from jormungandr.new_relic import record_custom_parameter


class Scenario(new_default.Scenario):
    def __init__(self):
        super(Scenario, self).__init__()

    @staticmethod
    def _compute_all(future_manager, request, instance, krakens_call):
        """
        For all krakens_call, call the kraken and aggregate the responses

        return the list of all responses
        """

        logger = logging.getLogger(__name__)
        logger.debug('request datetime: %s', request['datetime'])

        requested_dep_modes = {mode for mode, _ in krakens_call}
        requested_arr_modes = {mode for _, mode in krakens_call}

        logger.debug('requesting places by uri orig: %s dest %s', request['origin'], request['destination'])

        period_extremity = PeriodExtremity(request['datetime'], request['clockwise'])

        if not g.already_done:
            g.requested_orig = PlaceByUri(future_manager=future_manager, instance=instance, uri=request['origin'])
            g.requested_dest = PlaceByUri(future_manager=future_manager, instance=instance, uri=request['destination'])

            g.requested_orig_obj = get_entry_point_or_raise(g.requested_orig, request['origin'])
            g.requested_dest_obj = get_entry_point_or_raise(g.requested_dest, request['destination'])
            g.streetnetwork_path_pool = StreetNetworkPathPool(future_manager=future_manager, instance=instance)

            for mode in requested_dep_modes:
                g.streetnetwork_path_pool.add_async_request(
                    requested_orig_obj=g.requested_orig_obj,
                    requested_dest_obj=g.requested_dest_obj,
                    mode=mode,
                    period_extremity=period_extremity,
                    request=request,
                    streetnetwork_path_type=StreetNetworkPathType.DIRECT,
                )

            # if max_duration(time to pass in pt) is zero, there is no need to continue,
            # we return all direct path without pt
            if request['max_duration'] == 0:
                return [
                    g.streetnetwork_path_pool.wait_and_get(
                        requested_orig_obj=g.requested_orig_obj,
                        requested_dest_obj=g.requested_dest_obj,
                        mode=mode,
                        request=request,
                        period_extremity=period_extremity,
                        streetnetwork_path_type=StreetNetworkPathType.DIRECT,
                    )
                    for mode in requested_dep_modes
                ]

            # We'd like to get the duration of a direct path to do some optimizations in ProximitiesByCrowflyPool and
            # FallbackDurationsPool.
            # Note :direct_paths_by_mode is a dict of mode vs future of a direct paths, this line is not blocking
            g.direct_paths_by_mode = g.streetnetwork_path_pool.get_all_direct_paths()

            g.orig_proximities_by_crowfly = ProximitiesByCrowflyPool(
                future_manager=future_manager,
                instance=instance,
                requested_place_obj=g.requested_orig_obj,
                modes=requested_dep_modes,
                request=request,
                direct_paths_by_mode=g.direct_paths_by_mode,
                max_nb_crowfly_by_mode=request['max_nb_crowfly_by_mode'],
            )

            g.dest_proximities_by_crowfly = ProximitiesByCrowflyPool(
                future_manager=future_manager,
                instance=instance,
                requested_place_obj=g.requested_dest_obj,
                modes=requested_arr_modes,
                request=request,
                direct_paths_by_mode=g.direct_paths_by_mode,
                max_nb_crowfly_by_mode=request['max_nb_crowfly_by_mode'],
            )

            g.orig_places_free_access = PlacesFreeAccess(
                future_manager=future_manager, instance=instance, requested_place_obj=g.requested_orig_obj
            )
            g.dest_places_free_access = PlacesFreeAccess(
                future_manager=future_manager, instance=instance, requested_place_obj=g.requested_dest_obj
            )

            g.orig_fallback_durations_pool = FallbackDurationsPool(
                future_manager=future_manager,
                instance=instance,
                requested_place_obj=g.requested_orig_obj,
                modes=requested_dep_modes,
                proximities_by_crowfly_pool=g.orig_proximities_by_crowfly,
                places_free_access=g.orig_places_free_access,
                direct_paths_by_mode=g.direct_paths_by_mode,
                request=request,
                direct_path_type=StreetNetworkPathType.BEGINNING_FALLBACK,
            )

            g.dest_fallback_durations_pool = FallbackDurationsPool(
                future_manager=future_manager,
                instance=instance,
                requested_place_obj=g.requested_dest_obj,
                modes=requested_arr_modes,
                proximities_by_crowfly_pool=g.dest_proximities_by_crowfly,
                places_free_access=g.dest_places_free_access,
                direct_paths_by_mode=g.direct_paths_by_mode,
                request=request,
                direct_path_type=StreetNetworkPathType.ENDING_FALLBACK,
            )

        pt_journey_pool = PtJourneyPool(
            future_manager=future_manager,
            instance=instance,
            requested_orig_obj=g.requested_orig_obj,
            requested_dest_obj=g.requested_dest_obj,
            streetnetwork_path_pool=g.streetnetwork_path_pool,
            krakens_call=krakens_call,
            orig_fallback_durations_pool=g.orig_fallback_durations_pool,
            dest_fallback_durations_pool=g.dest_fallback_durations_pool,
            request=request,
        )

        completed_pt_journeys = wait_and_complete_pt_journey(
            future_manager=future_manager,
            requested_orig_obj=g.requested_orig_obj,
            requested_dest_obj=g.requested_dest_obj,
            pt_journey_pool=pt_journey_pool,
            streetnetwork_path_pool=g.streetnetwork_path_pool,
            orig_places_free_access=g.orig_places_free_access,
            dest_places_free_access=g.dest_places_free_access,
            orig_fallback_durations_pool=g.orig_fallback_durations_pool,
            dest_fallback_durations_pool=g.dest_fallback_durations_pool,
            request=request,
        )

        # At the stage, all types of journeys have been computed thus we build the final result here
        res = []
        if not g.already_done:
            for mode in requested_dep_modes:
                dp = g.direct_paths_by_mode.get(mode).wait_and_get()
                if getattr(dp, "journeys", None):
                    if mode == "ridesharing":
                        switch_back_to_ridesharing(dp, True)
                    res.append(dp)

        # completed_pt_journeys may contain None and res must be a list of protobuf journey
        res.extend((j for j in completed_pt_journeys if j))

        check_final_results_or_raise(res, g.orig_fallback_durations_pool, g.dest_fallback_durations_pool)

        for r in res:
            fill_uris(r)

        g.already_done = True
        return res

    def call_kraken(self, request_type, request, instance, krakens_call):
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
            with FutureManager() as future_manager:
                res = self._compute_all(future_manager, request, instance, krakens_call)
                return res
        except PtException as e:
            return [e.get()]
        except EntryPointException as e:
            return [e.get()]

    def isochrone(self, request, instance):
        return new_default.Scenario().isochrone(request, instance)
