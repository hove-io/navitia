import Future
from jormungandr import utils
from jormungandr.street_network.street_network import DirectPathType
from navitiacommon import response_pb2
from collections import namedtuple
import copy

PtPoolElement = namedtuple('PtPoolElement', ['dep_mode', 'arr_mode', 'pt_journey'])

class PtJourney:
    def __init__(self, instance, origs, dests, periode_extremity, journey_params, bike_in_pt, request):
        self._instance = instance
        self._origs = origs
        self._dests = dests
        self._periode_extremity = periode_extremity
        self._journey_params = copy.deepcopy(journey_params)
        self._bike_in_pt = bike_in_pt
        self._request = request

        self._value = None

        self.async_request()

    def _do_request(self):
        if not self._origs or not self._dests or not self._request.get('max_duration', 0):
            return None
        resp = self._instance.planner.journeys(self._origs, self._dests,
                                               self._periode_extremity.datetime,
                                               self._periode_extremity.represents_start,
                                               self._journey_params, self._bike_in_pt)
        for j in resp.journeys:
            j.internal_id = str(utils.generate_id())

        if resp.HasField(b"error"):
            #Here needs to modify error message of no_solution
            if not self._origs:
                resp.error.id = response_pb2.Error.no_origin
                resp.error.message = "no origin point"
            elif not self._dests:
                resp.error.id = response_pb2.Error.no_destination
                resp.error.message = "no destination point"

        return resp

    def async_request(self):
        self._value = Future.create_future(self._do_request)

    def wait_and_get(self):
        return self._value.wait_and_get() if self._value else None


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

            orig_fallback_duration_status = self._orig_fallback_durtaions_pool.wait_and_get(dep_mode)
            dest_fallback_duration_status = self._dest_fallback_durations_pool.wait_and_get(arr_mode)

            orig_fallback_durations = {k: v.duration for k, v in orig_fallback_duration_status.items()}
            dest_fallback_durations = {k: v.duration for k, v in dest_fallback_duration_status.items()}

            bike_in_pt = (dep_mode == 'bike' and arr_mode == 'bike')
            pt_journey = PtJourney(self._instance, orig_fallback_durations, dest_fallback_durations,
                                   periode_extremity, self._journey_params, bike_in_pt, self._request)

            self._value.append(PtPoolElement(dep_mode, arr_mode, pt_journey))

    def __iter__(self):
        return self._value.__iter__()

    def wait_and_get(self):
        futures = [elem.pt_journey for elem in self._value] if self._value else []
        Future.wait_all(futures)
        return self._value
