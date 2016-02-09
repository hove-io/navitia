# coding=utf-8

# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
# the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
# powered by Canal TP (www.canaltp.fr).
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
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io
from jormungandr.interfaces.v1 import Uri
from jormungandr.interfaces.v1 import Coverage
from jormungandr.interfaces.v1 import Journeys
from jormungandr.interfaces.v1 import Schedules
from jormungandr.interfaces.v1 import Places
from jormungandr.interfaces.v1 import Ptobjects
from jormungandr.interfaces.v1 import Coord
from jormungandr.interfaces.v1 import Disruptions
from jormungandr.interfaces.v1 import Calendars
from jormungandr.interfaces.v1 import converters_collection_type
from jormungandr.interfaces.v1 import Status
from werkzeug.routing import BaseConverter, FloatConverter, PathConverter
from jormungandr.modules_loader import AModule

from resources import Index


class RegionConverter(BaseConverter):
    """ The region you want to query"""

    def __init__(self, *args, **kwargs):
        BaseConverter.__init__(self, *args, **kwargs)
        self.type_ = "string"
        self.regex = '[^(/;)]+'


class LonConverter(FloatConverter):
    """ The longitude of where the coord you want to query"""

    def __init__(self, *args, **kwargs):
        FloatConverter.__init__(self, *args, **kwargs)
        self.type_ = "float"
        self.regex = '-?\\d+(\\.\\d+)?'


class LatConverter(FloatConverter):
    """ The latitude of where the coord you want to query"""

    def __init__(self, *args, **kwargs):
        FloatConverter.__init__(self, *args, **kwargs)
        self.type_ = "float"
        self.regex = '-?\\d+(\\.\\d+)?'


class UriConverter(PathConverter):
    """First part of the uri"""

    def __init__(self, *args, **kwargs):
        PathConverter.__init__(self, *args, **kwargs)
        self.type_ = "string"


class IdConverter(BaseConverter):
    """Id of the object you want to query"""

    def __init__(self, *args, **kwargs):
        BaseConverter.__init__(self, *args, **kwargs)
        self.type_ = "string"


class V1Routing(AModule):
    def __init__(self, api, name):
        super(V1Routing, self).__init__(api, name,
                                        description='Current version of navitia API',
                                        status='current',
                                        index_endpoint='index')

    def setup(self):
        self.api.app.url_map.converters['region'] = RegionConverter
        self.api.app.url_map.converters['lon'] = LonConverter
        self.api.app.url_map.converters['lat'] = LatConverter
        self.api.app.url_map.converters['uri'] = UriConverter
        self.api.app.url_map.converters['id'] = IdConverter
        self.api.app.url_map.strict_slashes = False

        self.module_resources_manager.register_resource(Index.Index())
        self.add_resource(Index.Index,
                          '/',
                          '',
                          endpoint='index')
        self.module_resources_manager.register_resource(Index.TechnicalStatus())
        self.add_resource(Index.TechnicalStatus,
                          '/status',
                          endpoint='technical_status')

        coverage = '/coverage/'
        region = coverage + '<region:region>/'
        coord = coverage + '<lon:lon>;<lat:lat>/'

        self.add_resource(Coverage.Coverage,
                          coverage,
                          region,
                          coord,
                          endpoint='coverage')

        self.add_resource(Coord.Coord,
                          '/coord/<lon:lon>;<lat:lat>',
                          '/coords/<lon:lon>;<lat:lat>',
                          endpoint='coord')

        collecs = converters_collection_type.collections_to_resource_type.keys()
        for collection in collecs:
            self.add_resource(getattr(Uri, collection)(True),
                              region + collection,
                              coord + collection,
                              region + '<uri:uri>/' + collection,
                              coord + '<uri:uri>/' + collection,
                              endpoint=collection + '.collection')

            self.add_resource(getattr(Uri, collection)(False),
                              region + collection + '/<id:id>',
                              coord + collection + '/<id:id>',
                              region + '<uri:uri>/' + collection + '/<id:id>',
                              coord + '<uri:uri>/' + collection + '/<id:id>',
                              endpoint=collection + '.id')

        collecs = ["routes", "lines", "line_groups", "networks", "stop_areas", "stop_points",
                   "vehicle_journeys"]
        for collection in collecs:
            self.add_resource(getattr(Uri, collection)(True),
                              '/' + collection,
                              endpoint=collection + '.external_codes')

        self.add_resource(Places.Places,
                          region + 'places',
                          coord + 'places',
                           '/places',
                          endpoint='places')
        self.add_resource(Ptobjects.Ptobjects,
                          region + 'pt_objects',
                          coord + 'pt_objects',
                          endpoint='pt_objects')

        self.add_resource(Places.PlaceUri,
                          region + 'places/<id:id>',
                          coord + 'places/<id:id>',
                          endpoint='place_uri')

        self.add_resource(Places.PlacesNearby,
                          region + 'places_nearby',
                          coord + 'places_nearby',
                          region + '<uri:uri>/places_nearby',
                          coord + '<uri:uri>/places_nearby',
                          endpoint='places_nearby')

        self.add_resource(Journeys.Journeys,
                          region + '<uri:uri>/journeys',
                          coord + '<uri:uri>/journeys',
                          region + 'journeys',
                          coord + 'journeys',
                          '/journeys',
                          endpoint='journeys')

        self.add_resource(Schedules.RouteSchedules,
                          region + '<uri:uri>/route_schedules',
                          coord + '<uri:uri>/route_schedules',
                          '/route_schedules',
                          endpoint='route_schedules')

        self.add_resource(Schedules.NextArrivals,
                          region + '<uri:uri>/arrivals',
                          coord + '<uri:uri>/arrivals',
                          region + 'arrivals',
                          coord + 'arrivals',
                          endpoint='arrivals')

        self.add_resource(Schedules.NextDepartures,
                          region + '<uri:uri>/departures',
                          coord + '<uri:uri>/departures',
                          region + 'departures',
                          coord + 'departures',
                          endpoint='departures')

        self.add_resource(Schedules.StopSchedules,
                          region + '<uri:uri>/stop_schedules',
                          coord + '<uri:uri>/stop_schedules',
                          '/stop_schedules',
                          endpoint='stop_schedules')

        self.add_resource(Disruptions.TrafficReport,
                          region + 'traffic_reports',
                          region + '<uri:uri>/traffic_reports',
                          endpoint='traffic_reports')

        self.add_resource(Status.Status,
                          region + 'status',
                          endpoint='status')

        self.add_resource(Calendars.Calendars,
                          region + 'calendars',
                          region + '<uri:uri>/calendars',
                          region + "calendars/<id:id>",
                          endpoint="calendars")
