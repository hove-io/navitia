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
from flask.ext.restful import fields
from jormungandr.interfaces.v1.serializer import base, api

class CollectionException(Exception):
    pass

class NonNullList(fields.List):

    def __init__(self, *args, **kwargs):
        super(NonNullList, self).__init__(*args, **kwargs)
        self.display_empty = False

class NonNullNested(fields.Nested):

    def __init__(self, *args, **kwargs):
        super(NonNullNested, self).__init__(*args, **kwargs)
        self.display_null = False
        self.allow_null = True


common_collection = (
    ("pagination", base.PbField(api.PaginationSerializer)),
    ("error",  base.PbField(api.ErrorSerializer)),
    ("feed_publishers", NonNullList(fields.Nested(api.FeedPublisherSerializer, display_null=False))),
    ("context", api.ContextSerializer),
    ("disruptions", fields.List(NonNullNested(api.DisruptionsSerializer), attribute="impacts"))
)


def get_collections(collection_name):
    map_collection = {
        "journey_pattern_points": api.JourneyPatternPointsSerializer,
        "commercial_modes": api.CommercialModesSerializer,
        "journey_patterns": api.JourneyPatternsSerializer,
        "vehicle_journeys": api.VehicleJourneysSerializer,
        "trips": api.TripsSerializer,
        "physical_modes": api.PhysicalModesSerializer,
        "stop_points": api.StopPointsSerializer,
        "stop_areas": api.StopAreasSerializer,
        "connections": api.ConnectionsSerializer,
        "companies": api.CompaniesSerializer,
        "poi_types": api.PoiTypesSerializer,
        "routes": api.RoutesSerializer,
        "line_groups": api.LineGroupsSerializer,
        "lines": api.LinesSerializer,
        "pois": api.PoisSerializer,
        "networks": api.NetworksSerializer,
        "disruptions": None,
        "contributors": api.ContributorsSerializer,
        "datasets": api.DatasetsSerializer,
    }

    collection = map_collection.get(collection_name)
    if collection:
        return [(collection_name, NonNullList(fields.Nested(collection, display_null=False)))] + list(common_collection)

    if collection_name == 'disruptions':
        return list(common_collection)

    raise CollectionException


def add_common_status(response, instance):
    response['status']["is_open_data"] = instance.is_open_data
    response['status']["is_open_service"] = instance.is_free
    response['status']['realtime_proxies'] = []
    for realtime_proxy in instance.realtime_proxy_manager.realtime_proxies.values():
        response['status']['realtime_proxies'].append(realtime_proxy.status())

    response['status']['street_networks'] = []
    for sn in instance.street_network_services:
        response['status']['street_networks'].append(sn.status())

    response['status']['ridesharing_services'] = []
    for rs in instance.ridesharing_services:
        response['status']['ridesharing_services'].append(rs.status())

    response['status']['autocomplete'] = instance.autocomplete.status()
