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

from copy import deepcopy
import itertools
import logging
from flask.ext.restful import abort
from jormungandr.scenarios import simple, journey_filter
from jormungandr.scenarios.utils import journey_sorter, change_ids, updated_request_with_default, get_or_default, \
    fill_uris
from navitiacommon import type_pb2, response_pb2, request_pb2


def get_kraken_calls(request):
    """
    return a list of tuple (departure fallback mode, arrival fallback mode)
    from the request dict.

    for the moment it's a simple stuff:
    if there is only one elt in {first|last}_section_mode we take those
    else we do the cartesian product and remove forbidden association.

    The good thing to do would be to have API param to be able to better handle this
    """
    dep_modes = request['origin_mode']
    arr_modes = request['destination_mode']

    if len(dep_modes) == len(arr_modes) == 1:
        return [(dep_modes[0], arr_modes[0])]

    # this allowed combination is temporary, it does not handle all the use cases at all
    allowed_combination = [('bss', 'bss'),
                           ('walking', 'walking'),
                           ('bike', 'walking'),
                           ('car', 'walking')]

    res = [c for c in allowed_combination if c in itertools.product(dep_modes, arr_modes)]

    if not res:
        abort(404, 'the asked first_section_mode[] ({}) and last_section_mode[] '
                   '({}) combination is not yet supported'.format(dep_modes, arr_modes))

    return res


def create_pb_request(requested_type, request, dep_mode, arr_mode):
    """Parse the request dict and create the protobuf version"""
    #TODO: bench if the creation of the request each time is expensive
    req = request_pb2.Request()
    req.requested_api = requested_type

    if "origin" in request and request["origin"]:
        if requested_type != type_pb2.NMPLANNER:
            origins, durations = ([request["origin"]], [0])
        else:
            # in the n-m query, we have several origin points, with their corresponding access duration
            origins, durations = (request["origin"], request["origin_access_duration"])
        for place, duration in zip(origins, durations):
            location = req.journeys.origin.add()
            location.place = place
            location.access_duration = duration
    if "destination" in request and request["destination"]:
        if requested_type != type_pb2.NMPLANNER:
            destinations, durations = ([request["destination"]], [0])
        else:
            destinations, durations = (request["destination"], request["destination_access_duration"])
        for place, duration in zip(destinations, durations):
            location = req.journeys.destination.add()
            location.place = place
            location.access_duration = duration

    req.journeys.datetimes.append(request["datetime"])  #TODO remove this datetime list completly in another PR

    req.journeys.clockwise = request["clockwise"]
    sn_params = req.journeys.streetnetwork_params
    sn_params.max_walking_duration_to_pt = request["max_walking_duration_to_pt"]
    sn_params.max_bike_duration_to_pt = request["max_bike_duration_to_pt"]
    sn_params.max_bss_duration_to_pt = request["max_bss_duration_to_pt"]
    sn_params.max_car_duration_to_pt = request["max_car_duration_to_pt"]
    sn_params.walking_speed = request["walking_speed"]
    sn_params.bike_speed = request["bike_speed"]
    sn_params.car_speed = request["car_speed"]
    sn_params.bss_speed = request["bss_speed"]
    sn_params.origin_filter = request.get("origin_filter", "")
    sn_params.destination_filter = request.get("destination_filter", "")

    #settings fallback modes
    sn_params.origin_mode = dep_mode
    sn_params.destination_mode = arr_mode

    req.journeys.max_duration = request["max_duration"]
    req.journeys.max_transfers = request["max_transfers"]
    req.journeys.wheelchair = request["wheelchair"]
    req.journeys.disruption_active = request["disruption_active"]
    req.journeys.show_codes = request["show_codes"]

    if "details" in request and request["details"]:
        req.journeys.details = request["details"]

    for forbidden_uri in get_or_default(request, "forbidden_uris[]", []):
        req.journeys.forbidden_uris.append(forbidden_uri)

    return req


def create_next_kraken_request(request, responses):
    """
    create a new request dict to call the next (resp previous for non clockwise search) journeys in kraken

    to do that we find the soonest departure (resp tardiest arrival) we have in the responses
    and add (resp remove) one minute
    """
    one_minute = 60
    if request['clockwise']:
        soonest_departure = min(j.departure_date_time for r in responses for j in r.journeys)
        new_datetime = soonest_departure + one_minute
    else:
        tardiest_arrival = max(j.arrival_date_time for r in responses for j in r.journeys)
        new_datetime = tardiest_arrival - one_minute

    request['datetime'] = new_datetime

    #TODO forbid ODTs
    return request


def sort_journeys(resp, journey_order, clockwise):
    if resp.journeys:
        resp.journeys.sort(journey_sorter[journey_order](clockwise=clockwise))


