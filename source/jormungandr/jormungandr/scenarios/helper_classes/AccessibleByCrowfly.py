import Future
from navitiacommon import type_pb2
from jormungandr import utils
from collections import namedtuple

AccessibleByCrowflyResult = namedtuple('AccessibleByCrowflyResult', ['crowfly', 'odt'])

class AccessibleByCrowfly:
    """
    stop_points that are accessible by crowfly: odt, stop_points of a stop_area, etc.
    """
    def __init__(self, instance, requested_place_obj):
        self._instance = instance
        self._requested_place_obj = requested_place_obj
        self._value = None
        self.async_request()

    def _do_request(self):
        crowfly = set()
        odt = set()

        stop_points = []
        place = self._requested_place_obj

        if place.embedded_type == type_pb2.STOP_AREA:
            stop_points = self._instance.georef.get_stop_points_for_stop_area(place.uri)
        elif place.embedded_type == type_pb2.ADMINISTRATIVE_REGION:
            stop_points = [sp for sa in place.administrative_region.main_stop_areas
                           for sp in sa.stop_points]
        elif place.embedded_type == type_pb2.STOP_POINT:
            stop_points = [place.stop_point]

        [crowfly.add(stop_point.uri) for stop_point in stop_points]

        coord = utils.get_pt_object_coord(place)

        if coord:
            odt_sps = self._instance.georef.get_odt_stop_points(coord)
            [odt.add(stop_point.uri) for stop_point in odt_sps]

        return AccessibleByCrowflyResult(crowfly, odt)

    def async_request(self):
        self._value = Future.create_future(self._do_request)

    def wait_and_get(self):
        return self._value.wait_and_get() if self._value else None
