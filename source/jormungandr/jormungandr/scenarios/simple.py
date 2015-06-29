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
from flask.ext.restful import abort
from jormungandr.utils import date_to_timestamp

import navitiacommon.type_pb2 as type_pb2
import navitiacommon.request_pb2 as request_pb2
import navitiacommon.response_pb2 as response_pb2
from jormungandr.interfaces.common import pb_odt_level
from jormungandr.scenarios.utils import pb_type, pt_object_type
from jormungandr.scenarios.utils import build_pagination
from datetime import datetime


class Scenario(object):
    """
    the most basic scenario, it's so simple it don't implements journeys!
    """

    def status(self, request, instance):
        req = request_pb2.Request()
        req.requested_api = type_pb2.STATUS
        resp = instance.send_and_receive(req)
        return resp

    def metadatas(self, request, instance):
        req = request_pb2.Request()
        req.requested_api = type_pb2.METADATAS
        resp = instance.send_and_receive(req)
        return resp

    def calendars(self, request, instance):
        req = request_pb2.Request()
        req.requested_api = type_pb2.calendars
        req.calendars.depth = request['depth']
        req.calendars.filter = request['filter']
        req.calendars.count = request['count']
        req.calendars.start_page = request['start_page']
        req.calendars.start_date = request['start_date']
        req.calendars.end_date = request['end_date']
        if request["forbidden_uris[]"]:
            for forbidden_uri in request["forbidden_uris[]"]:
                req.ptref.forbidden_uri.append(forbidden_uri)

        resp = instance.send_and_receive(req)
        return resp

    def disruptions(self, request, instance):
        req = request_pb2.Request()
        req.requested_api = type_pb2.disruptions
        req.disruptions.depth = request['depth']
        req.disruptions.filter = request['filter']
        req.disruptions.count = request['count']
        req.disruptions.start_page = request['start_page']
        req.disruptions._current_datetime = date_to_timestamp(request['_current_datetime'])

        if request["forbidden_uris[]"]:
            for forbidden_uri in request["forbidden_uris[]"]:
                req.ptref.forbidden_uri.append(forbidden_uri)

        resp = instance.send_and_receive(req)
        return resp

    def places(self, request, instance):
        req = request_pb2.Request()
        req.requested_api = type_pb2.places
        req.places.q = request['q']
        req.places.depth = request['depth']
        req.places.count = request['count']
        req.places.search_type = request['search_type']
        if request["type[]"]:
            for type in request["type[]"]:
                if type not in pb_type:
                    abort(422, message="{} is not an acceptable type".format(type))

                req.places.types.append(pb_type[type])

        if request["admin_uri[]"]:
            for admin_uri in request["admin_uri[]"]:
                req.places.admin_uris.append(admin_uri)

        resp = instance.send_and_receive(req)
        if len(resp.places) == 0 and request['search_type'] == 0:
            request["search_type"] = 1
            return self.places(request, instance)
        build_pagination(request, resp)

        return resp

    def pt_objects(self, request, instance):
        req = request_pb2.Request()
        req.requested_api = type_pb2.pt_objects
        req.pt_objects.q = request['q']
        req.pt_objects.depth = request['depth']
        req.pt_objects.count = request['count']
        req.pt_objects.search_type = request['search_type']
        if request["type[]"]:
            for type in request["type[]"]:
                req.pt_objects.types.append(pt_object_type[type])

        if request["admin_uri[]"]:
            for admin_uri in request["admin_uri[]"]:
                req.pt_objects.admin_uris.append(admin_uri)

        resp = instance.send_and_receive(req)
        #The result contains places but not pt_objects,
        #object place is transformed to pt_object afterwards.
        if len(resp.places) == 0 and request['search_type'] == 0:
            request["search_type"] = 1
            return self.pt_objects(request, instance)
        build_pagination(request, resp)

        return resp

    def place_uri(self, request, instance):
        req = request_pb2.Request()
        req.requested_api = type_pb2.place_uri
        req.place_uri.uri = request["uri"]
        return instance.send_and_receive(req)

    def __stop_times(self, request, instance, departure_filter,
                     arrival_filter, api):
        req = request_pb2.Request()
        req.requested_api = api
        st = req.next_stop_times
        st.departure_filter = departure_filter
        st.arrival_filter = arrival_filter
        if request["from_datetime"]:
            st.from_datetime = request["from_datetime"]
        if request["until_datetime"]:
            st.until_datetime = request["until_datetime"]
        st.duration = request["duration"]
        st.depth = request["depth"]
        st.show_codes = request["show_codes"]
        if not "nb_stoptimes" in request:
            st.nb_stoptimes = 0
        else:
            st.nb_stoptimes = request["nb_stoptimes"]
        if not "interface_version" in request:
            st.interface_version = 0
        else:
            st.interface_version = request["interface_version"]
        st.count = 10 if not "count" in request.keys() else request["count"]
        if not "start_page" in request:
            st.start_page = 0
        else:
            st.start_page = request["start_page"]
        if request["max_date_times"]:
            st.max_date_times = request["max_date_times"]
        if request["forbidden_uris[]"]:
            for forbidden_uri in request["forbidden_uris[]"]:
                st.forbidden_uri.append(forbidden_uri)
        if "calendar" in request and request["calendar"]:
            st.calendar = request["calendar"]
        st._current_datetime = date_to_timestamp(request['_current_datetime'])
        resp = instance.send_and_receive(req)
        return resp

    def route_schedules(self, request, instance):
        return self.__stop_times(request, instance, request["filter"], "", type_pb2.ROUTE_SCHEDULES)

    def next_arrivals(self, request, instance):
        return self.__stop_times(request, instance, "", request["filter"], type_pb2.NEXT_ARRIVALS)

    def next_departures(self, request, instance):
        return self.__stop_times(request, instance, request["filter"], "", type_pb2.NEXT_DEPARTURES)

    def previous_arrivals(self, request, instance):
        return self.__stop_times(request, instance, "", request["filter"], type_pb2.PREVIOUS_ARRIVALS)

    def previous_departures(self, request, instance):
        return self.__stop_times(request, instance, request["filter"], "", type_pb2.PREVIOUS_DEPARTURES)

    def stops_schedules(self, request, instance):
        return self.__stop_times(request, instance, request["departure_filter"], request["arrival_filter"],
                                 type_pb2.STOPS_SCHEDULES)

    def departure_boards(self, request, instance):
        return self.__stop_times(request, instance, request["filter"], "", type_pb2.DEPARTURE_BOARDS)


    def places_nearby(self, request, instance):
        req = request_pb2.Request()
        req.requested_api = type_pb2.places_nearby
        req.places_nearby.uri = request["uri"]
        req.places_nearby.distance = request["distance"]
        req.places_nearby.depth = request["depth"]
        req.places_nearby.count = request["count"]
        req.places_nearby.start_page = request["start_page"]
        if request["type[]"]:
            for type in request["type[]"]:
                if type not in pb_type:
                    abort(422, message="{} is not an acceptable type".format(type))

                req.places_nearby.types.append(pb_type[type])
        req.places_nearby.filter = request["filter"]
        resp = instance.send_and_receive(req)
        build_pagination(request, resp)
        return resp

    def __on_ptref(self, resource_name, requested_type, request, instance):
        req = request_pb2.Request()
        req.requested_api = type_pb2.PTREFERENTIAL

        req.ptref.requested_type = requested_type
        req.ptref.filter = request["filter"]
        req.ptref.depth = request["depth"]
        req.ptref.start_page = request["start_page"]
        req.ptref.count = request["count"]
        req.ptref.show_codes = request["show_codes"]
        req.ptref.datetime = date_to_timestamp(request["_current_datetime"])
        if request["odt_level"]:
            req.ptref.odt_level = pb_odt_level[request["odt_level"]]
        if request["forbidden_uris[]"]:
            for forbidden_uri in request["forbidden_uris[]"]:
                req.ptref.forbidden_uri.append(forbidden_uri)

        resp = instance.send_and_receive(req)
        build_pagination(request, resp)
        return resp

    def stop_areas(self, request, instance):
        return self.__on_ptref("stop_areas", type_pb2.STOP_AREA, request,instance)

    def stop_points(self, request, instance):
        return self.__on_ptref("stop_points", type_pb2.STOP_POINT, request, instance)

    def lines(self, request, instance):
        return self.__on_ptref("lines", type_pb2.LINE, request, instance)

    def line_groups(self, request, instance):
        return self.__on_ptref("line_groups", type_pb2.LINE_GROUP, request, instance)

    def routes(self, request, instance):
        return self.__on_ptref("routes", type_pb2.ROUTE, request, instance)

    def networks(self, request, instance):
        return self.__on_ptref("networks", type_pb2.NETWORK, request, instance)

    def physical_modes(self, request, instance):
        return self.__on_ptref("physical_modes", type_pb2.PHYSICAL_MODE, request, instance)

    def commercial_modes(self, request, instance):
        return self.__on_ptref("commercial_modes", type_pb2.COMMERCIAL_MODE, request, instance)

    def connections(self, request, instance):
        return self.__on_ptref("connections", type_pb2.CONNECTION, request, instance)

    def journey_pattern_points(self, request, instance):
        return self.__on_ptref("journey_pattern_points", type_pb2.JOURNEY_PATTERN_POINT, request, instance)

    def journey_patterns(self, request, instance):
        return self.__on_ptref("journey_patterns", type_pb2.JOURNEY_PATTERN, request, instance)

    def companies(self, request, instance):
        return self.__on_ptref("companies", type_pb2.COMPANY, request, instance)

    def vehicle_journeys(self, request, instance):
        return self.__on_ptref("vehicle_journeys", type_pb2.VEHICLE_JOURNEY, request, instance)

    def pois(self, request, instance):
        return self.__on_ptref("pois", type_pb2.POI, request, instance)

    def poi_types(self, request, instance):
        return self.__on_ptref("poi_types", type_pb2.POITYPE, request, instance)

    def journeys(self, request, instance):
        raise NotImplementedError()

    def nm_journeys(self, request, instance):
        raise NotImplementedError()

    def isochrone(self, request, instance):
        raise NotImplementedError()
