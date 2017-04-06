# coding=utf-8

# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
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
import hashlib

import logging
import pytz
from jormungandr import utils

from navitiacommon import type_pb2, request_pb2, response_pb2
from jormungandr.utils import date_to_timestamp, pb_del_if
import datetime
from copy import deepcopy
from jormungandr import new_relic

RT_PROXY_PROPERTY_NAME = 'realtime_system'
RT_PROXY_DATA_FRESHNESS = 'realtime'


def get_realtime_system_code(route_point):
    """
    If a line is associated to a realtime proxy system it has a property with the name of the system
    """
    line = route_point.pb_route.line

    rt_system = [p.value for p in line.properties if p.name == RT_PROXY_PROPERTY_NAME]

    if not rt_system:
        return None

    # for the moment we consider that there can be only one rt_system
    return rt_system[0]


class RealTimePassage(object):
    def __init__(self, datetime, direction=None, is_real_time = True):
        self.datetime = datetime
        self.direction = direction
        self.is_real_time = is_real_time


def _update_stop_schedule(stop_schedule, next_realtime_passages):
    """
    Update the stopschedule response with the new realtime passages

    for the moment we remove all base schedule data and replace them with the realtime

    If next_realtime_passages is None (and not if it's []) it means that the proxy failed,
    so we use the base schedule
    """
    if next_realtime_passages is None:
        return

    logging.getLogger(__name__).debug('next passages: : {}'
                                     .format(["dt: {}".format(d.datetime) for d in next_realtime_passages]))

    # we clean up the old schedule
    del stop_schedule.date_times[:]
    for passage in next_realtime_passages:
        new_dt = stop_schedule.date_times.add()
        midnight = datetime.datetime.combine(passage.datetime.date(), time=datetime.time(0))
        pytz.utc.localize(midnight)
        midnight = midnight.replace(tzinfo=pytz.UTC)
        time = (passage.datetime - midnight).total_seconds()
        new_dt.time = int(time)
        new_dt.date = date_to_timestamp(midnight)

        if passage.is_real_time:
            new_dt.realtime_level = type_pb2.REALTIME
        else:
            new_dt.realtime_level = type_pb2.BASE_SCHEDULE

        # we also add the direction in the note
        if passage.direction:
            note = type_pb2.Note()
            note.note = passage.direction
            note_uri = hashlib.md5(note.note.encode('utf-8')).hexdigest()
            note.uri = 'note:{md5}'.format(md5=note_uri)  # the id is a md5 of the direction to factorize them
            new_dt.properties.notes.extend([note])


def _create_template_from_passage(passage):
    template = deepcopy(passage)
    template.pt_display_informations.ClearField(str("headsign"))
    template.pt_display_informations.ClearField(str("direction"))
    template.pt_display_informations.ClearField(str("physical_mode"))
    template.pt_display_informations.ClearField(str("description"))
    template.pt_display_informations.ClearField(str("uris"))
    template.pt_display_informations.ClearField(str("has_equipments"))
    del template.pt_display_informations.messages[:]
    del template.pt_display_informations.impact_uris[:]
    del template.pt_display_informations.notes[:]
    del template.pt_display_informations.headsigns[:]
    template.stop_date_time.ClearField(str("arrival_date_time"))
    template.stop_date_time.ClearField(str("departure_date_time"))
    template.stop_date_time.ClearField(str("base_arrival_date_time"))
    template.stop_date_time.ClearField(str("base_departure_date_time"))
    template.stop_date_time.ClearField(str("properties"))
    template.stop_date_time.ClearField(str("data_freshness"))
    return template


def _create_template_from_pb_route_point(pb_route_point):
    template = response_pb2.Passage()
    template.pt_display_informations.CopyFrom(pb_route_point.pt_display_informations)
    template.route.CopyFrom(pb_route_point.route)
    template.stop_point.CopyFrom(pb_route_point.stop_point)
    return _create_template_from_passage(template)


def _update_passages(passages, route_point, template, next_realtime_passages):
    if next_realtime_passages is None:
        return

    # filter passages with entries of the asked route_point
    pb_del_if(passages, lambda p: RoutePoint(p.route, p.stop_point) == route_point)

    # append the realtime passages
    for rt_passage in next_realtime_passages:
        new_passage = deepcopy(template)
        new_passage.stop_date_time.arrival_date_time = date_to_timestamp(rt_passage.datetime)
        new_passage.stop_date_time.departure_date_time = date_to_timestamp(rt_passage.datetime)

        if rt_passage.is_real_time:
            new_passage.stop_date_time.data_freshness = type_pb2.REALTIME
        else:
            new_passage.stop_date_time.data_freshness = type_pb2.BASE_SCHEDULE

        # we also add the direction in the note
        if rt_passage.direction:
            new_passage.pt_display_informations.direction = rt_passage.direction

        # we add physical mode from route
        if len(route_point.pb_route.physical_modes) > 0:
            new_passage.pt_display_informations.physical_mode = route_point.pb_route.physical_modes[0].name

        passages.extend([new_passage])


