import Future
from jormungandr import utils
from jormungandr.street_network.street_network import DirectPathType
from navitiacommon import response_pb2
from collections import namedtuple
import copy

PtPoolElement = namedtuple('PtPoolElement', ['dep_mode', 'arr_mode', 'pt_journey'])

class PtJourney:
    def __init__(self, instance, orig_fallback_durtaions_pool, dest_fallback_durations_pool,
                 dep_mode, arr_mode, periode_extremity, journey_params, bike_in_pt, request):
        self._instance = instance
        self._orig_fallback_durtaions_pool = orig_fallback_durtaions_pool
        self._dest_fallback_durations_pool = dest_fallback_durations_pool
        self._dep_mode = dep_mode
        self._arr_mode = arr_mode
        self._periode_extremity = periode_extremity
        self._journey_params = copy.deepcopy(journey_params)
        self._bike_in_pt = bike_in_pt
        self._request = request
        self._value = None

        self.async_request()

    def _do_request(self):

        orig_fallback_duration_status = self._orig_fallback_durtaions_pool.wait_and_get(self._dep_mode)
        dest_fallback_duration_status = self._dest_fallback_durations_pool.wait_and_get(self._arr_mode)

        orig_fallback_durations = {k: v.duration for k, v in orig_fallback_duration_status.items()}
        dest_fallback_durations = {k: v.duration for k, v in dest_fallback_duration_status.items()}

        if not orig_fallback_durations or not dest_fallback_durations or not self._request.get('max_duration', 0):
            return None
        resp = self._instance.planner.journeys(orig_fallback_durations,
                                               dest_fallback_durations,
                                               self._periode_extremity.datetime,
                                               self._periode_extremity.represents_start,
                                               self._journey_params, self._bike_in_pt)
        for j in resp.journeys:
            j.internal_id = str(utils.generate_id())

        if resp.HasField(b"error"):
            #Here needs to modify error message of no_solution
            if not orig_fallback_durations:
                resp.error.id = response_pb2.Error.no_origin
                resp.error.message = "no origin point"
            elif not dest_fallback_durations:
                resp.error.id = response_pb2.Error.no_destination
                resp.error.message = "no destination point"
        return resp

    def async_request(self):
        self._value = Future.create_future(self._do_request)

    def wait_and_get(self):
        return self._value.wait_and_get() if self._value else None


class _PtJourneySorter(object):

    mode_weight = {"walking": 1, "bike": 100, "bss": 500, "car": 1000}

    def __call__(self, a, b):
        a_weight = self.mode_weight.get(a.dep_mode) + self.mode_weight.get(a.dep_mode)
        b_weight = self.mode_weight.get(b.dep_mode) + self.mode_weight.get(b.dep_mode)
        return -1 if a_weight < b_weight else 1

class PtJourneyPool:
    def __init__(self, instance, requested_orig_obj, requested_dest_obj, direct_path_pool, kraken_calls, orig_fallback_durtaions_pool,
                 dest_fallback_durations_pool, request):
        self._instance = instance
        self._requested_orig_obj = requested_orig_obj
        self._requested_dest_obj = requested_dest_obj
        self._direct_path_pool = direct_path_pool
        self._kraken_calls = kraken_calls
        self._orig_fallback_durtaions_pool = orig_fallback_durtaions_pool
        self._dest_fallback_durations_pool = dest_fallback_durations_pool
        self._journey_params = self._create_parameters(request)
        self._request = request
        self._value = []

        self.async_request()

    @staticmethod
    def _create_parameters(request):
        from jormungandr.planner import JourneyParameters
        return JourneyParameters(max_duration=request['max_duration'],
                                 max_transfers=request['max_transfers'],
                                 wheelchair=request['wheelchair'] or False,
                                 realtime_level=request['data_freshness'],
                                 max_extra_second_pass=request['max_extra_second_pass'],
                                 walking_transfer_penalty=request['_walking_transfer_penalty'],
                                 forbidden_uris=request['forbidden_uris[]'])

    def async_request(self):
        direct_path_type = DirectPathType.DIRECT_NO_PT
        periode_extremity = utils.PeriodExtremity(self._request['datetime'], self._request['clockwise'])
        for dep_mode, arr_mode in self._kraken_calls:
            dp = self._direct_path_pool.wait_and_get(self._requested_orig_obj,
                                                     self._requested_dest_obj,
                                                     dep_mode,
                                                     periode_extremity,
                                                     direct_path_type)
            if dp.journeys:
                self._journey_params.direct_path_duration = dp.journeys[0].durations.total
            else:
                self._journey_params.direct_path_duration = None

            bike_in_pt = (dep_mode == 'bike' and arr_mode == 'bike')
            pt_journey = PtJourney(self._instance,
                                   self._orig_fallback_durtaions_pool, self._dest_fallback_durations_pool,
                                   dep_mode, arr_mode,
                                   periode_extremity, self._journey_params, bike_in_pt, self._request)

            self._value.append(PtPoolElement(dep_mode, arr_mode, pt_journey))

        self._value.sort(_PtJourneySorter())

    def __iter__(self):
        return self._value.__iter__()
