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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "worker.h"

#include "autocomplete/autocomplete_api.h"
#include "calendar/calendar_api.h"
#include "disruption/line_reports_api.h"
#include "disruption/traffic_reports_api.h"
#include "equipment/equipment_api.h"
#include "proximity_list/proximitylist_api.h"
#include "ptreferential/ptreferential.h"
#include "ptreferential/ptreferential_api.h"
#include "routing/raptor.h"
#include "routing/raptor_api.h"
#include "time_tables/departure_boards.h"
#include "time_tables/passages.h"
#include "time_tables/route_schedules.h"
#include "type/meta_data.h"

#include <numeric>
#include <utility>

namespace nt = navitia::type;
namespace pt = boost::posix_time;
namespace bg = boost::gregorian;

using navitia::type::Type_e;

static const double CO2_ESTIMATION_COEFF = 1.35;

namespace navitia {

namespace {
// local exception, only used in this file
struct coord_conversion_exception : public recoverable_exception {
    explicit coord_conversion_exception(const std::string& msg) : recoverable_exception(msg) {}

    coord_conversion_exception(const coord_conversion_exception&) = default;
    coord_conversion_exception& operator=(const coord_conversion_exception&) = default;

    coord_conversion_exception(coord_conversion_exception&&) = default;

    ~coord_conversion_exception() noexcept override = default;
};
}  // namespace

template <typename T>
double get_speed(const T& request, const type::Mode_e mode) {
    switch (mode) {
        case type::Mode_e::Bike:
            return request.bike_speed();
        case type::Mode_e::Car:
            return request.car_speed();
        case type::Mode_e::CarNoPark:
            return request.car_no_park_speed();
        case type::Mode_e::Bss:
            return request.bss_speed();
        default:
            return request.walking_speed();
    }
}

static type::GeographicalCoord coord_of_entry_point(const type::EntryPoint& entry_point,
                                                    const navitia::type::Data& data) {
    if (entry_point.type == Type_e::Address) {
        auto way = data.geo_ref->way_map.find(entry_point.uri);
        if (way != data.geo_ref->way_map.end()) {
            const auto geo_way = data.geo_ref->ways[way->second];
            return geo_way->nearest_coord(entry_point.house_number, data.geo_ref->graph);
        }
    } else if (entry_point.type == Type_e::StopPoint) {
        auto sp_it = data.pt_data->stop_points_map.find(entry_point.uri);
        if (sp_it != data.pt_data->stop_points_map.end()) {
            return sp_it->second->coord;
        }
    } else if (entry_point.type == Type_e::StopArea) {
        auto sa_it = data.pt_data->stop_areas_map.find(entry_point.uri);
        if (sa_it != data.pt_data->stop_areas_map.end()) {
            return sa_it->second->coord;
        }
    } else if (entry_point.type == Type_e::Coord) {
        return entry_point.coordinates;
    } else if (entry_point.type == Type_e::Admin) {
        auto it_admin = data.geo_ref->admin_map.find(entry_point.uri);
        if (it_admin != data.geo_ref->admin_map.end()) {
            const auto admin = data.geo_ref->admins[it_admin->second];
            return admin->coord;
        }

    } else if (entry_point.type == Type_e::POI) {
        auto poi = data.geo_ref->poi_map.find(entry_point.uri);
        if (poi != data.geo_ref->poi_map.end()) {
            return poi->second->coord;
        }
    }
    LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger")),
                    "The entry point: " << entry_point.uri << " is not valid");
    throw navitia::coord_conversion_exception{"The entry point: " + entry_point.uri + " is not valid"};
}

static nt::Type_e get_type(pbnavitia::NavitiaType pb_type) {
    switch (pb_type) {
        case pbnavitia::ADDRESS:
            return nt::Type_e::Address;
        case pbnavitia::STOP_AREA:
            return nt::Type_e::StopArea;
        case pbnavitia::STOP_POINT:
            return nt::Type_e::StopPoint;
        case pbnavitia::LINE:
            return nt::Type_e::Line;
        case pbnavitia::LINE_GROUP:
            return nt::Type_e::LineGroup;
        case pbnavitia::ROUTE:
            return nt::Type_e::Route;
        case pbnavitia::JOURNEY_PATTERN:
            return nt::Type_e::JourneyPattern;
        case pbnavitia::NETWORK:
            return nt::Type_e::Network;
        case pbnavitia::COMMERCIAL_MODE:
            return nt::Type_e::CommercialMode;
        case pbnavitia::PHYSICAL_MODE:
            return nt::Type_e::PhysicalMode;
        case pbnavitia::CONNECTION:
            return nt::Type_e::Connection;
        case pbnavitia::JOURNEY_PATTERN_POINT:
            return nt::Type_e::JourneyPatternPoint;
        case pbnavitia::COMPANY:
            return nt::Type_e::Company;
        case pbnavitia::VEHICLE_JOURNEY:
            return nt::Type_e::VehicleJourney;
        case pbnavitia::POI:
            return nt::Type_e::POI;
        case pbnavitia::POITYPE:
            return nt::Type_e::POIType;
        case pbnavitia::ADMINISTRATIVE_REGION:
            return nt::Type_e::Admin;
        case pbnavitia::CALENDAR:
            return nt::Type_e::Calendar;
        case pbnavitia::IMPACT:
            return nt::Type_e::Impact;
        case pbnavitia::TRIP:
            return nt::Type_e::MetaVehicleJourney;
        case pbnavitia::CONTRIBUTOR:
            return nt::Type_e::Contributor;
        case pbnavitia::DATASET:
            return nt::Type_e::Dataset;
        default:
            return nt::Type_e::Unknown;
    }
}

static nt::OdtLevel_e get_odt_level(pbnavitia::OdtLevel pb_odt_level) {
    switch (pb_odt_level) {
        case pbnavitia::OdtLevel::with_stops:
            return nt::OdtLevel_e::with_stops;
        case pbnavitia::OdtLevel::zonal:
            return nt::OdtLevel_e::zonal;
        case pbnavitia::OdtLevel::all:
            return nt::OdtLevel_e::all;
        default:
            return nt::OdtLevel_e::scheduled;
    }
}

template <class T>
std::vector<nt::Type_e> vector_of_pb_types(const T& pb_object) {
    std::vector<nt::Type_e> result;
    result.reserve(pb_object.types_size());
    for (int i = 0; i < pb_object.types_size(); ++i) {
        result.push_back(get_type(pb_object.types(i)));
    }
    return result;
}