class RoutePoint(object):
    def __init__(self, route, stop_point):
        self.pb_stop_point = stop_point
        self.pb_route = route

    def __key(self):
        return (self.pb_stop_point.uri, self.pb_route.uri)

    def __eq__(self, other):
        return self.__key() == other.__key()

    def __hash__(self):
        return hash(self.__key())

    @staticmethod
    def _get_all_codes(obj, object_id_tag):
        return [c.value for c in obj.codes if c.type == object_id_tag]

    def _get_code(self, obj, object_id_tag):
        tags = self._get_all_codes(obj, object_id_tag)
        if len(tags) < 1:
            return None
        if len(tags) > 1:
            # there is more than one RT id for the given object, which shouldn't happen
            logging.getLogger(__name__).warning('Object {o} has multiple RealTime codes for tag {t}'.
                                                format(o=obj.uri, t=object_id_tag))
        return tags[0]

    # Cache this ?
    def fetch_stop_id(self, object_id_tag):
        return self._get_code(self.pb_stop_point, object_id_tag)

    def fetch_line_id(self, object_id_tag):
        return self._get_code(self.pb_route.line, object_id_tag)

    def fetch_route_id(self, object_id_tag):
        return self._get_code(self.pb_route, object_id_tag)

    def fetch_all_route_id(self, object_id_tag):
        return self._get_all_codes(self.pb_route, object_id_tag)

    def fetch_line_code(self):
        return self.pb_route.line.code

    def fetch_line_uri(self):
        return self.pb_route.line.uri


def _get_route_point_from_stop_schedule(stop_schedule):
    rp = RoutePoint(stop_point=stop_schedule.stop_point, route=stop_schedule.route)

    # TODO we need to check that we have at least a line in the route

    return rp


class MixedSchedule(object):
    """
    class dealing with schedule (arrivals, departure, route)
    this class manages mixing of stop_times (from an external RT-provider and from kraken)
    """

    def __init__(self, instance):
        self.instance = instance

    def _get_next_realtime_passages(self, route_point, request):
        log = logging.getLogger(__name__)

        if not route_point:
            return None

        rt_system_code = get_realtime_system_code(route_point)
        if not rt_system_code:
            return None

        rt_system = self.instance.realtime_proxy_manager.get(rt_system_code)
        if not rt_system:
            log.info('impossible to find {}, no realtime added'.format(rt_system_code))
            new_relic.record_custom_event('realtime_internal_failure', {'rt_system_id': rt_system_code,
                                                                        'message': 'no handler found'})
            return None

        next_rt_passages = None
        try:
            next_rt_passages = rt_system.next_passage_for_route_point(route_point,
                                                                      request['items_per_schedule'],
                                                                      request['from_datetime'],
                                                                      request['_current_datetime'])
        except Exception as e:
            log.exception('failure while requesting next passages to external RT system {}'.format(rt_system_code))
            new_relic.record_custom_event('realtime_internal_failure', {'rt_system_id': rt_system_code,
                                                                        'message': str(e)})

        if next_rt_passages is None:
            log.debug('no next passages, using base schedule')
            return None

        return next_rt_passages

    def __stop_times(self, request, api, departure_filter="", arrival_filter=""):
        req = request_pb2.Request()
        req.requested_api = api
        req._current_datetime = date_to_timestamp(request['_current_datetime'])
        st = req.next_stop_times
        st.disable_geojson = request["disable_geojson"]
        st.departure_filter = departure_filter
        st.arrival_filter = arrival_filter
        if request["from_datetime"]:
            st.from_datetime = request["from_datetime"]
        if request["until_datetime"]:
            st.until_datetime = request["until_datetime"]
        st.duration = request["duration"]
        st.depth = request["depth"]
        if "nb_stoptimes" not in request:
            st.nb_stoptimes = 0
        else:
            st.nb_stoptimes = request["nb_stoptimes"]
        st.count = request.get("count", 10)
        if "start_page" not in request:
            st.start_page = 0
        else:
            st.start_page = request["start_page"]
        if request["items_per_schedule"]:
            st.items_per_schedule = request["items_per_schedule"]
        if request["forbidden_uris[]"]:
            for forbidden_uri in request["forbidden_uris[]"]:
                st.forbidden_uri.append(forbidden_uri)
        if request.get("calendar"):
            st.calendar = request["calendar"]
        st.realtime_level = utils.realtime_level_to_pbf(request['data_freshness'])
        resp = self.instance.send_and_receive(req)

        return resp

    def route_schedules(self, request):
        return self.__stop_times(request, api=type_pb2.ROUTE_SCHEDULES, departure_filter=request["filter"])

    def next_arrivals(self, request):
        return self.__stop_times(request, api=type_pb2.NEXT_ARRIVALS, arrival_filter=request["filter"])

    def next_departures(self, request):
        resp = self.__stop_times(request, api=type_pb2.NEXT_DEPARTURES, departure_filter=request["filter"])
        if request['data_freshness'] != RT_PROXY_DATA_FRESHNESS:
            return resp

        route_points = {RoutePoint(stop_point=passage.stop_point, route=passage.route):
                        _create_template_from_passage(passage)
                        for passage in resp.next_departures}
        route_points.update((RoutePoint(rp.route, rp.stop_point),
                             _create_template_from_pb_route_point(rp))
                            for rp in resp.route_points)

        for route_point, template in route_points.items():
            next_rt_passages = self._get_next_realtime_passages(route_point, request)
            _update_passages(resp.next_departures, route_point, template, next_rt_passages)

        # sort
        def comparator(p1, p2):
            return cmp(p1.stop_date_time.departure_date_time,
                       p2.stop_date_time.departure_date_time)
        resp.next_departures.sort(comparator)

        # handle pagination
        del resp.next_departures[resp.pagination.itemsPerPage:]
        resp.pagination.itemsOnPage = len(resp.next_departures)

        return resp

    def departure_boards(self, request):
        resp = self.__stop_times(request, api=type_pb2.DEPARTURE_BOARDS, departure_filter=request["filter"])

        if request['data_freshness'] != RT_PROXY_DATA_FRESHNESS:
            return resp

        for stop_schedule in resp.stop_schedules:
            route_point = _get_route_point_from_stop_schedule(stop_schedule)
            next_rt_passages = self._get_next_realtime_passages(route_point, request)
            _update_stop_schedule(stop_schedule, next_rt_passages)
        return resp
