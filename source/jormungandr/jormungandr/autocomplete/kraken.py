# coding=utf-8

# Copyright (c) 2001-2022, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Hove (www.hove.com).
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
from jormungandr.autocomplete.abstract_autocomplete import AbstractAutocomplete, GeoStatusResponse
from jormungandr.exceptions import InvalidArguments
from jormungandr.interfaces.v1.decorators import get_serializer
from jormungandr.interfaces.v1.serializer import api
from jormungandr.scenarios.utils import build_pagination, places_type
from flask_restful import abort
import navitiacommon.request_pb2 as request_pb2
import navitiacommon.type_pb2 as type_pb2
from jormungandr.utils import date_to_timestamp


class Kraken(AbstractAutocomplete):
    @get_serializer(serpy=api.PlacesSerializer)
    def get(self, request, instances):
        if len(instances) != 1:
            raise InvalidArguments('kraken autocomplete works only for one (and only one) instance')
        instance = instances[0]
        req = request_pb2.Request()
        req.requested_api = type_pb2.places
        req.places.q = request['q']
        req.places.depth = request['depth']
        req.places.count = request['count']
        req.places.search_type = request['search_type']
        req.places.main_stop_area_weight_factor = request.get('_main_stop_area_weight_factor', 1.0)
        req._current_datetime = date_to_timestamp(request['_current_datetime'])
        req.disable_disruption = True
        if request["type[]"]:
            for type in request["type[]"]:
                if type not in places_type:
                    abort(422, message="{} is not an acceptable type".format(type))

                req.places.types.append(places_type[type])

        if request["admin_uri[]"]:
            for admin_uri in request["admin_uri[]"]:
                req.places.admin_uris.append(admin_uri)

        resp = instance.send_and_receive(req)

        if len(resp.places) == 0 and request['search_type'] == 0:
            req.places.search_type = 1
            resp = instance.send_and_receive(req)

        build_pagination(request, resp)
        return resp

    def geo_status(self, instance):
        req = request_pb2.Request()
        req.requested_api = type_pb2.geo_status
        resp = instance.send_and_receive(req)
        status = GeoStatusResponse()
        status.nb_pois = resp.geo_status.nb_poi
        status.nb_ways = resp.geo_status.nb_ways
        status.nb_admins = resp.geo_status.nb_admins
        status.nb_addresses = resp.geo_status.nb_addresses
        status.nb_admins_from_cities = resp.geo_status.nb_admins_from_cities
        if resp.geo_status.street_network_source:
            status.street_network_sources.append(resp.geo_status.street_network_source)
        if resp.geo_status.poi_source:
            status.poi_sources.append(resp.geo_status.poi_source)
        return status

    @get_serializer(serpy=api.PlacesSerializer)
    def get_by_uri(self, uri, request_id, instances=None, current_datetime=None, _add_poi_shape=False):
        if len(instances) != 1:
            raise InvalidArguments('kraken search by uri works only for one (and only one) instance')
        instance = instances[0]
        req = request_pb2.Request()
        req.requested_api = type_pb2.place_uri
        req.place_uri.uri = uri
        if current_datetime:
            req._current_datetime = date_to_timestamp(current_datetime)
        return instance.send_and_receive(req, request_id=request_id)

    def status(self):
        return {'class': self.__class__.__name__}

    def is_handling_stop_points(self):
        return True
