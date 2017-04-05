import Future
from jormungandr import utils
from collections import namedtuple
from jormungandr.street_network.street_network import StreetNetworkPath
import logging

DirectPathKey = namedtuple('DirectPathKey', ['mode', 'orig_uri', 'dest_uri', 'direct_path_type'])

class DirectPath:
    def __init__(self, instance, orig_obj, dest_obj, mode, fallback_extremity, request, direct_path_type):
        self._instance = instance
        self._orig_obj = orig_obj
        self._dest_obj = dest_obj
        self._mode = mode
        self._fallback_extremity = fallback_extremity
        self._request = request
        self._direct_path_type = direct_path_type
        self._future = None

        self.async_request()

    def _do_request(self):
        logger = logging.getLogger(__name__)
        logger.debug("requesting %s direct path from %s to %s by %s", self._direct_path_type,
                     self._orig_obj.uri, self._dest_obj.uri, self._mode)

        dp = self._instance.direct_path(self._mode, self._orig_obj, self._dest_obj,
                                        self._fallback_extremity, self._request,
                                        self._direct_path_type)
        if getattr(dp, "journeys", None):
            dp.journeys[0].internal_id = str(utils.generate_id())

        logger.debug("finish %s direct path from %s to %s by %s", self._direct_path_type,
                     self._orig_obj.uri, self._dest_obj.uri, self._mode)
        return dp

    def async_request(self):
        self._future = Future.create_future(self._do_request)
        return self._future

    def wait_and_get(self):
        if self._future:
            return self._future.wait_and_get()
        return None


class DirectPathPool:
    def __init__(self, instance):
        self._instance = instance
        self._value = {}

    def _make_key(self, dep_mode, orig_uri, dest_uri, direct_path_type, fallback_extremity):
        '''
        :param orig_uri, dest_uri, mode: matters obviously
        :param direct_path_type: whether it's a fallback at the beginning, the end of journey or a direct path without PT
            also matters especially for car (to know if we park before or after)
        :param fallback_extremity: is a PeriodExtremity (a datetime and it's meaning on the fallback period)
        Nota: fallback_extremity is not taken into consideration so far because we assume that a
            direct path from A to B remains the same even the departure time are different (no realtime)
        Nota: this implementation is connector-specific (so shouldn't be here)
        '''
        return DirectPathKey(dep_mode, orig_uri, dest_uri, direct_path_type)

    def add_async_request(self, requested_orig_obj, requested_dest_obj, mode, fallback_extremity, request, direct_path_type):
        key = self._make_key(mode, requested_orig_obj.uri, requested_dest_obj.uri, direct_path_type, fallback_extremity)
        if self._value.get(key):
            return
        self._value[key] = DirectPath(self._instance, requested_orig_obj, requested_dest_obj, mode,
                                      fallback_extremity, request, direct_path_type)

    def get_direct_paths_by_type(self, direct_path_type):
        res = {}
        for k in self._value:
            if k.direct_path_type is not direct_path_type:
                continue
            res[k.mode] = self._value[k]
        return res

    def has_valid_direct_path_no_pt(self):
        for k in self._value:
            if k.direct_path_type is not StreetNetworkPath.DIRECT:
                continue
            dp = self._value[k].wait_and_get()
            if getattr(dp, "journeys", None):
                return True
        return False

    def wait_and_get(self, requested_orig_obj, requested_dest_obj, mode, fallback_extremity, direct_path_type):
        key = self._make_key(mode, requested_orig_obj.uri, requested_dest_obj.uri, direct_path_type, fallback_extremity)
        dp = self._value.get(key)
        return dp.wait_and_get() if dp else None
