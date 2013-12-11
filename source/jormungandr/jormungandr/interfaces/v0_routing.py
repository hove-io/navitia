from v0 import Regions, Ptref, Places, PlacesNearby, Journeys, TimeTables
from werkzeug.routing import BaseConverter


class RegionConverter(BaseConverter):

    """ The region you want to query"""

    def __init__(self, *args, **kwargs):
        BaseConverter.__init__(self, *args, **kwargs)
        self.type_ = "string"


def v0_routing(api):
    api.app.url_map.converters['region'] = RegionConverter
    api.add_resource(Regions.Regions,
                     '/v0/regions.json',
                     endpoint='v0')  # it just redirect to regions
    api.add_resource(Regions.Regions,
                     '/v0/regions.json',
                     endpoint='v0.regions')
    api.add_resource(Ptref.StopAreas,
                     '/v0/<region:region>/stop_areas.json',
                     endpoint='v0.stop_areas')
    api.add_resource(Ptref.Networks,
                     '/v0/<region:region>/networks.json',
                     endpoint='v0.networks')
    api.add_resource(Ptref.StopPoints,
                     '/v0/<region:region>/stop_points.json',
                     endpoint='v0.stop_points')
    api.add_resource(Ptref.Lines,
                     '/v0/<region:region>/lines.json',
                     endpoint='v0.lines')
    api.add_resource(Ptref.Routes,
                     '/v0/<region:region>/routes.json',
                     endpoint='v0.routes')
    api.add_resource(Ptref.PhysicalModes,
                     '/v0/<region:region>/physical_modes.json',
                     endpoint='v0.physical_modes')
    api.add_resource(Ptref.CommercialModes,
                     '/v0/<region:region>/commercial_modes.json',
                     endpoint='v0.commercial_modes')
    api.add_resource(Ptref.Connections,
                     '/v0/<region:region>/connections.json',
                     endpoint='v0.connections')
    api.add_resource(Ptref.JourneyPatternPoints,
                     '/v0/<region:region>/journey_pattern_points.json',
                     endpoint='v0.journey_pattern_points')
    api.add_resource(Ptref.JourneyPatterns,
                     '/v0/<region:region>/journey_patterns.json',
                     endpoint='v0.journey_patterns')
    api.add_resource(Ptref.Companies,
                     '/v0/<region:region>/companies.json',
                     endpoint='v0.companies')
    api.add_resource(Ptref.VehicleJourneys,
                     '/v0/<region:region>/vehicle_journeys.json',
                     endpoint='v0.vehicle_journeys')
    api.add_resource(Ptref.Pois,
                     '/v0/<region:region>/pois.json',
                     endpoint='v0.pois')
    api.add_resource(Ptref.PoiTypes,
                     '/v0/<region:region>/poi_types.json',
                     endpoint='v0.poi_types')
    api.add_resource(Places.Places,
                     '/v0/<region:region>/places.json',
                     endpoint='v0.places')
    api.add_resource(PlacesNearby.PlacesNearby,
                     '/v0/<region:region>/places_nearby.json',
                     endpoint='v0.places_nearby')
    api.add_resource(Journeys.Journeys,
                     '/v0/<region:region>/journeys.json',
                     '/v0/journeys.json',
                     endpoint='v0.journeys')
    api.add_resource(Journeys.Isochrone,
                     '/v0/<region:region>/isochrone.json',
                     '/v0/isochrone.json',
                     endpoint='v0.isochrone')
    api.add_resource(TimeTables.NextDepartures,
                     '/v0/<region:region>/next_departures.json',
                     endpoint='v0.next_departures')
    api.add_resource(TimeTables.NextArrivals,
                     '/v0/<region:region>/next_arrivals.json',
                     endpoint='v0.next_arrivals')
    api.add_resource(TimeTables.DepartureBoards,
                     '/v0/<region:region>/departure_boards.json',
                     endpoint='v0.departure_boards')
    api.add_resource(TimeTables.RouteSchedules,
                     '/v0/<region:region>/route_schedules.json',
                     endpoint='v0.route_schedules')
    api.add_resource(TimeTables.StopsSchedules,
                     '/v0/<region:region>/stops_schedules.json',
                     endpoint='v0.stops_schedules')
