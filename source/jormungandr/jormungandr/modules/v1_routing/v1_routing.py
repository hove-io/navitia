# coding=utf-8

# Copyright (c) 2001-2014, Hove and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
# the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
# powered by Hove (www.kisio.com).
# Help us simplify mobility and open public transport:
# a non ending quest to the responsive locomotion way of traveling!
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
from jormungandr.interfaces.v1 import (
    Uri,
    Coverage,
    Journeys,
    GraphicalIsochrone,
    HeatMap,
    Schedules,
    Places,
    Ptobjects,
    Coord,
    Disruptions,
    Calendars,
    converters_collection_type,
    Status,
    GeoStatus,
    JSONSchema,
    LineReports,
    EquipmentReports,
    AccessPoints,
    VehiclePositions,
    free_floatings,
    users,
)
from werkzeug.routing import BaseConverter, FloatConverter, PathConverter
from jormungandr.modules_loader import AModule
from jormungandr import app

from jormungandr.modules.v1_routing.resources import Index


class RegionConverter(BaseConverter):
    """ The region you want to query"""

    type_ = str
    regex = '[^(/;)]+'

    def __init__(self, *args, **kwargs):
        BaseConverter.__init__(self, *args, **kwargs)


class LonConverter(FloatConverter):
    """ The longitude of where the coord you want to query"""

    type_ = float
    regex = '-?\\d+(\\.\\d+)?'

    def __init__(self, *args, **kwargs):
        FloatConverter.__init__(self, *args, **kwargs)


class LatConverter(FloatConverter):
    """ The latitude of where the coord you want to query"""

    type_ = float
    regex = '-?\\d+(\\.\\d+)?'

    def __init__(self, *args, **kwargs):
        FloatConverter.__init__(self, *args, **kwargs)


class UriConverter(PathConverter):
    """First part of the uri"""

    type_ = str

    def __init__(self, *args, **kwargs):
        PathConverter.__init__(self, *args, **kwargs)


class IdConverter(BaseConverter):
    """Id of the object you want to query"""

    type_ = str

    def __init__(self, *args, **kwargs):
        BaseConverter.__init__(self, *args, **kwargs)


