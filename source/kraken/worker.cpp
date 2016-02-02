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
#include "ptreferential/ptreferential.h"
#include "ptreferential/ptreferential_api.h"
#include "time_tables/route_schedules.h"
#include "time_tables/next_passages.h"
#include "time_tables/previous_passages.h"
#include "time_tables/departure_boards.h"
#include "disruption/traffic_reports_api.h"
#include "calendar/calendar_api.h"
#include "routing/raptor.h"
#include "type/meta_data.h"

namespace nt = navitia::type;
namespace pt = boost::posix_time;
namespace bg = boost::gregorian;

namespace navitia {

static nt::Type_e get_type(pbnavitia::NavitiaType pb_type) {
    switch(pb_type){
    case pbnavitia::ADDRESS: return nt::Type_e::Address;
    case pbnavitia::STOP_AREA: return nt::Type_e::StopArea;
    case pbnavitia::STOP_POINT: return nt::Type_e::StopPoint;
    case pbnavitia::LINE: return nt::Type_e::Line;
    case pbnavitia::LINE_GROUP: return nt::Type_e::LineGroup;
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
    case pbnavitia::IMPACT: return nt::Type_e::Impact;
    case pbnavitia::TRIP: return nt::Type_e::MetaVehicleJourney;
    case pbnavitia::CONTRIBUTOR: return nt::Type_e::Contributor;
    case pbnavitia::FRAME: return nt::Type_e::Frame;
    default: return nt::Type_e::Unknown;
    }
}

static nt::OdtLevel_e get_odt_level(pbnavitia::OdtLevel pb_odt_level) {
    switch(pb_odt_level){
        case pbnavitia::OdtLevel::with_stops: return nt::OdtLevel_e::with_stops;
        case pbnavitia::OdtLevel::zonal: return nt::OdtLevel_e::zonal;
        case pbnavitia::OdtLevel::all: return nt::OdtLevel_e::all;
        default: return nt::OdtLevel_e::scheduled;
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

static std::string get_string_status(const boost::shared_ptr<const nt::Data>& data) {
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
    status->set_navitia_version(config::project_version);
    status->set_loaded(d->loaded);
    status->set_last_load_status(d->last_load);
    status->set_last_load_at(pt::to_iso_string(d->last_load_at));
    status->set_last_rt_data_loaded(pt::to_iso_string(d->last_rt_data_loaded));
    status->set_nb_threads(conf.nb_thread());
    status->set_is_connected_to_rabbitmq(d->is_connected_to_rabbitmq);
    status->set_status(get_string_status(d));
    status->set_is_realtime_loaded(d->is_realtime_loaded);
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

void Worker::metadatas(pbnavitia::Response& response) {
    auto metadatas = response.mutable_metadatas();
    const auto d = data_manager.get_data();
    if (d->loaded) {
        metadatas->set_start_production_date(bg::to_iso_string(d->meta->production_date.begin()));
        metadatas->set_end_production_date(bg::to_iso_string(d->meta->production_date.last()));
        metadatas->set_shape(d->meta->shape);
        // we get the first timezone of the dataset
        const auto* tz = d->pt_data->tz_manager.get_first_timezone();
        if (tz) {
            metadatas->set_timezone(tz->tz_name);
        }
        for(const type::Contributor* contributor : d->pt_data->contributors) {
            metadatas->add_contributors(contributor->uri);
        }
        if (!d->meta->publisher_name.empty()){
            metadatas->set_name(d->meta->publisher_name);
        }
        metadatas->set_last_load_at(navitia::to_posix_timestamp(d->last_load_at));
    } else {
        metadatas->set_start_production_date("");
        metadatas->set_end_production_date("");
        metadatas->set_shape("");
        metadatas->set_timezone("");
    }
    metadatas->set_status(get_string_status(d));
}

void Worker::feed_publisher(pbnavitia::Response& response){
    const auto d = data_manager.get_data();
    auto pb_feed_publisher = response.add_feed_publishers();
    // instance_name is required
    pb_feed_publisher->set_id(d->meta->instance_name);
    if (!d->meta->publisher_name.empty()){
        pb_feed_publisher->set_name(d->meta->publisher_name);
    }
    if (!d->meta->publisher_url.empty()){
        pb_feed_publisher->set_url(d->meta->publisher_url);
    }
    if (!d->meta->license.empty()){
        pb_feed_publisher->set_license(d->meta->license);
    }
}

void Worker::init_worker_data(const boost::shared_ptr<const navitia::type::Data> data){
    //@TODO should be done in data_manager
    if(data->data_identifier != this->last_data_identifier || !planner){
        planner = std::unique_ptr<routing::RAPTOR>(new routing::RAPTOR(*data));
        street_network_worker = std::unique_ptr<georef::StreetNetwork>(new georef::StreetNetwork(*data->geo_ref));
        this->last_data_identifier = data->data_identifier;

        LOG4CPLUS_INFO(logger, "Instanciate planner");
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

pbnavitia::Response Worker::traffic_reports(const pbnavitia::TrafficReportsRequest &request){
    const auto data = data_manager.get_data();
    std::vector<std::string> forbidden_uris;
    for(int i = 0; i < request.forbidden_uris_size(); ++i)
        forbidden_uris.push_back(request.forbidden_uris(i));
    return navitia::disruption::traffic_reports(*data,
                                                request._current_datetime(),
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

pbnavitia::Response Worker::next_stop_times(const pbnavitia::NextStopTimeRequest& request,
        pbnavitia::API api) {
    const auto data = data_manager.get_data();
    int32_t max_date_times = request.has_max_date_times() ? request.max_date_times() : std::numeric_limits<int>::max();
    std::vector<std::string> forbidden_uri;
    for(int i = 0; i < request.forbidden_uri_size(); ++i)
        forbidden_uri.push_back(request.forbidden_uri(i));
    this->init_worker_data(data);

    bt::ptime from_datetime = bt::from_time_t(request.from_datetime());
    bt::ptime until_datetime = bt::from_time_t(request.until_datetime());
    bt::ptime current_datetime = bt::from_time_t(request._current_datetime());
    try {
        switch(api) {
        case pbnavitia::NEXT_DEPARTURES:
            return timetables::next_departures(request.departure_filter(),
                    forbidden_uri, from_datetime,
                    request.duration(), request.nb_stoptimes(), request.depth(),
                    type::AccessibiliteParams(), *data, type::RTLevel::Adapted, request.count(),
                    request.start_page(), request.show_codes(), current_datetime);
        case pbnavitia::NEXT_ARRIVALS:
            return timetables::next_arrivals(request.arrival_filter(),
                    forbidden_uri, from_datetime,
                    request.duration(), request.nb_stoptimes(), request.depth(),
                    type::AccessibiliteParams(), *data, type::RTLevel::Adapted, request.count(),
                    request.start_page(), request.show_codes(), current_datetime);
        case pbnavitia::PREVIOUS_DEPARTURES:
            return timetables::previous_departures(request.departure_filter(),
                    forbidden_uri, until_datetime,
                    request.duration(), request.nb_stoptimes(), request.depth(),
                    type::AccessibiliteParams(), *data, type::RTLevel::Adapted, request.count(),
                    request.start_page(), request.show_codes(), current_datetime);
        case pbnavitia::PREVIOUS_ARRIVALS:
            return timetables::previous_arrivals(request.arrival_filter(),
                    forbidden_uri, until_datetime,
                    request.duration(), request.nb_stoptimes(), request.depth(),
                    type::AccessibiliteParams(), *data, type::RTLevel::Adapted, request.count(),
                    request.start_page(), request.show_codes(), current_datetime);
        case pbnavitia::DEPARTURE_BOARDS:
            return timetables::departure_board(request.departure_filter(),
                    request.has_calendar() ? boost::optional<const std::string>(request.calendar()) :
                                             boost::optional<const std::string>(),
                    forbidden_uri, from_datetime,
                    request.duration(),
                    request.depth(), max_date_times, request.interface_version(),
                    request.count(), request.start_page(), *data, type::RTLevel::Base, request.show_codes());
        case pbnavitia::ROUTE_SCHEDULES:
            return timetables::route_schedule(request.departure_filter(),
                    request.has_calendar() ? boost::optional<const std::string>(request.calendar()) :
                    boost::optional<const std::string>(),
                    forbidden_uri,
                    from_datetime,
                    request.duration(), max_date_times, request.depth(),
                    request.count(), request.start_page(), *data, type::RTLevel::Base, request.show_codes());
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
            max_non_pt = request.max_bike_duration_to_pt();
            break;
        case type::Mode_e::Car:
            result.offset = data->geo_ref->offsets[type::Mode_e::Car];
            result.speed_factor = request.car_speed() / georef::default_speed[type::Mode_e::Car];
            max_non_pt = request.max_car_duration_to_pt();
            break;
        case type::Mode_e::Bss:
            result.offset = data->geo_ref->offsets[type::Mode_e::Bss];
            result.speed_factor = request.bss_speed() / georef::default_speed[type::Mode_e::Bss];
            max_non_pt = request.max_bss_duration_to_pt();
            break;
        default:
            result.offset = data->geo_ref->offsets[type::Mode_e::Walking];
            result.speed_factor = request.walking_speed() / georef::default_speed[type::Mode_e::Walking];
            max_non_pt = request.max_walking_duration_to_pt();
            break;
    }
    if (result.speed_factor <= 0) { throw navitia::recoverable_exception("invalid speed factor"); }
    result.max_duration = navitia::seconds(max_non_pt);
    result.enable_direct_path = request.enable_direct_path();
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
        navitia::fill_pb_object(it_sa->second, *data, place, 1);
    } else {
        auto it_sp = data->pt_data->stop_points_map.find(request.uri());
        if(it_sp != data->pt_data->stop_points_map.end()) {
            pbnavitia::PtObject* place = pb_response.add_places();
            navitia::fill_pb_object(it_sp->second, *data, place, 1);
        } else {
            auto it_poi = data->geo_ref->poi_map.find(request.uri());
            if(it_poi != data->geo_ref->poi_map.end()) {
                pbnavitia::PtObject* place = pb_response.add_places();
                navitia::fill_pb_object(data->geo_ref->pois[it_poi->second], *data,place, 1);
            } else {
                auto it_admin = data->geo_ref->admin_map.find(request.uri());
                if(it_admin != data->geo_ref->admin_map.end()) {
                    pbnavitia::PtObject* place = pb_response.add_places();
                    navitia::fill_pb_object(data->geo_ref->admins[it_admin->second], *data, place, 1);

                }else{
                    fill_pb_error(pbnavitia::Error::unable_to_parse, "Unable to parse : "+request.uri(),  pb_response.mutable_error());
                }
            }
        }
    }
    return pb_response;
}

template<typename T>
static void fill_or_error(const pbnavitia::PlaceCodeRequest &request,
                          pbnavitia::Response& pb_response,
                          const type::Data& data) {
    const auto& objs = data.pt_data->codes.get_objs<T>(request.type_code(), request.code());
    if (objs.empty()) {
        fill_pb_error(pbnavitia::Error::unknown_object, "Unknow object", pb_response.mutable_error());
    } else {
        // FIXME: add every object or (as before) just the first one?
        navitia::fill_pb_object(objs.front(), data, pb_response.add_places());
    }
}

pbnavitia::Response Worker::place_code(const pbnavitia::PlaceCodeRequest &request) {
    const auto data = data_manager.get_data();
    this->init_worker_data(data);
    pbnavitia::Response pb_response;

    switch(request.type()) {
    case pbnavitia::PlaceCodeRequest::StopArea:
        fill_or_error<nt::StopArea>(request, pb_response, *data);
        break;
    case pbnavitia::PlaceCodeRequest::Network:
        fill_or_error<nt::Network>(request, pb_response, *data);
        break;
    case pbnavitia::PlaceCodeRequest::Company:
        fill_or_error<nt::Company>(request, pb_response, *data);
        break;
    case pbnavitia::PlaceCodeRequest::Line:
        fill_or_error<nt::Line>(request, pb_response, *data);
        break;
    case pbnavitia::PlaceCodeRequest::Route:
        fill_or_error<nt::Line>(request, pb_response, *data);
        break;
    case pbnavitia::PlaceCodeRequest::VehicleJourney:
        fill_or_error<nt::VehicleJourney>(request, pb_response, *data);
        break;
    case pbnavitia::PlaceCodeRequest::StopPoint:
        fill_or_error<nt::StopPoint>(request, pb_response, *data);
        break;
    case pbnavitia::PlaceCodeRequest::Calendar:
        fill_or_error<nt::Calendar>(request, pb_response, *data);
        break;
    }

    return pb_response;
}

static type::RTLevel get_realtime_level(const pbnavitia::JourneysRequest& request) {
    // for retrocompatibility, we must handle both the new and the old way of setting the rt_level
    // retrocompatibility will be droped after the migration
    if (request.has_realtime_level()) {
        switch (request.realtime_level()) {
        case pbnavitia::RTLevel::BASE:
            return type::RTLevel::Base;
        case pbnavitia::RTLevel::ADAPTED:
            return type::RTLevel::Adapted;
        case pbnavitia::RTLevel::REAL_TIME:
            return type::RTLevel::RealTime;
        default:
            throw navitia::exception("unhandled realtime level");
        }
    }
    return type::RTLevel::Base;
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

    if (origins.empty() && destinations.empty()) {
        //should never happen, jormungandr filters that, but it never hurts to double check
        pbnavitia::Response response;
        fill_pb_error(pbnavitia::Error::no_origin_nor_destination, "no origin point nor destination point given", response.mutable_error());
        response.set_response_type(pbnavitia::NO_ORIGIN_NOR_DESTINATION_POINT);
        return response;
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
        if ((origin.type == type::Type_e::Address)
                || (origin.type == type::Type_e::Coord) || (origin.type == type::Type_e::Admin)
                || (origin.type == type::Type_e::POI) || (origin.type == type::Type_e::StopArea)) {
            origin.streetnetwork_params = this->streetnetwork_params_of_entry_point(
                        request.streetnetwork_params(), data);
        }
    }
    /// params for arrival fallback
    for(size_t i = 0; i < destinations.size(); i++) {
        type::EntryPoint &destination = destinations[i];
        if ((destination.type == type::Type_e::Address) || (destination.type == type::Type_e::Coord)
                || (destination.type == type::Type_e::Admin) || (destination.type == type::Type_e::POI)
                || (destination.type == type::Type_e::StopArea)) {
            destination.streetnetwork_params = this->streetnetwork_params_of_entry_point(
                        request.streetnetwork_params(), data, false);
        }
    }

    /// Accessibility params
    type::AccessibiliteParams accessibilite_params;
    accessibilite_params.properties.set(type::hasProperties::WHEELCHAIR_BOARDING, request.wheelchair());
    accessibilite_params.vehicle_properties.set(type::hasVehicleProperties::WHEELCHAIR_ACCESSIBLE, request.wheelchair());

    const auto rt_level = get_realtime_level(request);

    switch(api) {
    case pbnavitia::ISOCHRONE: {
        type::EntryPoint ep;
        if (! origins.empty()) {
            if (! request.clockwise()) {
                // isochrone works only on clockwise
                pbnavitia::Response response;
                fill_pb_error(pbnavitia::Error::bad_format, "isochrone works only for clockwise request", response.mutable_error());
                response.set_response_type(pbnavitia::NO_SOLUTION);
                return response;
            }
            ep = origins[0];
        } else {
            if (request.clockwise()) {
                // isochrone works only on clockwise
                pbnavitia::Response response;
                fill_pb_error(pbnavitia::Error::bad_format, "reverse isochrone works only for anti-clockwise request", response.mutable_error());
                response.set_response_type(pbnavitia::NO_SOLUTION);
                return response;
            }
            ep = destinations[0];
        }
        return navitia::routing::make_isochrone(*planner, ep, request.datetimes(0),
                                                request.clockwise(), accessibilite_params,
                                                forbidden, *street_network_worker,
                                                rt_level, request.max_duration(),
                                                request.max_transfers(), request.show_codes());
    }
    case pbnavitia::NMPLANNER:
        return routing::make_nm_response(*planner, origins, destinations, datetimes[0],
                request.clockwise(), accessibilite_params,
                forbidden, *street_network_worker,
                rt_level, request.max_duration(),
                request.max_transfers(), request.show_codes());

    case pbnavitia::pt_planner:
        return routing::make_pt_response(*planner, origins, destinations, datetimes[0],
                request.clockwise(), accessibilite_params,
                forbidden, rt_level, seconds{request.walking_transfer_penalty()}, request.max_duration(),
                request.max_transfers(), request.show_codes(), request.max_extra_second_pass());
    default:
        return routing::make_response(*planner, origins[0], destinations[0], datetimes,
                request.clockwise(), accessibilite_params,
                forbidden, *street_network_worker,
                rt_level, seconds{request.walking_transfer_penalty()}, request.max_duration(),
                request.max_transfers(), request.show_codes(), request.max_extra_second_pass());
    }
}

pbnavitia::Response Worker::pt_ref(const pbnavitia::PTRefRequest &request) {
    const auto data = data_manager.get_data();
    std::vector<std::string> forbidden_uri;
    for (int i = 0; i < request.forbidden_uri_size(); ++i) {
        forbidden_uri.push_back(request.forbidden_uri(i));
    }
    return navitia::ptref::query_pb(get_type(request.requested_type()),
                                    request.filter(),
                                    forbidden_uri,
                                    get_odt_level(request.odt_level()),
                                    request.depth(),
                                    request.show_codes(),
                                    request.start_page(),
                                    request.count(),
                                    boost::make_optional(request.has_since_datetime(),
                                                         bt::from_time_t(request.since_datetime())),
                                    boost::make_optional(request.has_until_datetime(),
                                                         bt::from_time_t(request.until_datetime())),
                                    *data,
                                    //no check on this datetime, it's
                                    //not important for it to be in
                                    //the production period, it's used
                                    //to filter the disruptions
                                    bt::from_time_t(request.datetime()));
}


pbnavitia::Response Worker::dispatch(const pbnavitia::Request& request) {
    pbnavitia::Response response ;
    // These api can respond even if the data isn't loaded
    if (request.requested_api() == pbnavitia::STATUS) {
        return status();
    }
    if (request.requested_api() ==  pbnavitia::METADATAS) {
        metadatas(response);
        return response;
    }
    if (! data_manager.get_data()->loaded){
        fill_pb_error(pbnavitia::Error::service_unavailable, "The service is loading data", response.mutable_error());
        return response;
    }
    switch(request.requested_api()){
        case pbnavitia::places: response = autocomplete(request.places()); break;
        case pbnavitia::pt_objects: response = pt_object(request.pt_objects()); break;
        case pbnavitia::place_uri: response = place_uri(request.place_uri()); break;
        case pbnavitia::ROUTE_SCHEDULES:
        case pbnavitia::NEXT_DEPARTURES:
        case pbnavitia::NEXT_ARRIVALS:
        case pbnavitia::PREVIOUS_DEPARTURES:
        case pbnavitia::PREVIOUS_ARRIVALS:
        case pbnavitia::DEPARTURE_BOARDS:
            response = next_stop_times(request.next_stop_times(), request.requested_api()); break;
        case pbnavitia::ISOCHRONE:
        case pbnavitia::NMPLANNER:
        case pbnavitia::pt_planner:
        case pbnavitia::PLANNER: response = journeys(request.journeys(), request.requested_api()); break;
        case pbnavitia::places_nearby: response = proximity_list(request.places_nearby()); break;
        case pbnavitia::PTREFERENTIAL: response = pt_ref(request.ptref()); break;
        case pbnavitia::traffic_reports : response = traffic_reports(request.traffic_reports()); break;
        case pbnavitia::calendars : response = calendars(request.calendars()); break;
        case pbnavitia::place_code : response = place_code(request.place_code()); break;
        case pbnavitia::nearest_stop_points : response = nearest_stop_points(request.nearest_stop_points()); break;
        default:
            LOG4CPLUS_WARN(logger, "Unknown API : " + API_Name(request.requested_api()));
            fill_pb_error(pbnavitia::Error::unknown_api, "Unknown API", response.mutable_error());
            break;
    }
    metadatas(response);//we add the metadatas for each response
    feed_publisher(response);
    return response;
}

pbnavitia::Response Worker::nearest_stop_points(const pbnavitia::NearestStopPointsRequest& request) {
    const auto data = data_manager.get_data();
    this->init_worker_data(data);

    //todo check the request

    type::EntryPoint entry_point;
    Type_e origin_type = data->get_type_of_id(request.place());
    entry_point = type::EntryPoint(origin_type, request.place(), 0);

    if (entry_point.type == type::Type_e::Address || entry_point.type == type::Type_e::Admin
            || entry_point.type == type::Type_e::StopArea || entry_point.type == type::Type_e::StopPoint
            || entry_point.type == type::Type_e::POI) {
        entry_point.coordinates = this->coord_of_entry_point(entry_point, data);
    }
    if ((entry_point.type == type::Type_e::Address)
            || (entry_point.type == type::Type_e::Coord) || (entry_point.type == type::Type_e::Admin)
            || (entry_point.type == type::Type_e::POI) || (entry_point.type == type::Type_e::StopArea)) {

        entry_point.streetnetwork_params.mode = type::static_data::get()->modeByCaption(request.mode());
        entry_point.streetnetwork_params.set_filter(request.filter());
    }
    switch(entry_point.streetnetwork_params.mode){
        case type::Mode_e::Bike:
            entry_point.streetnetwork_params.offset = data->geo_ref->offsets[type::Mode_e::Bike];
            entry_point.streetnetwork_params.speed_factor = request.bike_speed() / georef::default_speed[type::Mode_e::Bike];
            break;
        case type::Mode_e::Car:
            entry_point.streetnetwork_params.offset = data->geo_ref->offsets[type::Mode_e::Car];
            entry_point.streetnetwork_params.speed_factor = request.car_speed() / georef::default_speed[type::Mode_e::Car];
            break;
        case type::Mode_e::Bss:
            entry_point.streetnetwork_params.offset = data->geo_ref->offsets[type::Mode_e::Bss];
            entry_point.streetnetwork_params.speed_factor = request.bss_speed() / georef::default_speed[type::Mode_e::Bss];
            break;
        default:
            entry_point.streetnetwork_params.offset = data->geo_ref->offsets[type::Mode_e::Walking];
            entry_point.streetnetwork_params.speed_factor = request.walking_speed() / georef::default_speed[type::Mode_e::Walking];
            break;
    }
    if (entry_point.streetnetwork_params.speed_factor <= 0) {
        throw navitia::recoverable_exception("invalid speed factor");
    }
    entry_point.streetnetwork_params.max_duration = navitia::seconds(request.max_duration());
    street_network_worker->init(entry_point, {});
    //kraken don't handle reverse isochrone
    auto result = routing::get_stop_points(entry_point, *data, *street_network_worker, false);
    pbnavitia::Response response;
    for(const auto& item: result){
        auto* nsp = response.add_nearest_stop_points();
        navitia::fill_pb_object(planner->get_sp(item.first), *data, nsp->mutable_stop_point(), 0);
        nsp->set_access_duration(item.second.total_seconds());
    }
    return response;
}

}