def tag_journeys(resp):
    """
    qualify the journeys
    """
    pass


def culling_journeys(resp, request):
    """
    remove some journeys if there are too many of them
    since the journeys are sorted, we remove the last elt in the list

    no remove done in debug
    """
    if request['debug']:
        return

    if request["max_nb_journeys"] and len(resp.journeys) > request["max_nb_journeys"]:
        del resp.journeys[request["max_nb_journeys"]:]


def nb_journeys(responses):
    return sum(len(r.journeys) for r in responses)


def merge_responses(responses):
    """
    Merge all responses in one protobuf response
    """
    merged_response = response_pb2.Response()

    for r in responses:
        if r.HasField('error') or not r.journeys:
            # we do not take responses with error, but if all responses have errors, we'll aggregate them
            continue

        change_ids(r, len(merged_response.journeys))

        #we don't want to add a journey already there
        merged_response.journeys.extend(r.journeys)

        # we have to add the additional fares too
        # if at least one journey has the ticket we add it
        tickets_to_add = set(t for j in r.journeys for t in j.fare.ticket_id)
        merged_response.tickets.extend([t for t in r.tickets if t.id in tickets_to_add])

        initial_feed_publishers = {}
        for fp in merged_response.feed_publishers:
            initial_feed_publishers[fp.id] = fp
        merged_response.feed_publishers.extend([fp for fp in r.feed_publishers if fp.id not in initial_feed_publishers])

    if not merged_response.journeys:
        # we aggregate the errors found

        errors = {r.error.id: r.error for r in responses if r.HasField('error')}
        if len(errors) == 1:
            merged_response.error.id = next(errors.itervalues()).id
            merged_response.error.message = next(errors.itervalues()).message
        else:
            # we need to merge the errors
            merged_response.error.id = response_pb2.Error.no_solution
            merged_response.error.message = "several errors occured: \n * {}".format("\n * ".join(errors.itervalues()))

    return merged_response


class Scenario(simple.Scenario):
    """
    TODO: a bit of explanation about the new scenario
    """
    def __init__(self):
        super(Scenario, self).__init__()
        self.nb_kraken_calls = 0

    def fill_journeys(self, request_type, api_request, instance):

        krakens_call = get_kraken_calls(api_request)

        request = deepcopy(api_request)
        min_asked_journeys = get_or_default(request, 'min_nb_journeys', 1)
        
        responses = []
        last_nb_journeys = 0
        while nb_journeys(responses) < min_asked_journeys:

            tmp_resp = self.call_kraken(request_type, request, instance, krakens_call)

            responses.extend(tmp_resp)
            new_nb_journeys = nb_journeys(responses)
            if new_nb_journeys == 0:
                #no new journeys found, we stop
                break
            
            #we filter unwanted journeys by side effects
            journey_filter.filter_journeys(responses, instance, request=request, original_request=api_request)

            if last_nb_journeys == new_nb_journeys:
                #we are stuck with the same number of journeys, we stops
                break
            last_nb_journeys = new_nb_journeys
            
            request = create_next_kraken_request(request, responses)

        pb_resp = merge_responses(responses)
        sort_journeys(pb_resp, instance.journey_order, request['clockwise'])
        tag_journeys(pb_resp)
        culling_journeys(pb_resp, request)

        return pb_resp


    def call_kraken(self, request_type, request, instance, krakens_call):
        """
        For all krakens_call, call the kraken and aggregate the responses

        return the list of all responses
        """
        #TODO: handle min_alternative_journeys
        #TODO: call first bss|bss and do not call walking|walking if no bss in first results

        resp = []
        logger = logging.getLogger(__name__)
        for dep_mode, arr_mode in krakens_call:
            pb_request = create_pb_request(request_type, request, dep_mode, arr_mode)
            self.nb_kraken_calls += 1

            local_resp = instance.send_and_receive(pb_request)

            #for log purpose we put and id in each journeys
            for idx, j in enumerate(local_resp.journeys):
                j.internal_id = "{resp}-{j}".format(resp=self.nb_kraken_calls, j=idx)

            resp.append(local_resp)
            logger.debug("for mode %s|%s we have found %s journeys", dep_mode, arr_mode, len(local_resp.journeys))

        for r in resp:
            fill_uris(r)
        return resp

    def __on_journeys(self, requested_type, request, instance):
        updated_request_with_default(request, instance)

        # call to kraken
        resp = self.fill_journeys(requested_type, request, instance)
        return resp

    def journeys(self, request, instance):
        return self.__on_journeys(type_pb2.PLANNER, request, instance)

    def nm_journeys(self, request, instance):
        return self.__on_journeys(type_pb2.NMPLANNER, request, instance)

    def isochrone(self, request, instance):
        return self.__on_journeys(type_pb2.ISOCHRONE, request, instance)
