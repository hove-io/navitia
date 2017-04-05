import Future
from navitiacommon import response_pb2
from collections import namedtuple
from jormungandr.street_network.street_network import StreetNetworkPath
from helper_utils import get_max_fallback_duration
import logging

DurationElement = namedtuple('DurationElement', ['duration', 'status'])


class FallbackDurations:
    def __init__(self, instance, requested_place_obj, mode, proximities_by_crowfly_pool, places_free_access,
                 max_duration_to_pt, request, speed_switcher):
        self._instance = instance
        self._requested_place_obj = requested_place_obj
        self._mode = mode
        self._proximities_by_crowfly_pool = proximities_by_crowfly_pool
        self._places_free_access = places_free_access
        self._max_duration_to_pt = max_duration_to_pt
        self._request = request
        self._speed_switcher = speed_switcher
        self._value = None

        self.async_request()

    def _get_duration(self, resp, place):
        from math import sqrt
        map_response = {
            response_pb2.reached: resp.duration,
            # Calculate duration
            response_pb2.unknown: int((place.distance*sqrt(2))/self._speed_switcher.get(self._mode))
        }
        return map_response[resp.routing_status]

    def _do_request(self):
        logger = logging.getLogger(__name__)
        logger.debug("requesting fallback durations from %s by %s", self._requested_place_obj.uri, self._mode)

        # When max_duration_to_pt is 0, there is no need to compute the fallback to pt, except if place is a
        # stop_point or a stop_area
        center_isochrone = self._requested_place_obj
        free_access = self._places_free_access.wait_and_get()
        all_free_access = free_access.crowfly | free_access.odt
        proximities_by_crowfly = self._proximities_by_crowfly_pool.wait_and_get(self._mode)

        # if a place is freely accessible, there is no need to compute it's access duration in isochrone
        places_isochrone = [p for p in proximities_by_crowfly if p.uri not in all_free_access]

        result = {}
        # Since we have already places that have free access, we add them into the result
        [result.update({uri: DurationElement(0, response_pb2.reached)}) for uri in all_free_access]

        if self._max_duration_to_pt == 0:
            logger.debug("max_duration_to_pt equals to 0")

            # When max_duration_to_pt is 0, we can get on the public transport ONLY if the place is a stop_point
            if self._instance.georef.get_stop_points_from_uri(center_isochrone.uri):
                return {center_isochrone.uri: DurationElement(0, response_pb2.reached)}
            else:
                return result
        sn_routing_matrix = self._instance.get_street_network_routing_matrix([center_isochrone],
                                                                             places_isochrone,
                                                                             self._mode,
                                                                             self._max_duration_to_pt,
                                                                             self._request,
                                                                             **self._speed_switcher)

        if not len(sn_routing_matrix.rows) or not len(sn_routing_matrix.rows[0].routing_response):
            logger.debug("no fallback durations found from %s by %s", self._requested_place_obj.uri, self._mode)
            return result

        for pos, r in enumerate(sn_routing_matrix.rows[0].routing_response):
            if r.routing_status != response_pb2.unreached:
                duration = self._get_duration(r, places_isochrone[pos])
                if duration < self._max_duration_to_pt:
                    result.update({places_isochrone[pos].uri: DurationElement(duration, r.routing_status)})

        # We update the fallback duration matrix if the requested origin/destination is also
        # present in the fallback duration matrix, which means from stop_point_1 to itself, it takes 0 second
        # Ex:
        #                stop_point1   stop_point2  stop_point3
        # stop_point_1         0(s)       ...          ...
        if center_isochrone.uri in result:
            result[center_isochrone.uri] = DurationElement(0, response_pb2.reached)

        logger.debug("finish fallback durations from %s by %s", self._requested_place_obj.uri, self._mode)
        return result

    def async_request(self):
        self._value = Future.create_future(self._do_request)

    def wait_and_get(self):
        return self._value.wait_and_get() if self._value else None


class FallbackDurationsPool(dict):

    def __init__(self, instance, requested_place_obj, modes, proximities_by_crowfly_pool, places_free_access,
                 direct_path_pool, request):
        super(FallbackDurationsPool, self).__init__()
        self._instance = instance
        self._requested_place_obj = requested_place_obj
        self._modes = set(modes)
        self._proximities_by_crowfly_pool = proximities_by_crowfly_pool
        self._places_free_access = places_free_access
        self._direct_path_pool = direct_path_pool
        self._request = request
        self._speed_switcher = {
            "walking": instance.walking_speed,
            "bike": instance.bike_speed,
            "car": instance.car_speed,
            "bss": instance.bss_speed,
        }

        self._value = {}

        self.async_request()

    def async_request(self):
        dps_by_mode = {}
        if self._direct_path_pool:
            dps_by_mode = self._direct_path_pool.get_direct_paths_by_type(StreetNetworkPath.DIRECT)

        for mode in self._modes:
            max_fallback_duration = get_max_fallback_duration(self._request, mode, dps_by_mode.get(mode))
            fallback_durations = FallbackDurations(self._instance, self._requested_place_obj, mode,
                                                   self._proximities_by_crowfly_pool, self._places_free_access,
                                                   max_fallback_duration,
                                                   self._request, self._speed_switcher)
            self._value[mode] = fallback_durations

    def wait_and_get(self, mode):
        f = self._value.get(mode)
        return f.wait_and_get() if f else None

    def is_empty(self):
        return next((False for _, v in self._value.items() if v.wait_and_get()), True)
