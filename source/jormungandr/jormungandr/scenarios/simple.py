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
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io
from __future__ import absolute_import, print_function, unicode_literals, division
from flask_restful import abort
from flask.globals import request
from jormungandr.utils import date_to_timestamp, timestamp_to_str, dt_to_str, timestamp_to_datetime

import navitiacommon.type_pb2 as type_pb2
import navitiacommon.request_pb2 as request_pb2
from jormungandr.interfaces.common import pb_odt_level
from jormungandr.scenarios.utils import places_type, pt_object_type, add_link
from jormungandr.scenarios.utils import build_pagination
from jormungandr.exceptions import UnknownObject


def get_pb_data_freshness(request):
    if request['data_freshness'] == 'realtime':
        return type_pb2.REALTIME
    elif request['data_freshness'] == 'adapted_schedule':
        return type_pb2.ADAPTED_SCHEDULE
    else:
        return type_pb2.BASE_SCHEDULE


class Scenario(object):
    """
    The most basic scenario: it's so simple, it doesn't implement journeys!
    """

    def status(self, request, instance):
        req = request_pb2.Request()
        req.requested_api = type_pb2.STATUS
        resp = instance.send_and_receive(req)
        return resp

    def geo_status(self, request, instance):
        req = request_pb2.Request()
        req.requested_api = type_pb2.geo_status
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
        req._current_datetime = date_to_timestamp(request['_current_datetime'])
        if request["forbidden_uris[]"]:
            for forbidden_uri in request["forbidden_uris[]"]:
                req.calendars.forbidden_uris.append(forbidden_uri)

        resp = instance.send_and_receive(req)
        return resp

    def traffic_reports(self, request, instance):
        req = request_pb2.Request()
        req.requested_api = type_pb2.traffic_reports
        req.traffic_reports.depth = request['depth']
        req.traffic_reports.filter = request['filter']
        req.traffic_reports.count = request['count']
        req.traffic_reports.start_page = request['start_page']
        req._current_datetime = date_to_timestamp(request['_current_datetime'])

        if request["forbidden_uris[]"]:
            for forbidden_uri in request["forbidden_uris[]"]:
                req.traffic_reports.forbidden_uris.append(forbidden_uri)

        resp = instance.send_and_receive(req)
        return resp

    def line_reports(self, request, instance):
        req = request_pb2.Request()
        req.requested_api = type_pb2.line_reports
        req.line_reports.depth = request['depth']
        req.line_reports.filter = request['filter']
        req.line_reports.count = request['count']
        req.line_reports.start_page = request['start_page']
        req._current_datetime = date_to_timestamp(request['_current_datetime'])

        if request["forbidden_uris[]"]:
            for forbidden_uri in request["forbidden_uris[]"]:
                req.line_reports.forbidden_uris.append(forbidden_uri)

        if request['since']:
            req.line_reports.since_datetime = request['since']
        if request['until']:
            req.line_reports.until_datetime = request['until']

        resp = instance.send_and_receive(req)
        return resp

    def equipment_reports(self, request, instance):
        req = request_pb2.Request()
        req.requested_api = type_pb2.equipment_reports
        req.equipment_reports.depth = request['depth']
        req.equipment_reports.filter = request['filter']
        req.equipment_reports.count = request['count']
        req.equipment_reports.start_page = request['start_page']

        if request["forbidden_uris[]"]:
            for forbidden_uri in request["forbidden_uris[]"]:
                req.equipment_reports.forbidden_uris.append(forbidden_uri)

        resp = instance.send_and_receive(req)
        return resp

    def places(self, request, instance):
        return instance.get_autocomplete(request.get('_autocomplete')).get(request, instances=[instance])

    def place_uri(self, request, instance):
        autocomplete = instance.get_autocomplete(request.get('_autocomplete'))
        request_id = request.get('request_id', None)
        try:
            return autocomplete.get_by_uri(
                uri=request["uri"],
                request_id=request_id,
                instances=[instance],
                current_datetime=request['_current_datetime'],
            )
        except UnknownObject as e:
            # the autocomplete have not found anything
            # We'll check if we can find another autocomplete system that explictly handle stop_points
            # because for the moment mimir does not have stoppoints, but kraken do
            for autocomplete_system in instance.stop_point_fallbacks():
                if autocomplete_system == autocomplete:
                    continue
                res = autocomplete_system.get_by_uri(
                    uri=request["uri"],
                    request_id=request_id,
                    instances=[instance],
                    current_datetime=request['_current_datetime'],
                )
                if res.get("places"):
                    return res
            # we raise the initial exception
            raise e

    def pt_objects(self, request, instance):
        req = request_pb2.Request()
        req.requested_api = type_pb2.pt_objects
        req.pt_objects.q = request['q']
        req.pt_objects.depth = request['depth']
        req.pt_objects.disable_geojson = request['disable_geojson']
        req.pt_objects.count = request['count']
        req.pt_objects.search_type = request['search_type']
        req._current_datetime = date_to_timestamp(request['_current_datetime'])
        if request["type[]"]:
            for type in request["type[]"]:
                req.pt_objects.types.append(pt_object_type[type])

        if request["admin_uri[]"]:
            for admin_uri in request["admin_uri[]"]:
                req.pt_objects.admin_uris.append(admin_uri)
        req.disable_disruption = request["disable_disruption"]

        resp = instance.send_and_receive(req)
        # The result contains places but not pt_objects,
        # object place is transformed to pt_object afterwards.
        if len(resp.places) == 0 and request['search_type'] == 0:
            request["search_type"] = 1
            return self.pt_objects(request, instance)
        build_pagination(request, resp)

        return resp

    def route_schedules(self, request, instance):
        return instance.schedule.route_schedules(request)

    def next_arrivals(self, request, instance):
        return instance.schedule.next_arrivals(request)

    def next_departures(self, request, instance):
        return instance.schedule.next_departures(request)

    def departure_boards(self, request, instance):
        return instance.schedule.departure_boards(request)

    def terminus_schedules(self, request, instance):
        return instance.schedule.terminus_schedules(request)

    def places_nearby(self, request, instance):
        req = request_pb2.Request()
        req.requested_api = type_pb2.places_nearby
        req.places_nearby.uri = request["uri"]
        req.places_nearby.distance = request["distance"]
        req.places_nearby.depth = request["depth"]
        req.places_nearby.count = request["count"]
        req.places_nearby.start_page = request["start_page"]
        req._current_datetime = date_to_timestamp(request["_current_datetime"])
        if request["type[]"]:
            for type in request["type[]"]:
                if type not in places_type:
                    abort(422, message="{} is not an acceptable type".format(type))

                req.places_nearby.types.append(places_type[type])
        req.places_nearby.filter = request["filter"]
        req.disable_disruption = request["disable_disruption"] if request.get("disable_disruption") else False
        resp = instance.send_and_receive(req)
        build_pagination(request, resp)
        return resp

    def __on_ptref(self, resource_name, requested_type, request, instance):
        req = request_pb2.Request()
        req.requested_api = type_pb2.PTREFERENTIAL

        req.ptref.requested_type = requested_type
        req.ptref.filter = request.get("filter", '')
        req.ptref.depth = request["depth"]
        req.ptref.start_page = request["start_page"]
        req.ptref.count = request["count"]
        req.ptref.disable_geojson = request["disable_geojson"]
        req._current_datetime = date_to_timestamp(request["_current_datetime"])
        if request["odt_level"]:
            req.ptref.odt_level = pb_odt_level[request["odt_level"]]
        if request["forbidden_uris[]"]:
            for forbidden_uri in request["forbidden_uris[]"]:
                req.ptref.forbidden_uri.append(forbidden_uri)
        if request['since']:
            req.ptref.since_datetime = request['since']
        if request['until']:
            req.ptref.until_datetime = request['until']
        req.ptref.realtime_level = get_pb_data_freshness(request)
        req.disable_disruption = request["disable_disruption"]
        resp = instance.send_and_receive(req)
        build_pagination(request, resp)
        return resp

    def stop_areas(self, request, instance):
        return self.__on_ptref("stop_areas", type_pb2.STOP_AREA, request, instance)

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

    def trips(self, request, instance):
        return self.__on_ptref("trips", type_pb2.TRIP, request, instance)

    def pois(self, request, instance):
        return self.__on_ptref("pois", type_pb2.POI, request, instance)

    def poi_types(self, request, instance):
        return self.__on_ptref("poi_types", type_pb2.POITYPE, request, instance)

    def disruptions(self, request, instance):
        return self.__on_ptref("impact", type_pb2.IMPACT, request, instance)

    def contributors(self, request, instance):
        return self.__on_ptref("contributors", type_pb2.CONTRIBUTOR, request, instance)

    def datasets(self, request, instance):
        return self.__on_ptref("datasets", type_pb2.DATASET, request, instance)

    def journeys(self, request, instance):
        raise NotImplementedError()

    def isochrone(self, request, instance):
        raise NotImplementedError()

    def _add_ridesharing_link(self, resp, params, instance):
        req = request.args.to_dict(flat=False)
        req['partner_services[]'] = 'ridesharing'
        req['datetime'] = dt_to_str(params.original_datetime)
        req['region'] = instance.name
        add_link(resp, rel='ridesharing_journeys', **req)

    def _add_prev_link(self, resp, params, clockwise):
        prev_dt = self.previous_journey_datetime(resp.journeys, clockwise)
        if prev_dt is not None:
            params['datetime'] = timestamp_to_str(prev_dt)
            params['datetime_represents'] = 'arrival'
            add_link(resp, rel='prev', **params)

    def _add_next_link(self, resp, params, clockwise):
        next_dt = self.next_journey_datetime(resp.journeys, clockwise)
        if next_dt is not None:
            params['datetime'] = timestamp_to_str(next_dt)
            params['datetime_represents'] = 'departure'
            add_link(resp, rel='next', **params)

    def _add_first_last_links(self, resp, params):
        soonest_departure_ts = min(j.departure_date_time for j in resp.journeys)
        soonest_departure = timestamp_to_datetime(soonest_departure_ts)
        if soonest_departure:
            soonest_departure = soonest_departure.replace(hour=0, minute=0, second=0)
            params['datetime'] = dt_to_str(soonest_departure)
            params['datetime_represents'] = 'departure'
            add_link(resp, rel='first', **params)

        tardiest_arrival_ts = max(j.arrival_date_time for j in resp.journeys)
        tardiest_arrival = timestamp_to_datetime(tardiest_arrival_ts)
        if tardiest_arrival:
            tardiest_arrival = tardiest_arrival.replace(hour=23, minute=59, second=59)
            params['datetime'] = dt_to_str(tardiest_arrival)
            params['datetime_represents'] = 'arrival'
            add_link(resp, rel='last', **params)

    def _compute_pagination_links(self, resp, instance, clockwise):
        if not resp.journeys:
            return

        # NOTE: we use request.args and not the parser parameters not to have the default values of the params
        # request.args is a MultiDict, we want to flatten it by having list as value when needed (flat=True)
        cloned_params = request.args.to_dict(flat=False)
        cloned_params['region'] = instance.name  # we add the region in the args to have fully qualified links

        self._add_next_link(resp, cloned_params, clockwise)
        self._add_prev_link(resp, cloned_params, clockwise)
        # we also compute first/last journey link
        self._add_first_last_links(resp, cloned_params)
