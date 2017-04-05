import Future
from jormungandr.street_network.street_network import StreetNetworkPath
from helper_utils import get_max_fallback_duration
from jormungandr import utils
import logging


class ProximitiesByCrowfly:
    def __init__(self, instance, requested_place_obj, mode, max_duration, max_nb_crowfly=5000):
        self._instance = instance
        self._requested_place_obj = requested_place_obj
        self._mode = mode
        self._max_duration = max_duration
        self._max_nb_crowfly = max_nb_crowfly
        self._speed_switcher = {
            "walking": instance.walking_speed,
            "bike": instance.bike_speed,
            "car": instance.car_speed,
            "bss": instance.bss_speed,
        }
        self._value = None
        self.async_request()

    def _do_request(self):
        logger = logging.getLogger(__name__)
        logger.debug("requesting proximities by crowfly from %s in %s", self._requested_place_obj.uri, self._mode)

        # When max_duration_to_pt is 0, there is no need to compute the fallback to pt, except if place is a stop_point
        # or a stop_area
        if self._max_duration == 0:
            logger.debug("max duration equals to 0, no need to compute proximities by crowfly")

            # When max_duration_to_pt is 0, we can get on the public transport ONLY if the place is a stop_point
            if self._instance.georef.get_stop_points_from_uri(self._requested_place_obj.uri):
                return [self._requested_place_obj]

        coord = utils.get_pt_object_coord(self._requested_place_obj)
        if coord.lat and coord.lon:
            crow_fly = self._instance.georef.get_crow_fly(utils.get_uri_pt_object(self._requested_place_obj),
                                                          self._mode,  self._max_duration, self._max_nb_crowfly,
                                                          **self._speed_switcher)

            logger.debug("finish proximities by crowfly from %s in %s", self._requested_place_obj.uri, self._mode)
            return crow_fly

        logger.debug("the coord of requested places is not valid: %s", coord)
        return []

    def async_request(self):
        self._value = Future.create_future(self._do_request)

    def wait_and_get(self):
        return self._value.wait_and_get() if self._value else None


class ProximitiesByCrowflyPool:
    def __init__(self, instance, requested_place_obj, modes, request, direct_path_pool, max_nb_crowfly=5000):
        self._instance = instance
        self._requested_place_obj = requested_place_obj
        # we don't want to compute the same mode several times
        self._modes = set(modes)
        self._request = request
        self._direct_path_pool = direct_path_pool
        self._max_nb_crowfly = max_nb_crowfly

        self._future = None
        self._value = {}

        self.async_request()

    def async_request(self):
        dps_by_mode = {}
        if self._direct_path_pool:
            dps_by_mode = self._direct_path_pool.get_direct_paths_by_type(StreetNetworkPath.DIRECT)

        for mode in self._modes:
            max_fallback_duration = get_max_fallback_duration(self._request, mode, dps_by_mode.get(mode))
            p = ProximitiesByCrowfly(self. _instance, self._requested_place_obj, mode, max_fallback_duration,
                                     self._max_nb_crowfly)

            self._value[mode] = p

    def wait_and_get(self, mode):
        c = self._value.get(mode)
        return c.wait_and_get() if c else None