static type::RTLevel get_realtime_level(pbnavitia::RTLevel pb_level) {
    switch (pb_level) {
        case pbnavitia::RTLevel::BASE_SCHEDULE:
            return type::RTLevel::Base;
        case pbnavitia::RTLevel::ADAPTED_SCHEDULE:
            return type::RTLevel::Adapted;
        case pbnavitia::RTLevel::REALTIME:
            return type::RTLevel::RealTime;
        default:
            throw navitia::recoverable_exception("unhandled realtime level");
    }
}

template <class T>
std::vector<std::string> vector_of_admins(const T& admin) {
    std::vector<std::string> result;
    result.reserve(admin.admin_uris_size());
    for (int i = 0; i < admin.admin_uris_size(); ++i) {
        result.push_back(admin.admin_uris(i));
    }
    return result;
}

Worker::Worker(kraken::Configuration conf)
    : conf(std::move(conf)), logger(log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"))) {}

Worker::~Worker() = default;

static std::string get_string_status(const nt::Data* data) {
    if (data->loaded) {
        return "running";
    }
    if (data->loading) {
        return "loading_data";
    }
    return "no_data";
}

static bool get_geojson_state(const pbnavitia::Request& request) {
    bool result = false;
    switch (request.requested_api()) {
        case pbnavitia::pt_objects:
            result = request.pt_objects().disable_geojson();
            break;
        case pbnavitia::ROUTE_SCHEDULES:
        case pbnavitia::NEXT_DEPARTURES:
        case pbnavitia::NEXT_ARRIVALS:
        case pbnavitia::DEPARTURE_BOARDS:
        case pbnavitia::terminus_schedules:
            result = request.next_stop_times().disable_geojson();
            break;
        case pbnavitia::PTREFERENTIAL:
            result = request.ptref().disable_geojson();
            break;
        default:
            result = false;
            break;
    }
    return result;
}

void Worker::geo_status() {
    auto status = this->pb_creator.mutable_geo_status();
    const auto* d = this->pb_creator.data;
    status->set_nb_admins(d->geo_ref->admins.size());
    status->set_nb_ways(d->geo_ref->ways.size());
    int nb_addr = std::accumulate(begin(d->geo_ref->ways), end(d->geo_ref->ways), 0, [](int sum, const georef::Way* w) {
        return sum + w->house_number_left.size() + w->house_number_right.size();
    });
    status->set_nb_addresses(nb_addr);
    int nb_admins_from_cities =
        std::accumulate(begin(d->geo_ref->admins), end(d->geo_ref->admins), 0,
                        [](int sum, const georef::Admin* a) { return (a->from_original_dataset ? sum : sum + 1); });
    status->set_nb_admins_from_cities(nb_admins_from_cities);
    status->set_nb_poi(d->geo_ref->pois.size());
    status->set_poi_source(d->meta->poi_source);
    status->set_street_network_source(d->meta->street_network_source);
}

void Worker::status() {
    auto status = this->pb_creator.mutable_status();
    const auto* d = this->pb_creator.data;
    status->set_data_version(d->version);
    status->set_navitia_version(config::project_version);
    status->set_loaded(d->loaded);
    status->set_last_load_status(d->last_load_succeeded);
    status->set_last_load_at(pt::to_iso_string(d->last_load_at));
    status->set_last_rt_data_loaded(pt::to_iso_string(d->last_rt_data_loaded()));
    status->set_nb_threads(conf.nb_threads());
    status->set_is_connected_to_rabbitmq(d->is_connected_to_rabbitmq);
    status->set_disruption_error(d->disruption_error);
    status->set_status(get_string_status(d));
    status->set_is_realtime_loaded(d->is_realtime_loaded);
    for (const auto& contrib : this->conf.rt_topics()) {
        status->add_rt_contributors(contrib);
    }
    if (d->loaded) {
        status->set_publication_date(pt::to_iso_string(d->meta->publication_date));
        status->set_start_production_date(bg::to_iso_string(d->meta->production_date.begin()));
        status->set_end_production_date(bg::to_iso_string(d->meta->production_date.last()));
        status->set_dataset_created_at(pt::to_iso_string(d->meta->dataset_created_at));
    } else {
        status->set_publication_date("");
        status->set_start_production_date("");
        status->set_end_production_date("");
        status->set_dataset_created_at("");
    }
}

void Worker::metadatas() {
    auto metadatas = this->pb_creator.mutable_metadatas();
    const auto* d = this->pb_creator.data;
    if (d->loaded) {
        metadatas->set_start_production_date(bg::to_iso_string(d->meta->production_date.begin()));
        metadatas->set_end_production_date(bg::to_iso_string(d->meta->production_date.last()));
        metadatas->set_shape(d->meta->shape);
        // we get the first timezone of the dataset
        const auto* tz = d->pt_data->get_main_timezone();
        if (tz) {
            metadatas->set_timezone(tz->tz_name);
        }
        for (const type::Contributor* contributor : d->pt_data->contributors) {
            metadatas->add_contributors(contributor->uri);
        }
        if (!d->meta->publisher_name.empty()) {
            metadatas->set_name(d->meta->publisher_name);
        }
        metadatas->set_last_load_at(navitia::to_posix_timestamp(d->last_load_at));
        metadatas->set_dataset_created_at(pt::to_iso_string(d->meta->dataset_created_at));
    } else {
        metadatas->set_start_production_date("");
        metadatas->set_end_production_date("");
        metadatas->set_shape("");
        metadatas->set_timezone("");
        metadatas->set_dataset_created_at("");
    }
    metadatas->set_status(get_string_status(d));
}

void Worker::feed_publisher() {
    const auto* d = this->pb_creator.data;
    if (!conf.display_contributors()) {
        this->pb_creator.clear_feed_publishers();
    }
    if (!d->meta->license.empty()) {
        auto pt_feed_publisher = this->pb_creator.add_feed_publishers();
        // instance_name is required
        pt_feed_publisher->set_id(d->meta->instance_name);
        if (!d->meta->publisher_name.empty()) {
            pt_feed_publisher->set_name(d->meta->publisher_name);
        }
        if (!d->meta->publisher_url.empty()) {
            pt_feed_publisher->set_url(d->meta->publisher_url);
        }
        if (!d->meta->license.empty()) {
            pt_feed_publisher->set_license(d->meta->license);
        }
    }

    if (d->meta->street_network_source == "osm") {
        // we hardcode the osm feed_publisher
        auto osm_fp = this->pb_creator.add_feed_publishers();
        // instance_name is required
        osm_fp->set_id("osm");
        osm_fp->set_name("openstreetmap");
        osm_fp->set_url("https://www.openstreetmap.org/copyright");
        osm_fp->set_license("ODbL");
    }
}

void Worker::init_worker_data(const navitia::type::Data* data,
                              const pt::ptime now,
                              const pt::time_period action_period,
                              const bool disable_geojson,
                              const bool disable_feedpublisher,
                              const bool disable_disruption) {
    //@TODO should be done in data_manager
    if (data->data_identifier != this->last_data_identifier || !planner) {
        planner = std::make_unique<routing::RAPTOR>(*data);
        street_network_worker = std::make_unique<georef::StreetNetwork>(*data->geo_ref);
        this->last_data_identifier = data->data_identifier;
        LOG4CPLUS_INFO(logger, "Instanciate planner");
    }
    this->pb_creator.init(data, now, action_period, disable_geojson, disable_feedpublisher, disable_disruption);
}

void Worker::autocomplete(const pbnavitia::PlacesRequest& request) {
    const auto* data = this->pb_creator.data;
    navitia::autocomplete::autocomplete(this->pb_creator, request.q(), vector_of_pb_types(request), request.depth(),
                                        request.count(), vector_of_admins(request), request.search_type(), *data,
                                        request.main_stop_area_weight_factor());
}

void Worker::pt_object(const pbnavitia::PtobjectRequest& request) {
    const auto* data = this->pb_creator.data;
    navitia::autocomplete::autocomplete(this->pb_creator, request.q(), vector_of_pb_types(request), request.depth(),
                                        request.count(), vector_of_admins(request), request.search_type(), *data);
}

void Worker::traffic_reports(const pbnavitia::TrafficReportsRequest& request) {
    const auto* data = this->pb_creator.data;
    std::vector<std::string> forbidden_uris;
    forbidden_uris.reserve(request.forbidden_uris_size());
    for (int i = 0; i < request.forbidden_uris_size(); ++i) {
        forbidden_uris.push_back(request.forbidden_uris(i));
    }
    navitia::disruption::traffic_reports(this->pb_creator, *data, request.depth(), request.count(),
                                         request.start_page(), request.filter(), forbidden_uris);
}

void Worker::line_reports(const pbnavitia::LineReportsRequest& request) {
    const auto* data = this->pb_creator.data;
    std::vector<std::string> forbidden_uris;
    for (const auto& uri : request.forbidden_uris()) {
        forbidden_uris.push_back(uri);
    }
    navitia::disruption::line_reports(
        this->pb_creator, *data, request.depth(), request.count(), request.start_page(), request.filter(),
        forbidden_uris, boost::make_optional(request.has_since_datetime(), bt::from_time_t(request.since_datetime())),
        boost::make_optional(request.has_until_datetime(), bt::from_time_t(request.until_datetime())));
}

void Worker::calendars(const pbnavitia::CalendarsRequest& request) {
    const auto* data = this->pb_creator.data;
    std::vector<std::string> forbidden_uris;
    forbidden_uris.reserve(request.forbidden_uris_size());
    for (int i = 0; i < request.forbidden_uris_size(); ++i) {
        forbidden_uris.push_back(request.forbidden_uris(i));
    }
    navitia::calendar::calendars(this->pb_creator, *data, request.start_date(), request.end_date(), request.depth(),
                                 request.count(), request.start_page(), request.filter(), forbidden_uris);
}

void Worker::next_stop_times(const pbnavitia::NextStopTimeRequest& request, pbnavitia::API api) {
    std::vector<std::string> forbidden_uri;
    forbidden_uri.reserve(request.forbidden_uri_size());
    for (int i = 0; i < request.forbidden_uri_size(); ++i) {
        forbidden_uri.push_back(request.forbidden_uri(i));
    }

    bt::ptime from_datetime = bt::from_time_t(request.from_datetime());

    auto rt_level = get_realtime_level(request.realtime_level());
    try {
        switch (api) {
            case pbnavitia::NEXT_DEPARTURES:
                timetables::passages(this->pb_creator, request.departure_filter(), forbidden_uri, from_datetime,
                                     request.duration(), request.nb_stoptimes(), request.depth(),
                                     type::AccessibiliteParams(), rt_level, api, request.count(), request.start_page());
                break;
            case pbnavitia::NEXT_ARRIVALS:
                timetables::passages(this->pb_creator, request.arrival_filter(), forbidden_uri, from_datetime,
                                     request.duration(), request.nb_stoptimes(), request.depth(),
                                     type::AccessibiliteParams(), rt_level, api, request.count(), request.start_page());
                break;
            case pbnavitia::DEPARTURE_BOARDS:
                timetables::departure_board(
                    this->pb_creator, request.departure_filter(),
                    request.has_calendar() ? boost::optional<const std::string>(request.calendar())
                                           : boost::optional<const std::string>(),
                    forbidden_uri, from_datetime, request.duration(), request.depth(), request.count(),
                    request.start_page(), rt_level, request.items_per_schedule());
                break;
            case pbnavitia::ROUTE_SCHEDULES:
                timetables::route_schedule(
                    this->pb_creator, request.departure_filter(),
                    request.has_calendar() ? boost::optional<const std::string>(request.calendar())
                                           : boost::optional<const std::string>(),
                    forbidden_uri, from_datetime, request.duration(), request.items_per_schedule(), request.depth(),
                    request.count(), request.start_page(), rt_level);
                break;
            case pbnavitia::terminus_schedules:
                timetables::terminus_schedules(
                    this->pb_creator, request.departure_filter(),
                    request.has_calendar() ? boost::optional<const std::string>(request.calendar())
                                           : boost::optional<const std::string>(),
                    forbidden_uri, from_datetime, request.duration(), request.depth(), request.count(),
                    request.start_page(), rt_level, request.items_per_schedule());
                break;
            default:
                LOG4CPLUS_WARN(logger, "Unknown timetable query");
                pb_creator.fill_pb_error(pbnavitia::Error::unknown_api, "Unknown time table api");
        }

    } catch (const navitia::ptref::parsing_error& error) {
        LOG4CPLUS_ERROR(logger, "Error in the ptref request  : " + error.more);
        const auto str_error = "Unknow filter : " + error.more;
        pb_creator.fill_pb_error(pbnavitia::Error::bad_filter, str_error);
    }
}

void Worker::proximity_list(const pbnavitia::PlacesNearbyRequest& request) {
    const auto* data = this->pb_creator.data;
    std::vector<std::string> forbidden_uris;
    forbidden_uris.reserve(request.forbidden_uris_size());
    for (int i = 0; i < request.forbidden_uris_size(); ++i) {
        forbidden_uris.push_back(request.forbidden_uris(i));
    }
    type::EntryPoint ep(data->get_type_of_id(request.uri()), request.uri());
    type::GeographicalCoord coord;
    try {
        coord = coord_of_entry_point(ep, *data);
    } catch (const navitia::coord_conversion_exception& e) {
        this->pb_creator.fill_pb_error(pbnavitia::Error::bad_format, e.what());
        return;
    }
    double stop_points_nearby_radius = 0;
    if (request.has_stop_points_nearby_radius() && request.stop_points_nearby_radius() > 0) {
        stop_points_nearby_radius = request.stop_points_nearby_radius();
    }
    proximitylist::find(this->pb_creator, coord, request.distance(), vector_of_pb_types(request), forbidden_uris,
                        request.filter(), request.depth(), request.count(), request.start_page(), *data,
                        stop_points_nearby_radius);
}

static type::StreetNetworkParams streetnetwork_params_of_entry_point(const pbnavitia::StreetNetworkParams& request,
                                                                     const navitia::type::Data* data,
                                                                     const bool is_origin) {
    type::StreetNetworkParams result;
    std::string uri;

    if (is_origin) {
        result.mode = type::static_data::get()->modeByCaption(request.origin_mode());
        result.set_filter(request.origin_filter());
    } else {
        result.mode = type::static_data::get()->modeByCaption(request.destination_mode());
        result.set_filter(request.destination_filter());
    }
    int max_non_pt;
    switch (result.mode) {
        case type::Mode_e::Bike:
            result.offset = data->geo_ref->offsets[type::Mode_e::Bike];
            result.speed_factor = request.bike_speed() / double(georef::default_speed[type::Mode_e::Bike]);
            max_non_pt = request.max_bike_duration_to_pt();
            break;
        case type::Mode_e::Car:
            result.offset = data->geo_ref->offsets[type::Mode_e::Car];
            result.speed_factor = request.car_speed() / double(georef::default_speed[type::Mode_e::Car]);
            max_non_pt = request.max_car_duration_to_pt();
            break;
        case type::Mode_e::CarNoPark:
            result.offset = data->geo_ref->offsets[type::Mode_e::CarNoPark];
            result.speed_factor = request.car_no_park_speed() / double(georef::default_speed[type::Mode_e::CarNoPark]);
            max_non_pt = request.max_car_no_park_duration_to_pt();
            break;
        case type::Mode_e::Bss:
            result.offset = data->geo_ref->offsets[type::Mode_e::Bss];
            result.speed_factor = request.bss_speed() / double(georef::default_speed[type::Mode_e::Bss]);
            max_non_pt = request.max_bss_duration_to_pt();
            break;
        default:
            result.offset = data->geo_ref->offsets[type::Mode_e::Walking];
            result.speed_factor = request.walking_speed() / double(georef::default_speed[type::Mode_e::Walking]);
            max_non_pt = request.max_walking_duration_to_pt();
            break;
    }
    if (result.speed_factor <= 0) {
        throw navitia::recoverable_exception("invalid speed factor");
    }
    result.max_duration = navitia::seconds(max_non_pt);
    result.enable_direct_path = request.enable_direct_path();
    return result;
}

void Worker::place_uri(const pbnavitia::PlaceUriRequest& request) {
    const auto* data = this->pb_creator.data;
    if (type::EntryPoint::is_coord(request.uri())) {
        auto ep = type::EntryPoint(type::Type_e::Coord, request.uri());
        type::GeographicalCoord coord = ep.coordinates;

        try {
            auto address = this->pb_creator.data->geo_ref->nearest_addr(coord);
            const auto& way_coord = WayCoord(address.second, coord, address.first);
            pb_creator.fill(&way_coord, pb_creator.add_places(), request.depth());
        } catch (const proximitylist::NotFound&) {
            pb_creator.fill_pb_error(pbnavitia::Error::unknown_object, "Unable to find place: " + request.uri());
        }
        return;
    }

    auto it_sa = data->pt_data->stop_areas_map.find(request.uri());
    if (it_sa != data->pt_data->stop_areas_map.end()) {
        pb_creator.fill(it_sa->second, pb_creator.add_places(), request.depth());
    } else {
        auto it_sp = data->pt_data->stop_points_map.find(request.uri());
        if (it_sp != data->pt_data->stop_points_map.end()) {
            pb_creator.fill(it_sp->second, pb_creator.add_places(), request.depth());
        } else {
            auto it_poi = data->geo_ref->poi_map.find(request.uri());
            if (it_poi != data->geo_ref->poi_map.end()) {
                pb_creator.fill(it_poi->second, pb_creator.add_places(), request.depth());
            } else {
                auto it_admin = data->geo_ref->admin_map.find(request.uri());
                if (it_admin != data->geo_ref->admin_map.end()) {
                    pb_creator.fill(data->geo_ref->admins[it_admin->second], pb_creator.add_places(), request.depth());
                } else {
                    this->pb_creator.fill_pb_error(pbnavitia::Error::unable_to_parse,
                                                   "Unable to parse : " + request.uri());
                }
            }
        }
    }
}

template <typename T>
static void fill_or_error(const pbnavitia::PlaceCodeRequest& request, PbCreator& pb_creator) {
    const auto& objs = pb_creator.data->pt_data->codes.get_objs<T>(request.type_code(), request.code());
    if (objs.empty()) {
        pb_creator.fill_pb_error(pbnavitia::Error::unknown_object, "Unknown object");
    } else {
        // FIXME: add every object or (as before) just the first one?
        pb_creator.fill(objs.front(), pb_creator.add_places(), 0);
    }
}

void Worker::place_code(const pbnavitia::PlaceCodeRequest& request) {
    switch (request.type()) {
        case pbnavitia::PlaceCodeRequest::StopArea:
            fill_or_error<nt::StopArea>(request, pb_creator);
            break;
        case pbnavitia::PlaceCodeRequest::Network:
            fill_or_error<nt::Network>(request, pb_creator);
            break;
        case pbnavitia::PlaceCodeRequest::Company:
            fill_or_error<nt::Company>(request, pb_creator);
            break;
        case pbnavitia::PlaceCodeRequest::Line:
            fill_or_error<nt::Line>(request, pb_creator);
            break;
        case pbnavitia::PlaceCodeRequest::Route:
            fill_or_error<nt::Route>(request, pb_creator);
            break;
        case pbnavitia::PlaceCodeRequest::VehicleJourney:
            fill_or_error<nt::VehicleJourney>(request, pb_creator);
            break;
        case pbnavitia::PlaceCodeRequest::StopPoint:
            fill_or_error<nt::StopPoint>(request, pb_creator);
            break;
        case pbnavitia::PlaceCodeRequest::Calendar:
            fill_or_error<nt::Calendar>(request, pb_creator);
            break;
    }
}

static type::EntryPoint create_journeys_entry_point(const ::pbnavitia::LocationContext& location,
                                                    const ::pbnavitia::StreetNetworkParams* sn_params,
                                                    const navitia::type::Data* data,
                                                    const bool is_origin) {
    Type_e entry_point_type = data->get_type_of_id(location.place());
    type::EntryPoint entry_point = type::EntryPoint(entry_point_type, location.place(), location.access_duration());
    switch (entry_point.type) {
        case type::Type_e::Address:
        case type::Type_e::Coord:
        case type::Type_e::Admin:
        case type::Type_e::StopArea:
        case type::Type_e::POI:
        case type::Type_e::StopPoint:
            if (sn_params) {
                entry_point.streetnetwork_params = streetnetwork_params_of_entry_point(*sn_params, data, is_origin);
            }
            entry_point.coordinates = coord_of_entry_point(entry_point, *data);
            break;
        default:
            break;
    }
    return entry_point;
}

JourneysArg::JourneysArg(type::EntryPoints origins,
                         type::AccessibiliteParams accessibilite_params,
                         std::vector<std::string> forbidden,
                         std::vector<std::string> allowed,
                         type::RTLevel rt_level,
                         type::EntryPoints destinations,
                         std::vector<uint64_t> datetimes,
                         boost::optional<type::EntryPoint> isochrone_center)
    : origins(std::move(origins)),
      accessibilite_params(accessibilite_params),
      forbidden(std::move(forbidden)),
      allowed(std::move(allowed)),
      rt_level(rt_level),
      destinations(std::move(destinations)),
      datetimes(std::move(datetimes)),
      isochrone_center(std::move(isochrone_center)) {}
JourneysArg::JourneysArg() = default;

navitia::JourneysArg Worker::fill_journeys(const pbnavitia::JourneysRequest& request) {
    const auto* data = this->pb_creator.data;
    type::EntryPoints origins;
    const auto* sn_params = request.has_streetnetwork_params() ? &request.streetnetwork_params() : nullptr;
    for (int i = 0; i < request.origin().size(); i++) {
        origins.push_back(create_journeys_entry_point(request.origin(i), sn_params, data, true));
    }

    type::EntryPoints destinations;
    for (int i = 0; i < request.destination().size(); i++) {
        destinations.push_back(create_journeys_entry_point(request.destination(i), sn_params, data, false));
    }

    std::vector<std::string> forbidden;
    forbidden.reserve(request.forbidden_uris_size());
    for (int i = 0; i < request.forbidden_uris_size(); ++i) {
        forbidden.push_back(request.forbidden_uris(i));
    }

    std::vector<std::string> allowed;
    allowed.reserve(request.allowed_id_size());
    for (int i = 0; i < request.allowed_id_size(); ++i) {
        allowed.push_back(request.allowed_id(i));
    }

    std::vector<uint64_t> datetimes;
    datetimes.reserve(request.datetimes_size());
    for (int i = 0; i < request.datetimes_size(); ++i) {
        datetimes.push_back(request.datetimes(i));
    }

    /// Accessibility params
    type::AccessibiliteParams accessibilite_params;
    accessibilite_params.properties.set(type::hasProperties::WHEELCHAIR_BOARDING, request.wheelchair());
    accessibilite_params.properties.set(type::hasProperties::BIKE_ACCEPTED, request.bike_in_pt());

    accessibilite_params.vehicle_properties.set(type::hasVehicleProperties::WHEELCHAIR_ACCESSIBLE,
                                                request.wheelchair());
    accessibilite_params.vehicle_properties.set(type::hasVehicleProperties::BIKE_ACCEPTED, request.bike_in_pt());

    type::RTLevel rt_level = get_realtime_level(request.realtime_level());

    const auto isochrone_center = request.has_isochrone_center()
                                      ? create_journeys_entry_point(request.isochrone_center(), sn_params, data, true)
                                      : boost::optional<type::EntryPoint>{};

    return JourneysArg(std::move(origins), accessibilite_params, std::move(forbidden), std::move(allowed), rt_level,
                       std::move(destinations), std::move(datetimes), isochrone_center);
}

void Worker::err_msg_isochron(navitia::PbCreator& pb_creator, const std::string& err_msg) {
    pb_creator.fill_pb_error(pbnavitia::Error::bad_format, pbnavitia::NO_SOLUTION, err_msg);
}

void Worker::journeys(const pbnavitia::JourneysRequest& request, pbnavitia::API api) {
    try {
        navitia::JourneysArg arg = fill_journeys(request);

        if (arg.origins.empty() && arg.destinations.empty()) {
            // should never happen, jormungandr filters that, but it never hurts to double check
            this->pb_creator.fill_pb_error(pbnavitia::Error::no_origin_nor_destination,
                                           pbnavitia::NO_ORIGIN_NOR_DESTINATION_POINT,
                                           "no origin point nor destination point given");
            return;
        }
        uint32_t max_extra_second_pass = 0;  // request.max_extra_second_pass()
        switch (api) {
            case pbnavitia::pt_planner:
                routing::make_pt_response(
                    this->pb_creator, *planner, arg.origins, arg.destinations, arg.datetimes[0], request.clockwise(),
                    arg.accessibilite_params, arg.forbidden, arg.allowed, arg.rt_level,
                    seconds{request.walking_transfer_penalty()}, request.max_duration(), request.max_transfers(),
                    max_extra_second_pass,
                    request.has_direct_path_duration()
                        ? boost::optional<time_duration>(seconds{request.direct_path_duration()})
                        : boost::optional<time_duration>(),
                    request.has_min_nb_journeys() ? boost::make_optional<uint32_t>(request.min_nb_journeys())
                                                  : boost::none,
                    request.night_bus_filter_max_factor(), request.night_bus_filter_base_factor(),
                    request.has_timeframe_duration() ? boost::make_optional<uint32_t>(request.timeframe_duration())
                                                     : boost::none,
                    request.depth());
                break;
            default:
                routing::make_response(
                    this->pb_creator, *planner, arg.origins[0], arg.destinations[0], arg.datetimes, request.clockwise(),
                    arg.accessibilite_params, arg.forbidden, arg.allowed, *street_network_worker, arg.rt_level,
                    seconds{request.walking_transfer_penalty()}, request.max_duration(), request.max_transfers(),
                    max_extra_second_pass, request.free_radius_from(), request.free_radius_to(),
                    request.has_min_nb_journeys() ? boost::make_optional<uint32_t>(request.min_nb_journeys())
                                                  : boost::none,
                    request.night_bus_filter_max_factor(), request.night_bus_filter_base_factor(),
                    request.has_timeframe_duration() ? boost::make_optional<uint32_t>(request.timeframe_duration())
                                                     : boost::none,
                    request.depth());
        }
    } catch (const navitia::coord_conversion_exception& e) {
        this->pb_creator.fill_pb_error(pbnavitia::Error::bad_format, e.what());
    }
}

static std::pair<const type::EntryPoint, const boost::optional<const type::EntryPoints&>> get_center_and_stop_points(
    const JourneysArg& arg) {
    // If we have a center
    // Origins or destinations are already computed stop_points
    const auto& center = arg.isochrone_center ? *arg.isochrone_center
                                              : (arg.origins.empty() ? arg.destinations.front() : arg.origins.front());
    const auto& stop_points = arg.isochrone_center ? (arg.origins.empty() ? arg.destinations : arg.origins)
                                                   : boost::optional<const type::EntryPoints&>{};

    return std::make_pair(center, stop_points);
}

void Worker::isochrone(const pbnavitia::JourneysRequest& request) {
    navitia::JourneysArg arg = JourneysArg();
    if (!set_journeys_args(request, arg, "isochrone")) {
        return;
    }

    auto const center_and_stop_points = get_center_and_stop_points(arg);
    navitia::routing::make_isochrone(this->pb_creator, *planner, center_and_stop_points.first, request.datetimes(0),
                                     request.clockwise(), arg.accessibilite_params, arg.forbidden, arg.allowed,
                                     *street_network_worker, arg.rt_level, request.max_duration(),
                                     request.max_transfers(), center_and_stop_points.second);
}

void Worker::pt_ref(const pbnavitia::PTRefRequest& request) {
    const auto* data = this->pb_creator.data;
    std::vector<std::string> forbidden_uri;
    forbidden_uri.reserve(request.forbidden_uri_size());
    for (int i = 0; i < request.forbidden_uri_size(); ++i) {
        forbidden_uri.push_back(request.forbidden_uri(i));
    }
    auto rt_level = get_realtime_level(request.realtime_level());
    navitia::ptref::query_pb(
        this->pb_creator, get_type(request.requested_type()), request.filter(), forbidden_uri,
        get_odt_level(request.odt_level()), request.depth(), request.start_page(), request.count(),
        boost::make_optional(request.has_since_datetime(), bt::from_time_t(request.since_datetime())),
        boost::make_optional(request.has_until_datetime(), bt::from_time_t(request.until_datetime())), rt_level, *data);
}

// returns false if there is an error
bool Worker::set_journeys_args(const pbnavitia::JourneysRequest& request, JourneysArg& arg, const std::string& name) {
    try {
        arg = fill_journeys(request);
    } catch (const navitia::coord_conversion_exception& e) {
        this->pb_creator.fill_pb_error(pbnavitia::Error::bad_format, e.what());
        return false;
    }
    if (arg.origins.empty() && arg.destinations.empty()) {
        // should never happen, jormungandr filters that, but it never hurts to double check
        this->pb_creator.fill_pb_error(pbnavitia::Error::no_origin_nor_destination,
                                       pbnavitia::NO_ORIGIN_NOR_DESTINATION_POINT,
                                       "no origin point nor destination point given");
        return false;
    }

    if (!arg.origins.empty() && !request.clockwise()) {
        err_msg_isochron(this->pb_creator, name + " works only for clockwise request");
        return false;
    }
    if (arg.origins.empty() && request.clockwise()) {
        err_msg_isochron(this->pb_creator, "reverse " + name + " works only for anti-clockwise request");
        return false;
    }
    return true;
}

void Worker::graphical_isochrone(const pbnavitia::GraphicalIsochroneRequest& request) {
    const auto& request_journey = request.journeys_request();
    navitia::JourneysArg arg = JourneysArg();
    if (!set_journeys_args(request_journey, arg, "isochrone")) {
        return;
    }

    auto const center_and_stop_points = get_center_and_stop_points(arg);
    std::vector<DateTime> boundary_duration;
    boundary_duration.reserve(request.boundary_duration_size());
    for (int i = 0; i < request.boundary_duration_size(); ++i) {
        boundary_duration.push_back(request.boundary_duration(i));
    }
    const auto& sn = request_journey.streetnetwork_params();
    const auto end_mode_iso = request_journey.clockwise() ? sn.destination_mode() : sn.origin_mode();
    const auto end_mode = type::static_data::get()->modeByCaption(end_mode_iso);
    const double end_speed = get_speed(sn, end_mode);
    navitia::routing::make_graphical_isochrone(
        this->pb_creator, *planner, center_and_stop_points.first, request_journey.datetimes(0), boundary_duration,
        request_journey.max_transfers(), arg.accessibilite_params, arg.forbidden, arg.allowed,
        request_journey.clockwise(), arg.rt_level, *street_network_worker, end_speed, center_and_stop_points.second);
}

void Worker::heat_map(const pbnavitia::HeatMapRequest& request) {
    const auto& request_journey = request.journeys_request();
    navitia::JourneysArg arg;
    if (!set_journeys_args(request_journey, arg, "heat_map")) {
        return;
    }

    auto const center_and_stop_points = get_center_and_stop_points(arg);
    const auto& streetnetwork = request_journey.streetnetwork_params();
    auto end_mode_iso = request_journey.clockwise() ? streetnetwork.destination_mode() : streetnetwork.origin_mode();
    auto end_mode = type::static_data::get()->modeByCaption(end_mode_iso);
    auto end_speed = get_speed(streetnetwork, end_mode);
    navitia::routing::make_heat_map(this->pb_creator, *planner, center_and_stop_points.first,
                                    request_journey.datetimes(0), request_journey.max_duration(),
                                    request_journey.max_transfers(), arg.accessibilite_params, arg.forbidden,
                                    arg.allowed, request_journey.clockwise(), arg.rt_level, *street_network_worker,
                                    end_speed, end_mode, request.resolution(), center_and_stop_points.second);
}

void Worker::car_co2_emission_on_crow_fly(const pbnavitia::CarCO2EmissionRequest& request) {
    const auto* data = this->pb_creator.data;
    auto get_geographical_coord = [&](const pbnavitia::LocationContext& location) {
        auto origin_type = data->get_type_of_id(location.place());
        auto origin = type::EntryPoint{origin_type, location.place(), location.access_duration()};
        auto coordinates = coord_of_entry_point(origin, *data);
        return navitia::type::GeographicalCoord{coordinates.lon(), coordinates.lat()};
    };
    navitia::type::GeographicalCoord origin;
    navitia::type::GeographicalCoord destin;
    try {
        origin = get_geographical_coord(request.origin());
        destin = get_geographical_coord(request.destination());
    } catch (const navitia::coord_conversion_exception& e) {
        this->pb_creator.fill_pb_error(pbnavitia::Error::bad_format, e.what());
        return;
    }

    auto distance = origin.distance_to(destin);
    auto car_mode = data->pt_data->physical_modes_map.find("physical_mode:Car");

    if (car_mode != data->pt_data->physical_modes_map.end() && car_mode->second->co2_emission) {
        auto co2_emission = this->pb_creator.mutable_car_co2_emission();
        co2_emission->set_unit("gEC");
        co2_emission->set_value(CO2_ESTIMATION_COEFF * distance / 1000.0 * car_mode->second->co2_emission.get());
        return;
    }
    this->pb_creator.fill_pb_error(pbnavitia::Error::no_solution,
                                   "physical_mode:Car doesn't contain any information about co2 emission");
}

type::EntryPoint make_sn_entry_point(const std::string& place,
                                     const std::string& mode,
                                     const float speed,
                                     const int max_duration,
                                     const navitia::type::Data& data) {
    Type_e origin_type = data.get_type_of_id(place);
    auto entry_point = type::EntryPoint{origin_type, place, 0};

    switch (entry_point.type) {
        case type::Type_e::Address:
        case type::Type_e::Admin:
        case type::Type_e::Coord:
        case type::Type_e::StopArea:
        case type::Type_e::StopPoint:
        case type::Type_e::POI:
            entry_point.coordinates = coord_of_entry_point(entry_point, data);
            entry_point.streetnetwork_params.mode = type::static_data::get()->modeByCaption(mode);
            break;
        default:
            break;
    }
    auto mode_enum = entry_point.streetnetwork_params.mode;
    switch (mode_enum) {
        case type::Mode_e::Bike:
        case type::Mode_e::Car:
        case type::Mode_e::Bss:
        case type::Mode_e::CarNoPark:
            entry_point.streetnetwork_params.offset = data.geo_ref->offsets[mode_enum];
            entry_point.streetnetwork_params.speed_factor = speed / georef::default_speed[mode_enum];
            break;
        default:
            entry_point.streetnetwork_params.offset = data.geo_ref->offsets[type::Mode_e::Walking];
            entry_point.streetnetwork_params.speed_factor = speed / georef::default_speed[type::Mode_e::Walking];
            break;
    }
    if (entry_point.streetnetwork_params.speed_factor <= 0) {
        throw navitia::recoverable_exception("invalid speed factor");
    }
    entry_point.streetnetwork_params.max_duration = navitia::seconds(max_duration);

    return entry_point;
}

void Worker::street_network_routing_matrix(const pbnavitia::StreetNetworkRoutingMatrixRequest& request) {
    const auto* data = this->pb_creator.data;
    std::vector<type::GeographicalCoord> dest_coords;

    // In this loop, we try to get the coordinates of all destinations
    for (const auto& dest : request.destinations()) {
        Type_e origin_type = data->get_type_of_id(dest.place());
        auto entry_point = type::EntryPoint{origin_type, dest.place(), 0};
        try {
            dest_coords.push_back(coord_of_entry_point(entry_point, *data));
        } catch (const navitia::coord_conversion_exception& e) {
            this->pb_creator.fill_pb_error(pbnavitia::Error::bad_format, e.what());
            return;
        }
    }

    for (const auto& origin : request.origins()) {
        type::EntryPoint entry_point;
        try {
            entry_point =
                make_sn_entry_point(origin.place(), request.mode(), request.speed(), request.max_duration(), *data);
        } catch (const navitia::coord_conversion_exception& e) {
            this->pb_creator.fill_pb_error(pbnavitia::Error::bad_format, e.what());
            return;
        }

        street_network_worker->departure_path_finder.init(entry_point.coordinates,
                                                          entry_point.streetnetwork_params.mode,
                                                          entry_point.streetnetwork_params.speed_factor);
        auto nearest = street_network_worker->departure_path_finder.get_duration_with_dijkstra(
            navitia::time_duration::from_boost_duration(boost::posix_time::seconds(request.max_duration())),
            dest_coords);

        auto* row = this->pb_creator.mutable_sn_routing_matrix()->add_rows();
        for (auto coord : dest_coords) {
            auto* k = row->add_routing_response();
            auto it = nearest.find(coord.uri());
            if (it == nearest.end()) {
                throw navitia::recoverable_exception("Cannot find object: " + coord.uri());
            }
            k->set_duration(it->second.time_duration.total_seconds());
            switch (it->second.routing_status) {
                case georef::RoutingStatus_e::reached:
                    k->set_routing_status(pbnavitia::RoutingStatus::reached);
                    break;
                case georef::RoutingStatus_e::unreached:
                    k->set_routing_status(pbnavitia::RoutingStatus::unreached);
                    break;
                default:
                    k->set_routing_status(pbnavitia::RoutingStatus::unknown);
            }
        }
    }
}

void Worker::direct_path(const pbnavitia::Request& request) {
    const auto* data = this->pb_creator.data;
    const auto& dp_request = request.direct_path();
    const auto* sn_params = dp_request.has_streetnetwork_params() ? &dp_request.streetnetwork_params() : nullptr;
    const auto origin = create_journeys_entry_point(dp_request.origin(), sn_params, data, true);

    const auto destination = create_journeys_entry_point(dp_request.destination(), sn_params, data, false);
    const auto geo_path = street_network_worker->get_direct_path(origin, destination);

    routing::add_direct_path(this->pb_creator, geo_path, origin, destination, {bt::from_time_t(dp_request.datetime())},
                             dp_request.clockwise());
}

void Worker::dispatch(const pbnavitia::Request& request, const nt::Data& data) {
    bool disable_geojson = get_geojson_state(request);
    boost::posix_time::ptime current_datetime = bt::from_time_t(request._current_datetime());
    this->init_worker_data(&data, current_datetime, null_time_period, disable_geojson, request.disable_feedpublisher(),
                           request.disable_disruption());

    // These api can respond even if the data isn't loaded
    if (request.requested_api() == pbnavitia::STATUS) {
        status();
        // Metadatas are needed for /status requests to update timezone
        metadatas();
        return;
    }
    if (request.requested_api() == pbnavitia::METADATAS) {
        metadatas();
        return;
    }
    if (!data.loaded) {
        this->pb_creator.fill_pb_error(pbnavitia::Error::service_unavailable, "The service is loading data");
        return;
    }

    switch (request.requested_api()) {
        case pbnavitia::places:
            autocomplete(request.places());
            break;
        case pbnavitia::pt_objects:
            pt_object(request.pt_objects());
            break;
        case pbnavitia::place_uri:
            place_uri(request.place_uri());
            break;
        case pbnavitia::ROUTE_SCHEDULES:
        case pbnavitia::NEXT_DEPARTURES:
        case pbnavitia::NEXT_ARRIVALS:
        case pbnavitia::DEPARTURE_BOARDS:
        case pbnavitia::terminus_schedules:
            next_stop_times(request.next_stop_times(), request.requested_api());
            break;
        case pbnavitia::NMPLANNER:
        case pbnavitia::pt_planner:
        case pbnavitia::PLANNER:
            journeys(request.journeys(), request.requested_api());
            break;
        case pbnavitia::ISOCHRONE:
            isochrone(request.journeys());
            break;
        case pbnavitia::places_nearby:
            proximity_list(request.places_nearby());
            break;
        case pbnavitia::PTREFERENTIAL:
            pt_ref(request.ptref());
            break;
        case pbnavitia::traffic_reports:
            traffic_reports(request.traffic_reports());
            break;
        case pbnavitia::line_reports:
            line_reports(request.line_reports());
            break;
        case pbnavitia::calendars:
            calendars(request.calendars());
            break;
        case pbnavitia::place_code:
            place_code(request.place_code());
            break;
        case pbnavitia::nearest_stop_points:
            nearest_stop_points(request.nearest_stop_points());
            break;
        case pbnavitia::geo_status:
            geo_status();
            break;
        case pbnavitia::car_co2_emission:
            car_co2_emission_on_crow_fly(request.car_co2_emission());
            break;
        case pbnavitia::direct_path:
            direct_path(request);
            break;
        case pbnavitia::graphical_isochrone:
            graphical_isochrone(request.isochrone());
            break;
        case pbnavitia::heat_map:
            heat_map(request.heat_map());
            break;
        case pbnavitia::street_network_routing_matrix:
            street_network_routing_matrix(request.sn_routing_matrix());
            break;
        case pbnavitia::odt_stop_points:
            odt_stop_points(request.coord());
            break;
        case pbnavitia::matching_routes:
            get_matching_routes(request.matching_routes());
            break;
        case pbnavitia::equipment_reports:
            equipment_reports(request.equipment_reports());
            break;
        default:
            LOG4CPLUS_WARN(logger, "Unknown API : " + API_Name(request.requested_api()));
            this->pb_creator.fill_pb_error(pbnavitia::Error::unknown_api, "Unknown API");
            break;
    }
    metadatas();  // we add the metadatas for each response
    if (!request.disable_feedpublisher()) {
        feed_publisher();
    }
}

void Worker::nearest_stop_points(const pbnavitia::NearestStopPointsRequest& request) {
    const auto* data = this->pb_creator.data;
    double speed = get_speed(request, type::static_data::get()->modeByCaption(request.mode()));
    type::EntryPoint entry_point;
    try {
        entry_point = make_sn_entry_point(request.place(), request.mode(), speed, request.max_duration(), *data);
    } catch (const navitia::coord_conversion_exception& e) {
        this->pb_creator.fill_pb_error(pbnavitia::Error::bad_format, e.what());
        return;
    }
    entry_point.streetnetwork_params.max_duration = navitia::seconds(request.max_duration());
    street_network_worker->init(entry_point, {});
    // kraken don't handle reverse isochrone
    auto result = routing::get_stop_points(entry_point, *data, *street_network_worker, 0u);
    if (!result) {
        this->pb_creator.fill_pb_error(pbnavitia::Error::unknown_object,
                                       "The entry point: " + entry_point.uri + " is not valid");
        return;
    }

    for (const auto& item : *result) {
        auto* nsp = pb_creator.add_nearest_stop_points();
        this->pb_creator.fill(planner->get_sp(item.first), nsp->mutable_stop_point(), 0);
        nsp->set_access_duration(item.second.total_seconds());
    }
}

void Worker::odt_stop_points(const pbnavitia::GeographicalCoord& request) {
    navitia::type::GeographicalCoord coord;
    coord.set_lon(request.lon());
    coord.set_lat(request.lat());
    const auto* data = this->pb_creator.data;
    const auto& zonal_sps = data->pt_data->get_stop_points_by_area(coord);
    this->pb_creator.pb_fill(zonal_sps, 0);
}

void Worker::get_matching_routes(const pbnavitia::MatchingRoute& matching_route) {
    const auto* line = find_or_default(matching_route.line_uri(), this->pb_creator.data->get_assoc_data<nt::Line>());
    if (!line) {
        this->pb_creator.fill_pb_error(pbnavitia::Error::unable_to_parse,
                                       "Cannot find line " + matching_route.line_uri());
        return;
    }

    const auto* start =
        find_or_default(matching_route.start_stop_point_uri(), this->pb_creator.data->get_assoc_data<nt::StopPoint>());
    if (!start) {
        this->pb_creator.fill_pb_error(pbnavitia::Error::unable_to_parse,
                                       "Cannot find stoppoint " + matching_route.start_stop_point_uri());
        return;
    }

    ptref::fill_matching_routes(this->pb_creator, this->pb_creator.data, line, start,
                                {matching_route.destination_code_key(), matching_route.destination_code()});
}

void Worker::equipment_reports(const pbnavitia::EquipmentReportsRequest& equipment_reports) {
    const auto& proto_uris = equipment_reports.forbidden_uris();
    std::vector<std::string> forbidden_uris(proto_uris.begin(), proto_uris.end());

    equipment::equipment_reports(this->pb_creator, equipment_reports.filter(), equipment_reports.count(),
                                 equipment_reports.depth(), equipment_reports.start_page(), forbidden_uris);
}

}  // namespace navitia
