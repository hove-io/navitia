/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!

LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

Stay tuned using
twitter @navitia
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "worker.h"

#include "routing/raptor_api.h"
#include "autocomplete/autocomplete_api.h"
#include "proximity_list/proximitylist_api.h"
#include "ptreferential/ptreferential_api.h"
#include "time_tables/route_schedules.h"
#include "time_tables/next_passages.h"
#include "time_tables/2stops_schedules.h"
#include "time_tables/departure_boards.h"
#include "disruption/disruption_api.h"
#include "calendar/calendar_api.h"
#include "routing/raptor.h"
#include "type/meta_data.h"

namespace nt = navitia::type;
namespace pt = boost::posix_time;
namespace bg = boost::gregorian;

namespace navitia {

nt::Type_e get_type(pbnavitia::NavitiaType pb_type){
    switch(pb_type){
    case pbnavitia::ADDRESS: return nt::Type_e::Address;
    case pbnavitia::STOP_AREA: return nt::Type_e::StopArea;
    case pbnavitia::STOP_POINT: return nt::Type_e::StopPoint;
    case pbnavitia::LINE: return nt::Type_e::Line;
    case pbnavitia::ROUTE: return nt::Type_e::Route;
    case pbnavitia::JOURNEY_PATTERN: return nt::Type_e::JourneyPattern;
    case pbnavitia::NETWORK: return nt::Type_e::Network;
    case pbnavitia::COMMERCIAL_MODE: return nt::Type_e::CommercialMode;
    case pbnavitia::PHYSICAL_MODE: return nt::Type_e::PhysicalMode;
    case pbnavitia::CONNECTION: return nt::Type_e::Connection;
    case pbnavitia::JOURNEY_PATTERN_POINT: return nt::Type_e::JourneyPatternPoint;
    case pbnavitia::COMPANY: return nt::Type_e::Company;
    case pbnavitia::VEHICLE_JOURNEY: return nt::Type_e::VehicleJourney;
    case pbnavitia::POI: return nt::Type_e::POI;
    case pbnavitia::POITYPE: return nt::Type_e::POIType;
    case pbnavitia::ADMINISTRATIVE_REGION: return nt::Type_e::Admin;
    case pbnavitia::CALENDAR: return nt::Type_e::Calendar;
    default: return nt::Type_e::Unknown;
    }
}

nt::OdtLevel_e get_odt_level(pbnavitia::OdtLevel pb_odt_level) {
    switch(pb_odt_level){
        case pbnavitia::OdtLevel::mixt: return nt::OdtLevel_e::mixt;
        case pbnavitia::OdtLevel::zonal: return nt::OdtLevel_e::zonal;
        case pbnavitia::OdtLevel::all: return nt::OdtLevel_e::all;
        default: return nt::OdtLevel_e::none;
    }
}

template<class T>
std::vector<nt::Type_e> vector_of_pb_types(const T & pb_object){
    std::vector<nt::Type_e> result;
    for(int i = 0; i < pb_object.types_size(); ++i){
        result.push_back(get_type(pb_object.types(i)));
    }
    return result;
}

template<class T>
std::vector<std::string> vector_of_admins(const T & admin){
    std::vector<std::string> result;
    for (int i = 0; i < admin.admin_uris_size(); ++i){
        result.push_back(admin.admin_uris(i));
    }
    return result;
}

Worker::Worker(DataManager<navitia::type::Data>& data_manager, kraken::Configuration conf) :
    data_manager(data_manager), conf(conf),
    logger(log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"))){}

Worker::~Worker(){}

std::string get_string_status(const boost::shared_ptr<const nt::Data>& data) {
    if (data->loaded) {
        return "running";
    }
    if (data->loading) {
        return "loading_data";
    }
    return "no_data";
}

pbnavitia::Response Worker::status() {
    pbnavitia::Response result;

    auto status = result.mutable_status();
    const auto d = data_manager.get_data();
    status->set_data_version(d->version);
    status->set_navitia_version(config::kraken_version);
    status->set_loaded(d->loaded);
    status->set_last_load_status(d->last_load);
    status->set_last_load_at(pt::to_iso_string(d->last_load_at));
    status->set_nb_threads(conf.nb_thread());
    status->set_is_connected_to_rabbitmq(d->is_connected_to_rabbitmq);
    status->set_status(get_string_status(d));
    if (d->loaded) {
        status->set_publication_date(pt::to_iso_string(d->meta->publication_date));
        status->set_start_production_date(bg::to_iso_string(d->meta->production_date.begin()));
        status->set_end_production_date(bg::to_iso_string(d->meta->production_date.last()));
        for(auto data_sources: d->meta->data_sources){
            status->add_data_sources(data_sources);
        }
    } else {
        status->set_publication_date("");
        status->set_start_production_date("");
        status->set_end_production_date("");
    }
    return result;
}

pbnavitia::Response Worker::metadatas() {
    pbnavitia::Response result;
    auto metadatas = result.mutable_metadatas();
    const auto d = data_manager.get_data();
    if (d->loaded) {
        metadatas->set_start_production_date(bg::to_iso_string(d->meta->production_date.begin()));
        metadatas->set_end_production_date(bg::to_iso_string(d->meta->production_date.last()));
        metadatas->set_shape(d->meta->shape);
        metadatas->set_timezone(d->meta->timezone);
        for(const type::Contributor* contributor : d->pt_data->contributors) {
            metadatas->add_contributors(contributor->uri);
        }
    } else {
        metadatas->set_start_production_date("");
        metadatas->set_end_production_date("");
        metadatas->set_shape("");
        metadatas->set_timezone("");
    }
    metadatas->set_status(get_string_status(d));
    return result;
}

void Worker::init_worker_data(const boost::shared_ptr<const navitia::type::Data> data){
    //@TODO should be done in data_manager
    if(&*data != this->last_data || !planner){
        planner = std::unique_ptr<routing::RAPTOR>(new routing::RAPTOR(*data));
        street_network_worker = std::unique_ptr<georef::StreetNetwork>(new georef::StreetNetwork(*data->geo_ref));
        this->last_data = &*data;

        LOG4CPLUS_INFO(logger, "instanciation du planner");
    }
}


pbnavitia::Response Worker::autocomplete(const pbnavitia::PlacesRequest & request) {
    const auto data = data_manager.get_data();
    return navitia::autocomplete::autocomplete(request.q(),
            vector_of_pb_types(request), request.depth(), request.count(),
            vector_of_admins(request), request.search_type(), *data);
}

pbnavitia::Response Worker::pt_object(const pbnavitia::PtobjectRequest & request) {
    const auto data = data_manager.get_data();
    return navitia::autocomplete::autocomplete(request.q(),
            vector_of_pb_types(request), request.depth(), request.count(),
            vector_of_admins(request), request.search_type(), *data);
}

pbnavitia::Response Worker::disruptions(const pbnavitia::DisruptionsRequest &request){
    const auto data = data_manager.get_data();
    std::vector<std::string> forbidden_uris;
    for(int i = 0; i < request.forbidden_uris_size(); ++i)
        forbidden_uris.push_back(request.forbidden_uris(i));
    return navitia::disruption::disruptions(*data,
                                                request.datetime(),
                                                request.period(),
                                                request.depth(),
                                                request.count(),
                                                request.start_page(),
                                                request.filter(),
                                                forbidden_uris);
}

pbnavitia::Response Worker::calendars(const pbnavitia::CalendarsRequest &request){
    const auto data = data_manager.get_data();
    std::vector<std::string> forbidden_uris;
    for(int i = 0; i < request.forbidden_uris_size(); ++i)
        forbidden_uris.push_back(request.forbidden_uris(i));
    return navitia::calendar::calendars(*data,
                                        request.start_date(),
                                        request.end_date(),
                                        request.depth(),
                                        request.count(),
                                        request.start_page(),
                                        request.filter(),
                                        forbidden_uris);
}

pbnavitia::Response Worker::next_stop_times(const pbnavitia::NextStopTimeRequest &request,
        pbnavitia::API api) {
    const auto data = data_manager.get_data();
    int32_t max_date_times = request.has_max_date_times() ? request.max_date_times() : std::numeric_limits<int>::max();
    std::vector<std::string> forbidden_uri;
    for(int i = 0; i < request.forbidden_uri_size(); ++i)
        forbidden_uri.push_back(request.forbidden_uri(i));
    this->init_worker_data(data);

    bt::ptime from_datetime = bt::from_time_t(request.from_datetime());
    try {
        switch(api) {
        case pbnavitia::NEXT_DEPARTURES:
            return timetables::next_departures(request.departure_filter(),
                    forbidden_uri, from_datetime,
                    request.duration(), request.nb_stoptimes(), request.depth(),
                    type::AccessibiliteParams(), *data, false, request.count(),
                    request.start_page(), request.show_codes());
        case pbnavitia::NEXT_ARRIVALS:
            return timetables::next_arrivals(request.arrival_filter(),
                    forbidden_uri, from_datetime,
                    request.duration(), request.nb_stoptimes(), request.depth(),
                    type::AccessibiliteParams(),
                    *data, false, request.count(), request.start_page(), request.show_codes());
        case pbnavitia::STOPS_SCHEDULES:
            return timetables::stops_schedule(request.departure_filter(),
                                              request.arrival_filter(),
                                              forbidden_uri,
                    from_datetime, request.duration(), request.depth(),
                    *data, false);
        case pbnavitia::DEPARTURE_BOARDS:
            return timetables::departure_board(request.departure_filter(),
                    request.has_calendar() ? boost::optional<const std::string>(request.calendar()) :
                                             boost::optional<const std::string>(),
                    forbidden_uri, from_datetime,
                    request.duration(),
                    request.depth(), max_date_times, request.interface_version(),
                    request.count(), request.start_page(), *data, false, request.show_codes());
        case pbnavitia::ROUTE_SCHEDULES:
            return timetables::route_schedule(request.departure_filter(),
                    forbidden_uri, from_datetime,
                    request.duration(), request.interface_version(), request.depth(),
                    request.count(), request.start_page(), *data, false, request.show_codes());
        default:
            LOG4CPLUS_WARN(logger, "Unknown timetable query");
            pbnavitia::Response response;
            fill_pb_error(pbnavitia::Error::unknown_api, "Unknown time table api",
                    response.mutable_error());
            return response;
        }

    } catch (const navitia::ptref::parsing_error& error) {
        LOG4CPLUS_ERROR(logger, "Error in the ptref request  : "+ error.more);
        pbnavitia::Response response;
        const auto str_error = "Unknow filter : " + error.more;
        fill_pb_error(pbnavitia::Error::bad_filter, str_error, response.mutable_error());
        return response;
    }
}


pbnavitia::Response Worker::proximity_list(const pbnavitia::PlacesNearbyRequest &request) {
    const auto data = data_manager.get_data();
    type::EntryPoint ep(data->get_type_of_id(request.uri()), request.uri());
    auto coord = this->coord_of_entry_point(ep, data);
    return proximitylist::find(coord, request.distance(), vector_of_pb_types(request),
                request.filter(), request.depth(), request.count(),
                request.start_page(), *data);
}


type::GeographicalCoord Worker::coord_of_entry_point(
        const type::EntryPoint & entry_point,
        const boost::shared_ptr<const navitia::type::Data> data) {
    type::GeographicalCoord result;
    if(entry_point.type == Type_e::Address){
        auto way = data->geo_ref->way_map.find(entry_point.uri);
        if (way != data->geo_ref->way_map.end()){
            const auto geo_way = data->geo_ref->ways[way->second];
            result = geo_way->nearest_coord(entry_point.house_number, data->geo_ref->graph);
        }
    } else if (entry_point.type == Type_e::StopPoint) {
        auto sp_it = data->pt_data->stop_points_map.find(entry_point.uri);
        if(sp_it != data->pt_data->stop_points_map.end()) {
            result = sp_it->second->coord;
        }
    } else if (entry_point.type == Type_e::StopArea) {
           auto sa_it = data->pt_data->stop_areas_map.find(entry_point.uri);
           if(sa_it != data->pt_data->stop_areas_map.end()) {
               result = sa_it->second->coord;
           }
    } else if (entry_point.type == Type_e::Coord) {
        result = entry_point.coordinates;
    } else if (entry_point.type == Type_e::Admin) {
        auto it_admin = data->geo_ref->admin_map.find(entry_point.uri);
        if (it_admin != data->geo_ref->admin_map.end()) {
            const auto admin = data->geo_ref->admins[it_admin->second];
            result = admin->coord;
        }

    } else if(entry_point.type == Type_e::POI){
        auto poi = data->geo_ref->poi_map.find(entry_point.uri);
        if (poi != data->geo_ref->poi_map.end()){
            const auto geo_poi = data->geo_ref->pois[poi->second];
            result = geo_poi->coord;
        }
    }
    return result;
}


type::StreetNetworkParams Worker::streetnetwork_params_of_entry_point(const pbnavitia::StreetNetworkParams & request,
        const boost::shared_ptr<const navitia::type::Data> data,
        const bool use_second){
    type::StreetNetworkParams result;
    std::string uri;

    if(use_second){
        result.mode = type::static_data::get()->modeByCaption(request.origin_mode());
        result.set_filter(request.origin_filter());
    }else{
        result.mode = type::static_data::get()->modeByCaption(request.destination_mode());
        result.set_filter(request.destination_filter());
    }
    int max_non_pt;
    switch(result.mode){
        case type::Mode_e::Bike:
            result.offset = data->geo_ref->offsets[type::Mode_e::Bike];
            result.speed_factor = request.bike_speed() / georef::default_speed[type::Mode_e::Bike];
            if(request.has_max_bike_duration_to_pt()){
                max_non_pt = request.max_bike_duration_to_pt();
            }else{
                //we keep this field for compatibily with kraken 1.2, to be removed after the release of the 1.3
                max_non_pt = request.max_duration_to_pt();
            }
            break;
        case type::Mode_e::Car:
            result.offset = data->geo_ref->offsets[type::Mode_e::Car];
            result.speed_factor = request.car_speed() / georef::default_speed[type::Mode_e::Car];
            if(request.has_max_car_duration_to_pt()){
                max_non_pt = request.max_car_duration_to_pt();
            }else{
                //we keep this field for compatibily with kraken 1.2, to be removed after the release of the 1.3
                max_non_pt = request.max_duration_to_pt();
            }
            break;
        case type::Mode_e::Bss:
            result.offset = data->geo_ref->offsets[type::Mode_e::Bss];
            result.speed_factor = request.bss_speed() / georef::default_speed[type::Mode_e::Bss];
            if(request.has_max_bss_duration_to_pt()){
                max_non_pt = request.max_bss_duration_to_pt();
            }else{
                //we keep this field for compatibily with kraken 1.2, to be removed after the release of the 1.3
                max_non_pt = request.max_duration_to_pt();
            }
            break;
        default:
            result.offset = data->geo_ref->offsets[type::Mode_e::Walking];
            result.speed_factor = request.walking_speed() / georef::default_speed[type::Mode_e::Walking];
            if(request.has_max_walking_duration_to_pt()){
                max_non_pt = request.max_walking_duration_to_pt();
            }else{
                //we keep this field for compatibily with kraken 1.2, to be removed after the release of the 1.3
                max_non_pt = request.max_duration_to_pt();
            }
            break;
    }
    result.max_duration = navitia::seconds(max_non_pt);
    return result;
}


pbnavitia::Response Worker::place_uri(const pbnavitia::PlaceUriRequest &request) {
    const auto data = data_manager.get_data();
    this->init_worker_data(data);
    pbnavitia::Response pb_response;

    if(request.uri().size() > 6 && request.uri().substr(0, 6) == "coord:") {
        type::EntryPoint ep(type::Type_e::Coord, request.uri());
        auto coord = this->coord_of_entry_point(ep, data);
        auto tmp = proximitylist::find(coord, 100, {type::Type_e::Address}, "", 1, 1, 0, *data);
        if(tmp.places_nearby().size() == 1){
            auto place = pb_response.add_places();
            place->CopyFrom(tmp.places_nearby(0));
        }
        return pb_response;
    }
    auto it_sa = data->pt_data->stop_areas_map.find(request.uri());
    if(it_sa != data->pt_data->stop_areas_map.end()) {
        pbnavitia::PtObject* place = pb_response.add_places();
        fill_pb_placemark(it_sa->second, *data, place, 1);
    } else {
        auto it_sp = data->pt_data->stop_points_map.find(request.uri());
        if(it_sp != data->pt_data->stop_points_map.end()) {
            pbnavitia::PtObject* place = pb_response.add_places();
            fill_pb_placemark(it_sp->second, *data, place, 1);
        } else {
            auto it_poi = data->geo_ref->poi_map.find(request.uri());
            if(it_poi != data->geo_ref->poi_map.end()) {
                pbnavitia::PtObject* place = pb_response.add_places();
                fill_pb_placemark(data->geo_ref->pois[it_poi->second], *data,place, 1);
            } else {
                auto it_admin = data->geo_ref->admin_map.find(request.uri());
                if(it_admin != data->geo_ref->admin_map.end()) {
                    pbnavitia::PtObject* place = pb_response.add_places();
                    fill_pb_placemark(data->geo_ref->admins[it_admin->second],*data, place, 1);

                }else{
                    fill_pb_error(pbnavitia::Error::unable_to_parse, "Unable to parse : "+request.uri(),  pb_response.mutable_error());
                }
            }
        }
    }
    return pb_response;
}

template<typename T>
void fill_or_error(const std::string& uri, const T& map, pbnavitia::Response& pb_response,
        const type::Data& data) {
    auto it = map.find(uri);
    if (it == map.end()) {
        fill_pb_error(pbnavitia::Error::unknown_object, "Unknow object", pb_response.mutable_error());
    } else {
        fill_pb_placemark(it->second, data, pb_response.add_places());
    }
}

pbnavitia::Response Worker::place_code(const pbnavitia::PlaceCodeRequest &request) {
    const auto data = data_manager.get_data();
    this->init_worker_data(data);
    pbnavitia::Response pb_response;

    const auto& ext_codes_map = data->pt_data->ext_codes_map[request.type()];
    const auto codes_map = ext_codes_map.find(request.type_code());
    if (codes_map == ext_codes_map.end()) {
        fill_pb_error(pbnavitia::Error::unknown_object, "Unknow object", pb_response.mutable_error());
        return pb_response;
    }
    const auto uri_it = codes_map->second.find(request.code());
    if (uri_it == codes_map->second.end()) {
        fill_pb_error(pbnavitia::Error::unknown_object, "Unknow object", pb_response.mutable_error());
        return pb_response;
    }
    switch(request.type()) {
        case pbnavitia::PlaceCodeRequest::StopArea:
            fill_or_error(uri_it->second, data->pt_data->stop_areas_map, pb_response, *data);
            break;
        case pbnavitia::PlaceCodeRequest::Network:
            fill_or_error(uri_it->second, data->pt_data->networks_map, pb_response, *data);
            break;
        case pbnavitia::PlaceCodeRequest::Company:
            fill_or_error(uri_it->second, data->pt_data->companies_map, pb_response, *data);
            break;
        case pbnavitia::PlaceCodeRequest::Line:
            fill_or_error(uri_it->second, data->pt_data->lines_map, pb_response, *data);
            break;
        case pbnavitia::PlaceCodeRequest::Route:
            fill_or_error(uri_it->second, data->pt_data->routes_map, pb_response, *data);
            break;
        case pbnavitia::PlaceCodeRequest::VehicleJourney:
            fill_or_error(uri_it->second, data->pt_data->vehicle_journeys_map, pb_response, *data);
            break;
        case pbnavitia::PlaceCodeRequest::StopPoint:
            fill_or_error(uri_it->second, data->pt_data->stop_points_map, pb_response, *data);
            break;
        case pbnavitia::PlaceCodeRequest::Calendar:
            fill_or_error(uri_it->second, data->pt_data->calendars_map, pb_response, *data);
            break;
    }

    return pb_response;
}

pbnavitia::Response Worker::journeys(const pbnavitia::JourneysRequest &request, pbnavitia::API api) {
    const auto data = data_manager.get_data();
    this->init_worker_data(data);

    std::vector<type::EntryPoint> origins;
    for(int i = 0; i < request.origin().size(); i++) {
        Type_e origin_type = data->get_type_of_id(request.origin(i).place());
        type::EntryPoint origin = type::EntryPoint(origin_type, request.origin(i).place(), request.origin(i).access_duration());

        if (origin.type == type::Type_e::Address || origin.type == type::Type_e::Admin
                || origin.type == type::Type_e::StopArea || origin.type == type::Type_e::StopPoint
                || origin.type == type::Type_e::POI) {
            origin.coordinates = this->coord_of_entry_point(origin, data);
        }
        origins.push_back(origin);
    }

    std::vector<type::EntryPoint> destinations;
    if(api != pbnavitia::ISOCHRONE) {
        for (int i = 0; i < request.destination().size(); i++) {
            Type_e destination_type = data->get_type_of_id(request.destination(i).place());
            type::EntryPoint destination = type::EntryPoint(destination_type, request.destination(i).place(), request.destination(i).access_duration());

            if (destination.type == type::Type_e::Address || destination.type == type::Type_e::Admin
                    || destination.type == type::Type_e::StopArea || destination.type == type::Type_e::StopPoint
                    || destination.type == type::Type_e::POI) {
                destination.coordinates = this->coord_of_entry_point(destination, data);
            }
            destinations.push_back(destination);
        }
    }

    std::vector<std::string> forbidden;
    for(int i = 0; i < request.forbidden_uris_size(); ++i)
        forbidden.push_back(request.forbidden_uris(i));

    std::vector<uint64_t> datetimes;
    for(int i = 0; i < request.datetimes_size(); ++i)
        datetimes.push_back(request.datetimes(i));

    /// params for departure fallback
    for(size_t i = 0; i < origins.size(); i++) {
        type::EntryPoint &origin = origins[i];
        if ((origin.type == type::Type_e::Address) || (origin.type == type::Type_e::Coord)
                || (origin.type == type::Type_e::Admin) || (origin.type == type::Type_e::POI) || (origin.type == type::Type_e::StopArea)){
            origin.streetnetwork_params = this->streetnetwork_params_of_entry_point(request.streetnetwork_params(), data);
        }
    }
    /// params for arrival fallback
    for(size_t i = 0; i < destinations.size(); i++) {
        type::EntryPoint &destination = destinations[i];
        if ((destination.type == type::Type_e::Address) || (destination.type == type::Type_e::Coord)
                || (destination.type == type::Type_e::Admin) || (destination.type == type::Type_e::POI) || (destination.type == type::Type_e::StopArea)){
            destination.streetnetwork_params = this->streetnetwork_params_of_entry_point(request.streetnetwork_params(), data, false);
        }
    }
    /// Accessibilité, il faut initialiser ce paramètre
    //HOT FIX degueulasse
    type::AccessibiliteParams accessibilite_params;
    accessibilite_params.properties.set(type::hasProperties::WHEELCHAIR_BOARDING, request.wheelchair());
    switch(api) {
        case pbnavitia::ISOCHRONE:
            return navitia::routing::make_isochrone(*planner, origins[0], request.datetimes(0),
                request.clockwise(), accessibilite_params,
                forbidden, *street_network_worker,
                request.disruption_active(), request.allow_odt(), request.max_duration(),
                request.max_transfers(), request.show_codes());
        case pbnavitia::NMPLANNER:
            return routing::make_nm_response(*planner, origins, destinations, datetimes[0],
                request.clockwise(), accessibilite_params,
                forbidden, *street_network_worker,
                request.disruption_active(), request.allow_odt(), request.max_duration(),
                request.max_transfers(), request.show_codes());
        default:
            return routing::make_response(*planner, origins[0], destinations[0], datetimes,
                request.clockwise(), accessibilite_params,
                forbidden, *street_network_worker,
                request.disruption_active(), request.allow_odt(), request.max_duration(),
                request.max_transfers(), request.show_codes());
    }
}


pbnavitia::Response Worker::pt_ref(const pbnavitia::PTRefRequest &request){
    const auto data = data_manager.get_data();
    std::vector<std::string> forbidden_uri;
    for(int i = 0; i < request.forbidden_uri_size(); ++i)
        forbidden_uri.push_back(request.forbidden_uri(i));
    return navitia::ptref::query_pb(get_type(request.requested_type()),
                                    request.filter(), forbidden_uri,
                                    get_odt_level(request.odt_level()),
                                    request.depth(), request.show_codes(),
                                    request.start_page(),
                                    request.count(), *data);
}


pbnavitia::Response Worker::dispatch(const pbnavitia::Request& request) {
    pbnavitia::Response result ;
    // These api can respond even if the data isn't loaded
    if (request.requested_api() == pbnavitia::STATUS) {
        return status();
    }
    if (request.requested_api() ==  pbnavitia::METADATAS) {
        return metadatas();
    }
    if (! data_manager.get_data()->loaded){
        fill_pb_error(pbnavitia::Error::service_unavailable, "The service is loading data", result.mutable_error());
        return result;
    }
    switch(request.requested_api()){
        case pbnavitia::places: return autocomplete(request.places());
        case pbnavitia::pt_objects: return pt_object(request.pt_objects());
        case pbnavitia::place_uri: return place_uri(request.place_uri());
        case pbnavitia::ROUTE_SCHEDULES:
        case pbnavitia::NEXT_DEPARTURES:
        case pbnavitia::NEXT_ARRIVALS:
        case pbnavitia::STOPS_SCHEDULES:
        case pbnavitia::DEPARTURE_BOARDS:
            return next_stop_times(request.next_stop_times(), request.requested_api());
        case pbnavitia::ISOCHRONE:
        case pbnavitia::NMPLANNER:
        case pbnavitia::PLANNER: return journeys(request.journeys(), request.requested_api());
        case pbnavitia::places_nearby: return proximity_list(request.places_nearby());
        case pbnavitia::PTREFERENTIAL: return pt_ref(request.ptref());
        case pbnavitia::disruptions : return disruptions(request.disruptions());
        case pbnavitia::calendars : return calendars(request.calendars());
        case pbnavitia::place_code : return place_code(request.place_code());
        default:
            LOG4CPLUS_WARN(logger, "Unknown API : " + API_Name(request.requested_api()));
            fill_pb_error(pbnavitia::Error::unknown_api, "Unknown API", result.mutable_error());
            break;
    }

    return result;
}

}