class V1Routing(AModule):
    def __init__(self, api, name):
        super(V1Routing, self).__init__(
            api, name, description='Current version of navitia API', status='current', index_endpoint='index'
        )

    def setup(self):
        self.api.app.url_map.converters['region'] = RegionConverter
        self.api.app.url_map.converters['lon'] = LonConverter
        self.api.app.url_map.converters['lat'] = LatConverter
        self.api.app.url_map.converters['uri'] = UriConverter
        self.api.app.url_map.converters['id'] = IdConverter
        self.api.app.url_map.strict_slashes = False

        self.module_resources_manager.register_resource(Index.Index())
        self.add_resource(Index.Index, '/', '', endpoint='index')
        self.module_resources_manager.register_resource(Index.TechnicalStatus())
        self.add_resource(Index.TechnicalStatus, '/status', endpoint='technical_status')
        lon_lat = '<lon:lon>;<lat:lat>/'
        coverage = '/coverage/'
        region = coverage + '<region:region>/'
        coord = coverage + lon_lat

        self.add_resource(Coverage.Coverage, coverage, region, coord, endpoint='coverage')

        self.add_resource(
            Coord.Coord,
            '/coord/' + lon_lat,
            '/coords/' + lon_lat,
            region + 'coord/' + lon_lat + 'addresses',
            region + 'coords/' + lon_lat + 'addresses',
            endpoint='coord',
        )

        collecs = list(converters_collection_type.collections_to_resource_type.keys())
        for collection in collecs:
            # we want to hide the connections apis, as they are only for debug
            hide = collection == 'connections'
            self.add_resource(
                getattr(Uri, collection)(True),
                region + collection,
                coord + collection,
                region + '<uri:uri>/' + collection,
                coord + '<uri:uri>/' + collection,
                endpoint=collection + '.collection',
                hide=hide,
            )

            if collection == 'connections':
                # connections api cannot be query by id
                continue
            self.add_resource(
                getattr(Uri, collection)(False),
                region + collection + '/<id:id>',
                coord + collection + '/<id:id>',
                region + '<uri:uri>/' + collection + '/<id:id>',
                coord + '<uri:uri>/' + collection + '/<id:id>',
                endpoint=collection + '.id',
                hide=hide,
            )

        collecs = ["routes", "lines", "line_groups", "networks", "stop_areas", "stop_points", "vehicle_journeys"]
        for collection in collecs:
            self.add_resource(
                getattr(Uri, collection)(True), '/' + collection, endpoint=collection + '.external_codes'
            )

        self.add_resource(Places.Places, region + 'places', coord + 'places', '/places', endpoint='places')
        self.add_resource(
            Ptobjects.Ptobjects, region + 'pt_objects', coord + 'pt_objects', endpoint='pt_objects'
        )

        self.add_resource(
            Places.PlaceUri,
            '/places/<id:id>',
            region + 'places/<id:id>',
            coord + 'places/<id:id>',
            endpoint='place_uri',
        )

        self.add_resource(
            Places.PlacesNearby,
            region + 'places_nearby',
            coord + 'places_nearby',
            region + '<uri:uri>/places_nearby',
            coord + '<uri:uri>/places_nearby',
            '/coord/' + lon_lat + 'places_nearby',
            '/coords/' + lon_lat + 'places_nearby',
            endpoint='places_nearby',
        )

        self.add_resource(
            Journeys.Journeys,
            region + '<uri:uri>/journeys',
            coord + '<uri:uri>/journeys',
            region + 'journeys',
            coord + 'journeys',
            '/journeys',
            endpoint='journeys',
            # we don't want to document those routes as we consider them deprecated
            hide_routes=(region + '<uri:uri>/journeys', coord + '<uri:uri>/journeys'),
        )

        if app.config['GRAPHICAL_ISOCHRONE']:
            self.add_resource(
                GraphicalIsochrone.GraphicalIsochrone,
                region + 'isochrones',
                coord + 'isochrones',
                endpoint='isochrones',
            )

        if app.config.get('HEAT_MAP'):
            self.add_resource(HeatMap.HeatMap, region + 'heat_maps', coord + 'heat_maps', endpoint='heat_maps')

        self.add_resource(
            Schedules.RouteSchedules,
            region + '<uri:uri>/route_schedules',
            coord + '<uri:uri>/route_schedules',
            '/route_schedules',
            endpoint='route_schedules',
        )

        self.add_resource(
            Schedules.NextArrivals,
            region + '<uri:uri>/arrivals',
            coord + '<uri:uri>/arrivals',
            region + 'arrivals',
            coord + 'arrivals',
            endpoint='arrivals',
        )

        self.add_resource(
            Schedules.NextDepartures,
            region + '<uri:uri>/departures',
            coord + '<uri:uri>/departures',
            region + 'departures',
            coord + 'departures',
            endpoint='departures',
        )

        self.add_resource(
            Schedules.StopSchedules,
            region + '<uri:uri>/stop_schedules',
            coord + '<uri:uri>/stop_schedules',
            '/stop_schedules',
            endpoint='stop_schedules',
        )

        self.add_resource(
            Disruptions.TrafficReport,
            region + 'traffic_reports',
            coord + 'traffic_reports',
            region + '<uri:uri>/traffic_reports',
            coord + '<uri:uri>/traffic_reports',
            endpoint='traffic_reports',
        )

        self.add_resource(
            LineReports.LineReports,
            region + 'line_reports',
            coord + 'line_reports',
            region + '<uri:uri>/line_reports',
            coord + '<uri:uri>/line_reports',
            endpoint='line_reports',
        )

        self.add_resource(
            EquipmentReports.EquipmentReports,
            region + 'equipment_reports',
            coord + 'equipment_reports',
            region + '<uri:uri>/equipment_reports',
            coord + '<uri:uri>/equipment_reports',
            '/coord/' + lon_lat + 'equipment_reports',
            '/coords/' + lon_lat + 'equipment_reports',
            endpoint='equipment_reports',
        )

        self.add_resource(
            AccessPoints.AccessPoints,
            region + 'access_points',
            coord + 'access_points',
            region + '<uri:uri>/access_points',
            coord + '<uri:uri>/access_points',
            '/coord/' + lon_lat + 'access_points',
            '/coords/' + lon_lat + 'access_points',
            endpoint='access_points',
        )

        self.add_resource(
            VehiclePositions.VehiclePositions,
            region + 'vehicle_positions',
            coord + 'vehicle_positions',
            region + '<uri:uri>/vehicle_positions',
            coord + '<uri:uri>/vehicle_positions',
            '/coord/' + lon_lat + 'vehicle_positions',
            '/coords/' + lon_lat + 'vehicle_positions',
            endpoint='vehicle_positions',
        )

        self.add_resource(Status.Status, region + 'status', coord + 'status', endpoint='status')
        self.add_resource(
            GeoStatus.GeoStatus, region + '_geo_status', coord + '_geo_status', endpoint='geo_status'
        )

        self.add_resource(
            Calendars.Calendars,
            region + 'calendars',
            coord + 'calendars',
            region + '<uri:uri>/calendars',
            coord + '<uri:uri>/calendars',
            region + "calendars/<id:id>",
            coord + "calendars/<id:id>",
            endpoint="calendars",
        )

        self.add_resource(
            Schedules.TerminusSchedules,
            region + '<uri:uri>/terminus_schedules',
            coord + '<uri:uri>/terminus_schedules',
            '/terminus_schedules',
            endpoint='terminus_schedules',
        )

        self.add_resource(
            free_floatings.FreeFloatingsNearby,
            region + 'freefloatings_nearby',
            coord + 'freefloatings_nearby',
            region + '<uri:uri>/freefloatings_nearby',
            coord + '<uri:uri>/freefloatings_nearby',
            '/coord/' + lon_lat + 'freefloatings_nearby',
            '/coords/' + lon_lat + 'freefloatings_nearby',
            endpoint='freefloatings_nearby',
        )

        self.add_resource(users.User, "/users", endpoint='users')
        self.add_resource(JSONSchema.Schema, '/schema', endpoint="schema")
