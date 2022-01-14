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

#include "ed_reader.h"

#include "ed/connectors/fare_utils.h"
#include "type/meta_data.h"
#include "type/access_point.h"
#include "type/network.h"
#include "type/company.h"
#include "type/contributor.h"
#include "type/commercial_mode.h"
#include "type/dataset.h"

#include <boost/foreach.hpp>
#include <boost/geometry.hpp>
#include <boost/make_shared.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
namespace ed {

namespace bg = boost::gregorian;
namespace bt = boost::posix_time;
namespace nt = navitia::type;
namespace ng = navitia::georef;
namespace nf = navitia::fare;

// A function to release the memory of a collection. For a vector, it
// is equivalent to `a.clear(); a.shrink_to_fit();` but some
// collections don't have these methods.
template <typename T>
static void release(T& a) {
    T b;
    a.swap(b);
}

void EdReader::fill(navitia::type::Data& data,
                    const double min_non_connected_graph_ratio,
                    const bool export_georef_edges_geometries) {
    pqxx::work work(*conn, "loading ED");

    this->fill_vector_to_ignore(work, min_non_connected_graph_ratio);
    this->fill_meta(data, work);
    // TODO merge fill_feed_infos, fill_meta
    this->fill_feed_infos(data, work);
    this->fill_timezones(data, work);
    this->fill_networks(data, work);
    this->fill_commercial_modes(data, work);
    this->fill_physical_modes(data, work);
    this->fill_companies(data, work);
    this->fill_contributors(data, work);
    this->fill_datasets(data, work);

    this->fill_stop_areas(data, work);
    this->fill_stop_points(data, work);
    this->fill_access_points(data, work);
    this->fill_ntfs_addresses(work);

    this->fill_lines(data, work);
    this->fill_line_groups(data, work);
    this->fill_routes(data, work);
    this->fill_validity_patterns(data, work);

    // the comments are loaded before the stop time (and thus the vj)
    // to reduce the memory foot print
    this->fill_comments(data, work);
    // the stop times are loaded before the vj as create_vj need the
    // list of stop times
    this->fill_shapes(data, work);
    this->fill_stop_times(data, work);
    this->fill_vehicle_journeys(data, work);
    this->finish_stop_times(data);

    /// grid calendar
    this->fill_calendars(data, work);
    this->fill_periods(data, work);
    this->fill_exception_dates(data, work);
    this->fill_rel_calendars_lines(data, work);

    /// meta vj associated calendars
    this->fill_associated_calendar(data, work);
    this->fill_meta_vehicle_journeys(data, work);

    this->fill_admins(data, work);
    this->fill_admins_postal_codes(data, work);
    this->fill_admin_stop_areas(data, work);
    this->fill_object_codes(data, work);

    //@TODO: les connections ont des doublons, en attendant que ce soit corrigé, on ne les enregistre pas
    this->fill_stop_point_connections(data, work);
    this->fill_poi_types(data, work);
    this->fill_pois(data, work);
    this->fill_poi_properties(data, work);
    this->fill_ways(data, work);
    this->fill_house_numbers(data, work);
    this->fill_vertex(data, work);
    this->fill_graph(data, work, export_georef_edges_geometries);

    // we need the proximity_list to build bss and parking edges
    data.geo_ref->build_proximity_list();
    this->fill_graph_bss(data, work);
    this->fill_graph_parking(data, work);

    // Charger les synonymes
    this->fill_synonyms(data, work);

    /// les relations admin et les autres objets
    this->build_rel_way_admin(data, work);
    this->build_rel_admin_admin(data, work);

    this->fill_prices(data, work);
    this->fill_transitions(data, work);
    this->fill_origin_destinations(data, work);

    check_coherence(data);
}

void EdReader::fill_admins(navitia::type::Data& nav_data, pqxx::work& work) {
    std::string request =
        "SELECT id, name, uri, comment, insee, level, ST_X(coord::geometry) as lon, "
        "ST_Y(coord::geometry) as lat, ST_asText(boundary) as boundary "
        "FROM georef.admin";

    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto* admin = new navitia::georef::Admin;
        const_it["comment"].to(admin->comment);
        const_it["uri"].to(admin->uri);
        const_it["name"].to(admin->name);
        const_it["insee"].to(admin->insee);
        const_it["level"].to(admin->level);
        admin->coord.set_lon(const_it["lon"].as<double>());
        admin->coord.set_lat(const_it["lat"].as<double>());

        if (!const_it["boundary"].is_null()) {
            boost::geometry::read_wkt(const_it["boundary"].as<std::string>(), admin->boundary);
        }

        admin->idx = nav_data.geo_ref->admins.size();

        nav_data.geo_ref->admins.push_back(admin);
        this->admin_map[const_it["id"].as<idx_t>()] = admin;

        admin_by_insee_code[admin->insee] = admin;
    }
}

void EdReader::fill_admins_postal_codes(navitia::type::Data& /*unused*/, pqxx::work& work) {
    std::string request = "select admin_id, postal_code from georef.postal_codes";
    size_t nb_unknown_admin(0);
    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto it_admin = admin_map.find(const_it["admin_id"].as<idx_t>());
        if (it_admin == admin_map.end()) {
            LOG4CPLUS_WARN(log, "impossible to find admin " << const_it["admin_id"] << " for postal code "
                                                            << const_it["postal_code"]);
            nb_unknown_admin++;
            continue;
        }
        it_admin->second->postal_codes.push_back(const_it["postal_code"].as<std::string>());
    }
    if (nb_unknown_admin) {
        LOG4CPLUS_WARN(log, nb_unknown_admin << " admin not found for postal codes");
    }
}

void EdReader::fill_admin_stop_areas(navitia::type::Data& /*unused*/, pqxx::work& work) {
    std::string request = "SELECT admin_id, stop_area_id from navitia.admin_stop_area";

    size_t nb_unknown_admin(0), nb_unknown_stop(0), nb_valid_admin(0);

    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto it_admin = admin_by_insee_code.find(const_it["admin_id"].as<std::string>());
        if (it_admin == admin_by_insee_code.end()) {
            LOG4CPLUS_TRACE(log, "impossible to find admin " << const_it["admin_id"]
                                                             << ", we cannot associate stop_area "
                                                             << const_it["stop_area_id"] << " to it");
            nb_unknown_admin++;
            continue;
        }
        navitia::georef::Admin* admin = it_admin->second;

        auto it_sa = stop_area_map.find(const_it["stop_area_id"].as<idx_t>());
        if (it_sa == stop_area_map.end()) {
            LOG4CPLUS_TRACE(log, "impossible to find stop_area " << const_it["stop_area_id"]
                                                                 << ", we cannot associate it to admin "
                                                                 << const_it["admin_id"]);
            nb_unknown_stop++;
            continue;
        }

        navitia::type::StopArea* sa = it_sa->second;

        admin->main_stop_areas.push_back(sa);
        nb_valid_admin++;
    }
    LOG4CPLUS_INFO(log, nb_valid_admin << " admin with at least one main stop");

    if (nb_unknown_admin) {
        LOG4CPLUS_WARN(log, nb_unknown_admin << " admin not found for admin main stops");
    }
    if (nb_unknown_stop) {
        LOG4CPLUS_WARN(log, nb_unknown_stop << " stops not found for admin main stops");
    }
}

// The pqxx::const_result_iterator is not always named the same, thus,
// we just use poor man's type inference.  Love the .template syntax.
template <typename Map, typename PqxxIt>
static void add_code(const Map& map, const PqxxIt& db_it, nt::Data& data) {
    auto search = map.find(db_it["object_id"].template as<idx_t>());
    if (search != map.end()) {
        data.pt_data->codes.add(search->second, db_it["key"].template as<std::string>(),
                                db_it["value"].template as<std::string>());
    }
}

void EdReader::fill_object_codes(navitia::type::Data& data, pqxx::work& work) {
    std::string request = "select object_type_id, object_id, key, value from navitia.object_code";

    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        nt::Type_e object_type_e = static_cast<nt::Type_e>(const_it["object_type_id"].as<int>());
        switch (object_type_e) {
            case nt::Type_e::StopArea:
                add_code(this->stop_area_map, const_it, data);
                break;
            case nt::Type_e::Network:
                add_code(this->network_map, const_it, data);
                break;
            case nt::Type_e::Company:
                add_code(this->company_map, const_it, data);
                break;
            case nt::Type_e::Line:
                add_code(this->line_map, const_it, data);
                break;
            case nt::Type_e::Route:
                add_code(this->route_map, const_it, data);
                break;
            case nt::Type_e::VehicleJourney:
                add_code(this->vehicle_journey_map, const_it, data);
                break;
            case nt::Type_e::StopPoint:
                add_code(this->stop_point_map, const_it, data);
                break;
            case nt::Type_e::Calendar:
                add_code(this->calendar_map, const_it, data);
                break;
            default:
                break;
        }
    }
}

void EdReader::fill_meta(navitia::type::Data& nav_data, pqxx::work& work) {
    std::string request =
        "SELECT beginning_date, end_date, st_astext(shape) as bounding_shape,"
        "street_network_source, poi_source  FROM navitia.parameters";
    pqxx::result result = work.exec(request);

    if (result.empty()) {
        throw navitia::exception(
            "Cannot find entry in navitia.parameters, "
            " it's likely that no data have been imported, we cannot create a nav file");
    }
    auto const_it = result.begin();
    if (const_it["beginning_date"].is_null() || const_it["end_date"].is_null()) {
        throw navitia::exception(
            "Cannot find beginning_date and end_date in navitia.parameters, "
            " it's likely that no gtfs data have been imported, we cannot create a nav file");
    }

    bg::date begin = bg::from_string(const_it["beginning_date"].as<std::string>());
    // we add a day because 'end' is not in the period (and we want it to be)
    bg::date end = bg::from_string(const_it["end_date"].as<std::string>()) + bg::days(1);

    nav_data.meta->production_date = bg::date_period(begin, end);
    if (!const_it["poi_source"].is_null()) {
        const_it["poi_source"].to(nav_data.meta->poi_source);
    }
    if (!const_it["street_network_source"].is_null()) {
        const_it["street_network_source"].to(nav_data.meta->street_network_source);
    }

    const_it["bounding_shape"].to(nav_data.meta->shape);
}

void EdReader::fill_feed_infos(navitia::type::Data& data, pqxx::work& work) {
    std::string request = "SELECT key, value FROM navitia.feed_info";

    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        if (const_it["key"].as<std::string>() == "feed_publisher_name") {
            data.meta->publisher_name = const_it["value"].as<std::string>();
        }
        if (const_it["key"].as<std::string>() == "feed_publisher_url") {
            data.meta->publisher_url = const_it["value"].as<std::string>();
        }
        if (const_it["key"].as<std::string>() == "feed_license") {
            data.meta->license = const_it["value"].as<std::string>();
        }
        if (const_it["key"].as<std::string>() == "feed_creation_datetime" && !const_it["value"].is_null()) {
            try {
                data.meta->dataset_created_at = bt::from_iso_string(const_it["value"].as<std::string>());
            } catch (const std::out_of_range&) {
                LOG4CPLUS_INFO(log, "feed_creation_datetime is not valid");
            }
        }
    }
}

void EdReader::fill_timezones(navitia::type::Data& data, pqxx::work& work) {
    std::string request =
        "SELECT tz.id as id,"
        " tz.name as name,"
        " dst.beginning_date as beg,"
        " dst.end_date as end,"
        " dst.utc_offset as offset"
        " FROM navitia.timezone as tz"
        " JOIN navitia.tz_dst as dst ON dst.tz_id = tz.id";

    pqxx::result result = work.exec(request);
    std::map<std::string, idx_t> id_by_name;
    std::map<std::string, navitia::type::TimeZoneHandler::dst_periods> timezones;
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        bg::date begin = bg::from_string(const_it["beg"].as<std::string>());
        // we add a day because 'end' is not in the period (and we want it to be)
        bg::date end = bg::from_string(const_it["end"].as<std::string>()) + bg::days(1);
        auto name = const_it["name"].as<std::string>();
        auto utc_offset = const_it["offset"].as<int32_t>();
        timezones[name][utc_offset].push_back(bg::date_period(begin, end));

        auto id = const_it["id"].as<idx_t>();
        id_by_name[name] = id;
    }

    for (const auto& p : timezones) {
        auto tz = data.pt_data->tz_manager.get_or_create(p.first, data.meta->production_date.begin(), p.second);
        timezone_map[id_by_name[p.first]] = tz;
    }
}

void EdReader::fill_networks(nt::Data& data, pqxx::work& work) {
    std::string request = "SELECT id, name, uri, sort, website FROM navitia.network";

    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto* network = new nt::Network();
        const_it["uri"].to(network->uri);
        const_it["name"].to(network->name);
        const_it["sort"].to(network->sort);
        const_it["website"].to(network->website);
        network->idx = data.pt_data->networks.size();

        data.pt_data->networks.push_back(network);
        this->network_map[const_it["id"].as<idx_t>()] = network;
    }
}

void EdReader::fill_commercial_modes(nt::Data& data, pqxx::work& work) {
    std::string request = "SELECT id, name, uri FROM navitia.commercial_mode";

    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto* mode = new nt::CommercialMode();
        const_it["uri"].to(mode->uri);
        const_it["name"].to(mode->name);

        mode->idx = data.pt_data->commercial_modes.size();

        data.pt_data->commercial_modes.push_back(mode);
        this->commercial_mode_map[const_it["id"].as<idx_t>()] = mode;
    }
}

void EdReader::fill_physical_modes(nt::Data& data, pqxx::work& work) {
    std::string request = "SELECT id, name, uri, co2_emission FROM navitia.physical_mode";

    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto* mode = new nt::PhysicalMode();
        const_it["uri"].to(mode->uri);
        const_it["name"].to(mode->name);
        if (!const_it["co2_emission"].is_null()) {
            mode->co2_emission = const_it["co2_emission"].as<double>();
        }
        mode->idx = data.pt_data->physical_modes.size();

        data.pt_data->physical_modes.push_back(mode);
        this->physical_mode_map[const_it["id"].as<idx_t>()] = mode;
    }
}

void EdReader::fill_contributors(nt::Data& data, pqxx::work& work) {
    std::string request = "SELECT id, name, uri, website, license FROM navitia.contributor";

    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto* contributor = new nt::Contributor();
        const_it["uri"].to(contributor->uri);
        const_it["name"].to(contributor->name);
        const_it["website"].to(contributor->website);
        const_it["license"].to(contributor->license);

        contributor->idx = data.pt_data->contributors.size();

        data.pt_data->contributors.push_back(contributor);
        this->contributor_map[const_it["id"].as<idx_t>()] = contributor;
    }
}

void EdReader::fill_datasets(nt::Data& data, pqxx::work& work) {
    std::string request =
        "SELECT id, uri, description, system, start_date, end_date, "
        "contributor_id FROM navitia.dataset";
    size_t nb_unknown_contributor(0);
    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto contributor_it = this->contributor_map.find(const_it["contributor_id"].as<idx_t>());
        if (contributor_it == this->contributor_map.end()) {
            LOG4CPLUS_TRACE(log, "impossible to find contributor" << const_it["contributor_id"]
                                                                  << ", we cannot assoicate it to dataset "
                                                                  << const_it["uri"]);
            nb_unknown_contributor++;
            continue;
        }

        auto* dataset = new nt::Dataset();
        const_it["uri"].to(dataset->uri);
        const_it["description"].to(dataset->desc);
        const_it["system"].to(dataset->system);
        boost::gregorian::date start(bg::from_string(const_it["start_date"].as<std::string>()));
        boost::gregorian::date end(bg::from_string(const_it["end_date"].as<std::string>()));
        dataset->validation_period = bg::date_period(start, end);
        dataset->contributor = contributor_it->second;
        dataset->idx = data.pt_data->datasets.size();

        dataset->contributor->dataset_list.insert(dataset);
        data.pt_data->datasets.push_back(dataset);
        this->dataset_map[const_it["id"].as<idx_t>()] = dataset;
    }
    if (nb_unknown_contributor) {
        LOG4CPLUS_WARN(log, nb_unknown_contributor << "contributor not found for dataset");
    }
}

void EdReader::fill_companies(nt::Data& data, pqxx::work& work) {
    std::string request = "SELECT id, name, uri, website FROM navitia.company";

    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto* company = new nt::Company();
        const_it["uri"].to(company->uri);
        const_it["name"].to(company->name);
        const_it["website"].to(company->website);

        company->idx = data.pt_data->companies.size();

        data.pt_data->companies.push_back(company);
        this->company_map[const_it["id"].as<idx_t>()] = company;
    }
}

void EdReader::fill_stop_areas(nt::Data& data, pqxx::work& work) {
    std::string request =
        "SELECT sa.id as id, sa.name as name, sa.uri as uri, "
        "sa.visible as visible, sa.timezone as timezone, "
        "ST_X(sa.coord::geometry) as lon, ST_Y(sa.coord::geometry) as lat,"
        "pr.wheelchair_boarding as wheelchair_boarding, pr.sheltered as sheltered,"
        "pr.elevator as elevator, pr.escalator as escalator,"
        "pr.bike_accepted as bike_accepted, pr.bike_depot as bike_depot,"
        "pr.visual_announcement as visual_announcement,"
        "pr.audible_announcement as audible_announcement,"
        "pr.appropriate_escort as appropriate_escort,"
        "pr.appropriate_signage as appropriate_signage "
        "FROM navitia.stop_area as sa, navitia.properties  as pr "
        "where sa.properties_id=pr.id ";

    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto* sa = new nt::StopArea();
        const_it["uri"].to(sa->uri);
        const_it["name"].to(sa->name);
        const_it["timezone"].to(sa->timezone);
        sa->coord.set_lon(const_it["lon"].as<double>());
        sa->coord.set_lat(const_it["lat"].as<double>());
        sa->visible = const_it["visible"].as<bool>();
        if (const_it["wheelchair_boarding"].as<bool>()) {
            sa->set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);
        }
        if (const_it["sheltered"].as<bool>()) {
            sa->set_property(navitia::type::hasProperties::SHELTERED);
        }
        if (const_it["elevator"].as<bool>()) {
            sa->set_property(navitia::type::hasProperties::ELEVATOR);
        }
        if (const_it["escalator"].as<bool>()) {
            sa->set_property(navitia::type::hasProperties::ESCALATOR);
        }
        if (const_it["bike_accepted"].as<bool>()) {
            sa->set_property(navitia::type::hasProperties::BIKE_ACCEPTED);
        }
        if (const_it["bike_depot"].as<bool>()) {
            sa->set_property(navitia::type::hasProperties::BIKE_DEPOT);
        }
        if (const_it["visual_announcement"].as<bool>()) {
            sa->set_property(navitia::type::hasProperties::VISUAL_ANNOUNCEMENT);
        }
        if (const_it["audible_announcement"].as<bool>()) {
            sa->set_property(navitia::type::hasProperties::AUDIBLE_ANNOUNVEMENT);
        }
        if (const_it["appropriate_escort"].as<bool>()) {
            sa->set_property(navitia::type::hasProperties::APPOPRIATE_ESCORT);
        }
        if (const_it["appropriate_signage"].as<bool>()) {
            sa->set_property(navitia::type::hasProperties::APPOPRIATE_SIGNAGE);
        }

        sa->idx = data.pt_data->stop_areas.size();

        data.pt_data->stop_areas.push_back(sa);
        this->stop_area_map[const_it["id"].as<idx_t>()] = sa;
        this->uri_to_idx_stop_area[const_it["uri"].as<std::string>()] = const_it["id"].as<idx_t>();
    }
}

void EdReader::fill_stop_points(nt::Data& data, pqxx::work& work) {
    std::string request =
        "SELECT sp.id as id, sp.name as name, sp.uri as uri, "
        "ST_X(sp.coord::geometry) as lon, ST_Y(sp.coord::geometry) as lat,"
        "sp.fare_zone as fare_zone, sp.stop_area_id as stop_area_id,"
        "sp.platform_code as platform_code,"
        "sp.is_zonal as is_zonal,"
        "sp.address_id as address_id,"
        "ST_AsText(sp.area) as area,"
        "pr.wheelchair_boarding as wheelchair_boarding,"
        "pr.sheltered as sheltered, pr.elevator as elevator,"
        "pr.escalator as escalator, pr.bike_accepted as bike_accepted,"
        "pr.bike_depot as bike_depot,"
        "pr.visual_announcement as visual_announcement,"
        "pr.audible_announcement as audible_announcement,"
        "pr.appropriate_escort as appropriate_escort,"
        "pr.appropriate_signage as appropriate_signage "
        "FROM navitia.stop_point as sp, navitia.properties  as pr "
        "where sp.properties_id=pr.id";

    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto* sp = new nt::StopPoint();
        const_it["uri"].to(sp->uri);
        const_it["name"].to(sp->name);
        const_it["fare_zone"].to(sp->fare_zone);
        const_it["platform_code"].to(sp->platform_code);
        const_it["is_zonal"].to(sp->is_zonal);
        const_it["address_id"].to(sp->address_id);
        sp->coord.set_lon(const_it["lon"].as<double>());
        sp->coord.set_lat(const_it["lat"].as<double>());
        if (const_it["wheelchair_boarding"].as<bool>()) {
            sp->set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);
        }
        if (const_it["sheltered"].as<bool>()) {
            sp->set_property(navitia::type::hasProperties::SHELTERED);
        }
        if (const_it["elevator"].as<bool>()) {
            sp->set_property(navitia::type::hasProperties::ELEVATOR);
        }
        if (const_it["escalator"].as<bool>()) {
            sp->set_property(navitia::type::hasProperties::ESCALATOR);
        }
        if (const_it["bike_accepted"].as<bool>()) {
            sp->set_property(navitia::type::hasProperties::BIKE_ACCEPTED);
        }
        if (const_it["bike_depot"].as<bool>()) {
            sp->set_property(navitia::type::hasProperties::BIKE_DEPOT);
        }
        if (const_it["visual_announcement"].as<bool>()) {
            sp->set_property(navitia::type::hasProperties::VISUAL_ANNOUNCEMENT);
        }
        if (const_it["audible_announcement"].as<bool>()) {
            sp->set_property(navitia::type::hasProperties::AUDIBLE_ANNOUNVEMENT);
        }
        if (const_it["appropriate_escort"].as<bool>()) {
            sp->set_property(navitia::type::hasProperties::APPOPRIATE_ESCORT);
        }
        if (const_it["appropriate_signage"].as<bool>()) {
            sp->set_property(navitia::type::hasProperties::APPOPRIATE_SIGNAGE);
        }
        sp->stop_area = stop_area_map[const_it["stop_area_id"].as<idx_t>()];
        sp->stop_area->stop_point_list.push_back(sp);
        if (!const_it["area"].is_null() && sp->is_zonal) {
            nt::MultiPolygon area;
            boost::geometry::read_wkt(const_it["area"].as<std::string>(), area);
            data.pt_data->add_stop_point_area(area, sp);
        }
        data.pt_data->stop_points.push_back(sp);
        this->stop_point_map[const_it["id"].as<idx_t>()] = sp;
        this->uri_to_idx_stop_point[const_it["uri"].as<std::string>()] = const_it["id"].as<idx_t>();
    }
}

void EdReader::fill_access_point_field(navitia::type::AccessPoint* access_point,
                                       const pqxx::result::iterator const_it,
                                       const bool from_access_point,
                                       const std::string& sp_id) {
    if (!const_it["pathway_mode"].is_null()) {
        access_point->pathway_mode = const_it["pathway_mode"].as<unsigned int>();
    }
    if (!const_it["is_bidirectional"].is_null()) {
        const bool is_bidirectional = const_it["is_bidirectional"].as<bool>();
        if (is_bidirectional) {
            access_point->is_entrance = true;
            access_point->is_exit = true;
        } else {
            if (from_access_point) {
                access_point->is_entrance = true;
                access_point->is_exit = false;
            } else {
                access_point->is_entrance = false;
                access_point->is_exit = true;
            }
        }
    }
    if (!const_it["length"].is_null()) {
        access_point->length = const_it["length"].as<unsigned int>();
    }
    if (!const_it["traversal_time"].is_null()) {
        access_point->traversal_time = const_it["traversal_time"].as<unsigned int>();
    }
    if (!const_it["stair_count"].is_null()) {
        access_point->stair_count = const_it["stair_count"].as<unsigned int>();
    }
    if (!const_it["max_slope"].is_null()) {
        access_point->max_slope = const_it["max_slope"].as<unsigned int>();
    }
    if (!const_it["min_width"].is_null()) {
        access_point->min_width = const_it["min_width"].as<unsigned int>();
    }
    if (!const_it["signposted_as"].is_null()) {
        const_it["signposted_as"].to(access_point->signposted_as);
    }
    if (!const_it["reversed_signposted_as"].is_null()) {
        const_it["reversed_signposted_as"].to(access_point->reversed_signposted_as);
    }
    // link with SP
    auto sp_key = uri_to_idx_stop_point.find("stop_point:" + sp_id);
    if (sp_key != uri_to_idx_stop_point.end()) {
        auto sp = stop_point_map.find(sp_key->second);
        if (sp != stop_point_map.end()) {
            sp->second->access_points.insert(access_point);
        }
    } else {
        LOG4CPLUS_ERROR(log, "pathway.to_stop_id not match with a stop point uri " << sp_id);
    }
}

void EdReader::fill_access_points(nt::Data& data, pqxx::work& work) {
    // access_point
    std::string request =
        "SELECT id, name, uri, "
        "ST_X(coord::geometry) as lon, ST_Y(coord::geometry) as lat, "
        "stop_code, "
        "parent_station "
        "FROM navitia.access_point";

    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto* ap = new nt::AccessPoint();
        const_it["uri"].to(ap->uri);
        const_it["name"].to(ap->name);
        const_it["stop_code"].to(ap->stop_code);
        ap->coord.set_lon(const_it["lon"].as<double>());
        ap->coord.set_lat(const_it["lat"].as<double>());
        // parent station is Stop Area
        auto sa_key = uri_to_idx_stop_area.find("stop_area:" + const_it["parent_station"].as<std::string>());
        if (sa_key != uri_to_idx_stop_area.end()) {
            auto sa = stop_area_map.find(sa_key->second);
            if (sa != stop_area_map.end()) {
                ap->parent_station = sa->second;
            }
        } else {
            LOG4CPLUS_ERROR(log, "access_point.parent_station not match with a stop area uri "
                                     << const_it["parent_station"].as<std::string>());
        }

        // store access_point temporarily before finishing in a SP link
        access_point_map[const_it["uri"].as<std::string>()] = ap;
    }

    // Pathway
    request =
        "SELECT id, name, uri, "
        "from_stop_id, "
        "to_stop_id, "
        "pathway_mode, "
        "is_bidirectional, "
        "length, "
        "traversal_time, "
        "stair_count, "
        "max_slope, "
        "min_width, "
        "signposted_as, "
        "reversed_signposted_as "
        "FROM navitia.pathway";

    result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        std::string from_stop_id;
        const_it["from_stop_id"].to(from_stop_id);
        std::string to_stop_id;
        const_it["to_stop_id"].to(to_stop_id);

        // Access Point URI match for from_stop_id
        // so, to_stop_id have to be a StopPoint
        auto from_access_p = access_point_map.find("access_point:" + from_stop_id);
        if (from_access_p != access_point_map.end()) {
            fill_access_point_field(from_access_p->second, const_it, true, to_stop_id);
            continue;
        }
        // Access Point URI match for to_stop_id
        // so, from_stop_id have to be a StopPoint
        auto to_access_p = access_point_map.find("access_point:" + to_stop_id);
        if (to_access_p != access_point_map.end()) {
            fill_access_point_field(to_access_p->second, const_it, false, from_stop_id);
            continue;
        }

        // An other case exists: StopPoint <=> StopPoint connection.
        // In the future, it will be handled like AccessPoint <=> SP
    }
}

void EdReader::fill_ntfs_addresses(pqxx::work& work) {
    std::string request = "SELECT id, house_number, street_name FROM navitia.address";
    const pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto* addr = new navitia::type::Address();
        const_it["id"].to(addr->id);
        const_it["house_number"].to(addr->house_number);
        const_it["street_name"].to(addr->street_name);
        this->address_by_address_id[const_it["id"].as<std::string>()] = addr;
    }
}

void EdReader::fill_lines(nt::Data& data, pqxx::work& work) {
    std::string request =
        "SELECT id, name, uri, code, color, text_color,"
        "network_id, commercial_mode_id, sort, ST_AsText(shape) AS shape, "
        "opening_time, closing_time "
        "FROM navitia.line";

    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto* line = new nt::Line();
        const_it["uri"].to(line->uri);
        const_it["name"].to(line->name);
        const_it["code"].to(line->code);
        const_it["color"].to(line->color);
        const_it["text_color"].to(line->text_color);
        const_it["sort"].to(line->sort);
        if (!const_it["opening_time"].is_null()) {
            line->opening_time = boost::posix_time::duration_from_string(const_it["opening_time"].as<std::string>());
        }
        if (!const_it["closing_time"].is_null()) {
            line->closing_time = boost::posix_time::duration_from_string(const_it["closing_time"].as<std::string>());
        }

        line->network = network_map[const_it["network_id"].as<idx_t>()];
        line->network->line_list.push_back(line);

        line->commercial_mode = commercial_mode_map[const_it["commercial_mode_id"].as<idx_t>()];
        line->commercial_mode->line_list.push_back(line);

        boost::geometry::read_wkt(const_it["shape"].as<std::string>("MULTILINESTRING()"), line->shape);

        data.pt_data->lines.push_back(line);
        this->line_map[const_it["id"].as<idx_t>()] = line;
    }

    // Add Object properties on lines
    std::string properties_request =
        "SELECT object_id, property_name, property_value FROM navitia.object_properties WHERE object_type = 'line'";

    pqxx::result property_result = work.exec(properties_request);
    for (auto const_it = property_result.begin(); const_it != property_result.end(); ++const_it) {
        auto line_it = this->line_map.find(const_it["object_id"].as<idx_t>());
        if (line_it != this->line_map.end()) {
            line_it->second->properties[const_it["property_name"].as<std::string>()] =
                const_it["property_value"].as<std::string>();
        }
    }
}

void EdReader::fill_line_groups(nt::Data& data, pqxx::work& work) {
    std::string request = "SELECT id, uri, name, main_line_id FROM navitia.line_group";

    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto* line_group = new nt::LineGroup();
        const_it["uri"].to(line_group->uri);
        const_it["name"].to(line_group->name);
        line_group->main_line = this->line_map[const_it["main_line_id"].as<idx_t>()];
        this->line_group_map[const_it["id"].as<idx_t>()] = line_group;
        data.pt_data->line_groups.push_back(line_group);
    }

    pqxx::result line_group_result = work.exec("SELECT group_id, line_id FROM navitia.line_group_link");
    for (auto const_it = line_group_result.begin(); const_it != line_group_result.end(); ++const_it) {
        auto group_it = this->line_group_map.find(const_it["group_id"].as<idx_t>());
        if (group_it != this->line_group_map.end()) {
            auto line_it = this->line_map.find(const_it["line_id"].as<idx_t>());
            if (line_it != this->line_map.end()) {
                group_it->second->line_list.push_back(line_it->second);
                line_it->second->line_group_list.push_back(group_it->second);
            }
        }
    }
}

void EdReader::fill_routes(nt::Data& data, pqxx::work& work) {
    std::string request =
        "SELECT id, name, uri, line_id, destination_stop_area_id,"
        "ST_AsText(shape) AS shape, direction_type FROM navitia.route";

    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto* route = new nt::Route();
        const_it["uri"].to(route->uri);
        const_it["name"].to(route->name);
        const_it["direction_type"].to(route->direction_type);
        boost::geometry::read_wkt(const_it["shape"].as<std::string>("MULTILINESTRING()"), route->shape);

        route->line = line_map[const_it["line_id"].as<idx_t>()];
        route->line->route_list.push_back(route);

        if (!const_it["destination_stop_area_id"].is_null()) {
            route->destination = stop_area_map[const_it["destination_stop_area_id"].as<idx_t>()];
        }

        data.pt_data->routes.push_back(route);
        this->route_map[const_it["id"].as<idx_t>()] = route;
    }
}

void EdReader::fill_validity_patterns(nt::Data& data, pqxx::work& work) {
    std::string request = "SELECT id, days FROM navitia.validity_pattern";

    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        nt::ValidityPattern* validity_pattern = nullptr;
        validity_pattern =
            new nt::ValidityPattern(data.meta->production_date.begin(), const_it["days"].as<std::string>());

        validity_pattern->idx = data.pt_data->validity_patterns.size();

        data.pt_data->validity_patterns.push_back(validity_pattern);
        this->validity_pattern_map[const_it["id"].as<idx_t>()] = validity_pattern;
    }
}

void EdReader::fill_stop_point_connections(nt::Data& data, pqxx::work& work) {
    std::string request =
        "SELECT conn.departure_stop_point_id as departure_stop_point_id,"
        "conn.destination_stop_point_id as destination_stop_point_id,"
        "conn.connection_type_id as connection_type_id,"
        "conn.display_duration as display_duration, conn.duration as duration, "
        "conn.max_duration as max_duration,"
        "pr.wheelchair_boarding as wheelchair_boarding,"
        "pr.sheltered as sheltered, pr.elevator as elevator,"
        "pr.escalator as escalator, pr.bike_accepted as bike_accepted,"
        "pr.bike_depot as bike_depot,"
        "pr.visual_announcement as visual_announcement,"
        "pr.audible_announcement as audible_announcement,"
        "pr.appropriate_escort as appropriate_escort,"
        "pr.appropriate_signage as appropriate_signage "
        "FROM navitia.connection as conn, navitia.properties  as pr "
        "where conn.properties_id=pr.id ";

    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto it_departure = stop_point_map.find(const_it["departure_stop_point_id"].as<idx_t>());
        auto it_destination = stop_point_map.find(const_it["destination_stop_point_id"].as<idx_t>());
        if (it_departure != stop_point_map.end() && it_destination != stop_point_map.end()) {
            auto* stop_point_connection = new nt::StopPointConnection();
            stop_point_connection->departure = it_departure->second;
            stop_point_connection->destination = it_destination->second;
            stop_point_connection->connection_type =
                static_cast<nt::ConnectionType>(const_it["connection_type_id"].as<int>());
            stop_point_connection->display_duration = const_it["display_duration"].as<int>();
            stop_point_connection->duration = const_it["duration"].as<int>();
            stop_point_connection->max_duration = const_it["max_duration"].as<int>();

            if (const_it["wheelchair_boarding"].as<bool>()) {
                stop_point_connection->set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);
            }
            if (const_it["sheltered"].as<bool>()) {
                stop_point_connection->set_property(navitia::type::hasProperties::SHELTERED);
            }
            if (const_it["elevator"].as<bool>()) {
                stop_point_connection->set_property(navitia::type::hasProperties::ELEVATOR);
            }
            if (const_it["escalator"].as<bool>()) {
                stop_point_connection->set_property(navitia::type::hasProperties::ESCALATOR);
            }
            if (const_it["bike_accepted"].as<bool>()) {
                stop_point_connection->set_property(navitia::type::hasProperties::BIKE_ACCEPTED);
            }
            if (const_it["bike_depot"].as<bool>()) {
                stop_point_connection->set_property(navitia::type::hasProperties::BIKE_DEPOT);
            }
            if (const_it["visual_announcement"].as<bool>()) {
                stop_point_connection->set_property(navitia::type::hasProperties::VISUAL_ANNOUNCEMENT);
            }
            if (const_it["audible_announcement"].as<bool>()) {
                stop_point_connection->set_property(navitia::type::hasProperties::AUDIBLE_ANNOUNVEMENT);
            }
            if (const_it["appropriate_escort"].as<bool>()) {
                stop_point_connection->set_property(navitia::type::hasProperties::APPOPRIATE_ESCORT);
            }
            if (const_it["appropriate_signage"].as<bool>()) {
                stop_point_connection->set_property(navitia::type::hasProperties::APPOPRIATE_SIGNAGE);
            }

            data.pt_data->stop_point_connections.push_back(stop_point_connection);

            // add the connection in the stop points
            stop_point_connection->departure->stop_point_connection_list.push_back(stop_point_connection);
            stop_point_connection->destination->stop_point_connection_list.push_back(stop_point_connection);
        }
    }
}

void EdReader::fill_vehicle_journeys(nt::Data& data, pqxx::work& work) {
    std::string request =
        "SELECT vj.id as id, vj.name as name, vj.uri as uri,"
        "vj.company_id as company_id, "
        "vj.validity_pattern_id as validity_pattern_id,"
        "vj.physical_mode_id as physical_mode_id,"
        "vj.route_id as route_id,"
        "vj.adapted_validity_pattern_id as adapted_validity_pattern_id,"
        "vj.theoric_vehicle_journey_id as theoric_vehicle_journey_id ,"
        "vj.odt_type_id as odt_type_id, vj.odt_message as odt_message,"
        "vj.next_vehicle_journey_id as next_vj_id,"
        "vj.previous_vehicle_journey_id as prev_vj_id,"
        "vj.start_time as start_time,"
        "vj.end_time as end_time,"
        "vj.headway_sec as headway_sec,"
        "vj.is_frequency as is_frequency, "
        "vj.meta_vj_name as meta_vj_name, "
        "vj.vj_class as vj_class, "
        "vj.dataset_id as dataset_id, "
        "vj.headsign as headsign, "
        "vp.wheelchair_accessible as wheelchair_accessible,"
        "vp.bike_accepted as bike_accepted,"
        "vp.air_conditioned as air_conditioned,"
        "vp.visual_announcement as visual_announcement,"
        "vp.audible_announcement as audible_announcement,"
        "vp.appropriate_escort as appropriate_escort,"
        "vp.appropriate_signage as appropriate_signage,"
        "vp.school_vehicle as school_vehicle "
        "FROM navitia.vehicle_journey as vj, navitia.vehicle_properties as vp "
        "WHERE vj.vehicle_properties_id = vp.id";

    pqxx::result result = work.exec(request);
    std::multimap<idx_t, nt::VehicleJourney*> prev_vjs, next_vjs;
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto* route = route_map[const_it["route_id"].as<idx_t>()];
        navitia::type::VehicleJourney* vj = nullptr;
        std::string vj_name;
        const_it["name"].to(vj_name);
        std::string mvj_name;
        const_it["meta_vj_name"].to(mvj_name);
        if (mvj_name.empty()) {
            mvj_name = vj_name;
        }

        auto mvj = data.pt_data->meta_vjs.get_or_create(mvj_name);
        auto rt_level = navitia::type::get_rt_level_from_string(const_it["vj_class"].as<std::string>());
        const auto& vp = *validity_pattern_map[const_it["validity_pattern_id"].as<idx_t>()];
        const auto uri = const_it["uri"].as<std::string>();
        const auto name = const_it["name"].as<std::string>();
        const auto headsign = const_it["headsign"].as<std::string>();

        const auto vj_id = const_it["id"].as<idx_t>();
        if (const_it["is_frequency"].as<bool>()) {
            auto f_vj = mvj->create_frequency_vj(uri, name, headsign, rt_level, vp, route,
                                                 std::move(sts_from_vj[vj_id]), *data.pt_data);
            const_it["start_time"].to(f_vj->start_time);
            const_it["end_time"].to(f_vj->end_time);
            const_it["headway_sec"].to(f_vj->headway_secs);
            vj = f_vj;
        } else {
            vj = mvj->create_discrete_vj(uri, name, headsign, rt_level, vp, route, std::move(sts_from_vj[vj_id]),
                                         *data.pt_data);
        }
        const_it["odt_message"].to(vj->odt_message);
        // TODO ODT NTFSv0.3: remove that when we stop to support NTFSv0.1
        vj->vehicle_journey_type = static_cast<nt::VehicleJourneyType>(const_it["odt_type_id"].as<int>());
        vj->physical_mode = physical_mode_map[const_it["physical_mode_id"].as<idx_t>()];

        vj->company = company_map[const_it["company_id"].as<idx_t>()];
        assert(vj->company);
        assert(vj->route);

        if (vj->route && vj->route->line) {
            if (boost::range::find(vj->route->line->company_list, vj->company) == vj->route->line->company_list.end()) {
                vj->route->line->company_list.push_back(vj->company);
            }
            if (boost::range::find(vj->company->line_list, vj->route->line) == vj->company->line_list.end()) {
                vj->company->line_list.push_back(vj->route->line);
            }
        }

        if (const_it["wheelchair_accessible"].as<bool>()) {
            vj->set_vehicle(navitia::type::hasVehicleProperties::WHEELCHAIR_ACCESSIBLE);
        }
        if (const_it["bike_accepted"].as<bool>()) {
            vj->set_vehicle(navitia::type::hasVehicleProperties::BIKE_ACCEPTED);
        }
        if (const_it["air_conditioned"].as<bool>()) {
            vj->set_vehicle(navitia::type::hasVehicleProperties::AIR_CONDITIONED);
        }
        if (const_it["visual_announcement"].as<bool>()) {
            vj->set_vehicle(navitia::type::hasVehicleProperties::VISUAL_ANNOUNCEMENT);
        }
        if (const_it["audible_announcement"].as<bool>()) {
            vj->set_vehicle(navitia::type::hasVehicleProperties::AUDIBLE_ANNOUNCEMENT);
        }
        if (const_it["appropriate_escort"].as<bool>()) {
            vj->set_vehicle(navitia::type::hasVehicleProperties::APPOPRIATE_ESCORT);
        }
        if (const_it["appropriate_signage"].as<bool>()) {
            vj->set_vehicle(navitia::type::hasVehicleProperties::APPOPRIATE_SIGNAGE);
        }
        if (const_it["school_vehicle"].as<bool>()) {
            vj->set_vehicle(navitia::type::hasVehicleProperties::SCHOOL_VEHICLE);
        }
        if (!const_it["prev_vj_id"].is_null()) {
            prev_vjs.insert(std::make_pair(const_it["prev_vj_id"].as<idx_t>(), vj));
        }
        if (!const_it["next_vj_id"].is_null()) {
            next_vjs.insert(std::make_pair(const_it["next_vj_id"].as<idx_t>(), vj));
        }

        data.pt_data->headsign_handler.change_name_and_register_as_headsign(*vj, vj->headsign);
        vehicle_journey_map[vj_id] = vj;

        // we check if we have some comments
        const auto& it_comments = vehicle_journey_comments.find(vj_id);
        if (it_comments != vehicle_journey_comments.end()) {
            for (const auto& comment : it_comments->second) {
                data.pt_data->comments.add(vj, comment);
            }
        }
        if (!const_it["dataset_id"].is_null()) {
            auto dataset_it = this->dataset_map.find(const_it["dataset_id"].as<idx_t>());
            if (dataset_it != this->dataset_map.end()) {
                vj->dataset = dataset_it->second;
                vj->dataset->vehiclejourney_list.insert(vj);
            }
        }
    }

    for (auto vjid_vj : prev_vjs) {
        vjid_vj.second->prev_vj = vehicle_journey_map[vjid_vj.first];
    }
    for (auto vjid_vj : next_vjs) {
        vjid_vj.second->next_vj = vehicle_journey_map[vjid_vj.first];
    }
    release(sts_from_vj);
}

void EdReader::fill_associated_calendar(nt::Data& data, pqxx::work& work) {
    // fill associated_calendar
    std::string request = "SELECT id, calendar_id from navitia.associated_calendar";
    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        nt::AssociatedCalendar* associated_calendar;
        int associated_calendar_idx = const_it["id"].as<idx_t>();

        int calendar_idx = const_it["calendar_id"].as<idx_t>();
        const auto calendar_it = this->calendar_map.find(calendar_idx);
        if (calendar_it == this->calendar_map.end()) {
            LOG4CPLUS_ERROR(log,
                            "Impossible to find the calendar " << calendar_idx << ", we won't add associated calendar");
            continue;
        }

        associated_calendar = new nt::AssociatedCalendar();
        associated_calendar->calendar = calendar_it->second;
        data.pt_data->associated_calendars.push_back(associated_calendar);
        this->associated_calendar_map[associated_calendar_idx] = associated_calendar;
    }

    // then we fill the links
    request =
        "SELECT l.datetime as datetime, l.type_ex as type_ex,"
        " calendar.id as id, calendar.calendar_id as calendar_id"
        " from navitia.associated_calendar as calendar, navitia.associated_exception_date as l"
        " WHERE calendar.id = l.associated_calendar_id ORDER BY calendar.id";
    result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        int associated_calendar_idx = const_it["id"].as<idx_t>();

        const auto associated_calendar_it = this->associated_calendar_map.find(associated_calendar_idx);

        if (associated_calendar_it == this->associated_calendar_map.end()) {
            LOG4CPLUS_ERROR(log, "Impossible to find the associated calendar " << associated_calendar_idx
                                                                               << ", we won't add exception date");
            continue;
        }

        nt::ExceptionDate exception_date;
        exception_date.date = bg::from_string(const_it["datetime"].as<std::string>());
        exception_date.type = navitia::type::to_exception_type(const_it["type_ex"].as<std::string>());

        auto associated_calendar = associated_calendar_it->second;
        associated_calendar->exceptions.push_back(exception_date);
    }
}

void EdReader::fill_meta_vehicle_journeys(nt::Data& data, pqxx::work& work) {
    // then we fill the links between a metavj, its associated calendars and it's timezone
    const auto request =
        "SELECT meta.name as name, "
        "meta.timezone as timezone, "
        "rel.associated_calendar_id as associated_calendar_id, "
        "c.uri as associated_calendar_name "
        "from navitia.meta_vj as meta "
        "LEFT JOIN navitia.rel_metavj_associated_calendar as rel ON rel.meta_vj_id = meta.id "
        "LEFT JOIN navitia.associated_calendar as l2 ON rel.associated_calendar_id = l2.id "
        "LEFT JOIN navitia.calendar as c ON c.id = l2.calendar_id";

    const pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        const std::string name = const_it["name"].as<std::string>();
        nt::MetaVehicleJourney* meta_vj = data.pt_data->meta_vjs.get_mut(name);
        if (meta_vj == nullptr) {
            throw navitia::exception("impossible to find metavj " + name + " data are not valid");
        }

        if (!const_it["associated_calendar_id"].is_null()) {
            int associated_calendar_idx = const_it["associated_calendar_id"].as<idx_t>();
            nt::AssociatedCalendar* associated_calendar;
            auto it_ac = this->associated_calendar_map.find(associated_calendar_idx);
            if (it_ac == this->associated_calendar_map.end()) {
                LOG4CPLUS_ERROR(log, "Impossible to find the associated calendar " << associated_calendar_idx
                                                                                   << ", we won't add it to meta vj");
            } else {
                associated_calendar = it_ac->second;

                meta_vj->associated_calendars[const_it["associated_calendar_name"].as<std::string>()] =
                    associated_calendar;
            }
        }

        const auto tz_id = const_it["timezone"].as<idx_t>();
        const auto* tz = find_or_default(tz_id, timezone_map);
        if (!tz) {
            throw navitia::exception("impossible to find timezone " + std::to_string(tz_id)
                                     + " data is in an invalid state");
        }
        meta_vj->tz_handler = tz;
    }
}

void EdReader::fill_shapes(nt::Data& /*unused*/, pqxx::work& work) {
    std::string request = "SELECT id as id, ST_AsText(geom) as geom FROM navitia.shape";
    const pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto shape = boost::make_shared<nt::LineString>();
        boost::geometry::read_wkt(const_it["geom"].as<std::string>("LINESTRING()"), *shape);
        this->shapes_map[const_it["id"].as<idx_t>()] = shape;
    }
}

void EdReader::fill_stop_times(nt::Data& /*unused*/, pqxx::work& work) {
    std::string request =
        "SELECT "
        "st.vehicle_journey_id as vehicle_journey_id,"
        "st.arrival_time as arrival_time,"
        "st.departure_time as departure_time,"
        "st.local_traffic_zone as local_traffic_zone,"
        "st.odt as odt,"
        "st.pick_up_allowed as pick_up_allowed,"
        "st.drop_off_allowed as drop_off_allowed,"
        "st.skipped_stop as skipped_stop,"
        "st.is_frequency as is_frequency,"
        "st.date_time_estimated as date_time_estimated,"
        "st.id as id,"
        "st.headsign as headsign,"
        "st.stop_point_id as stop_point_id,"
        "st.\"order\" as st_order,"
        "st.shape_from_prev_id as shape_from_prev_id,"
        "st.boarding_time as boarding_time,"
        "st.alighting_time as alighting_time "
        "FROM navitia.stop_time as st ";

    pqxx::stateless_cursor<pqxx::cursor_base::read_only, pqxx::cursor_base::owned> cursor(work, request, "stcursor",
                                                                                          false);

    size_t chunk_limit = 100000;
    size_t nb_rows = cursor.size();
    for (size_t current_idx = 0; current_idx < nb_rows; current_idx += chunk_limit) {
        pqxx::result result = cursor.retrieve(current_idx, std::min(current_idx + chunk_limit, nb_rows));
        const int vehicle_journey_id_c = result.column_number("vehicle_journey_id");
        const int st_order_c = result.column_number("st_order");
        const int arrival_time_c = result.column_number("arrival_time");
        const int departure_time_c = result.column_number("departure_time");
        const int local_traffic_zone_c = result.column_number("local_traffic_zone");
        const int date_time_estimated_c = result.column_number("date_time_estimated");
        const int odt_c = result.column_number("odt");
        const int pick_up_allowed_c = result.column_number("pick_up_allowed");
        const int drop_off_allowed_c = result.column_number("drop_off_allowed");
        const int skipped_stop_c = result.column_number("skipped_stop");
        const int is_frequency_c = result.column_number("is_frequency");
        const int stop_point_id_c = result.column_number("stop_point_id");
        const int shape_from_prev_id_c = result.column_number("shape_from_prev_id");
        const int id_c = result.column_number("id");
        const int headsign_c = result.column_number("headsign");

        for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
            const auto vj_id = const_it[vehicle_journey_id_c].as<idx_t>();
            auto& sts = sts_from_vj[vj_id];
            size_t order = const_it[st_order_c].as<idx_t>();
            if (order + 1 > sts.size()) {
                sts.resize(order + 1);
            }
            nt::StopTime& stop = sts[order];

            const_it[arrival_time_c].to(stop.arrival_time);
            const_it[departure_time_c].to(stop.departure_time);
            if (!const_it[local_traffic_zone_c].is_null()) {
                const_it[local_traffic_zone_c].to(stop.local_traffic_zone);
            }
            stop.set_date_time_estimated(const_it[date_time_estimated_c].as<bool>());
            stop.set_odt(const_it[odt_c].as<bool>());
            stop.set_pick_up_allowed(const_it[pick_up_allowed_c].as<bool>());
            stop.set_drop_off_allowed(const_it[drop_off_allowed_c].as<bool>());
            stop.set_skipped_stop(const_it[skipped_stop_c].as<bool>());
            stop.set_is_frequency(const_it[is_frequency_c].as<bool>());

            stop.stop_point = stop_point_map[const_it[stop_point_id_c].as<idx_t>()];

            if (!const_it[shape_from_prev_id_c].is_null()) {
                stop.shape_from_prev = this->shapes_map[const_it[shape_from_prev_id_c].as<idx_t>()];
            }

            const_it["boarding_time"].to(stop.boarding_time);
            const_it["alighting_time"].to(stop.alighting_time);

            const auto st_id = const_it[id_c].as<nt::idx_t>();
            const StKey st_key = {vj_id, sts.size() - 1};

            if (!const_it[headsign_c].is_null()) {
                std::string headsign = const_it[headsign_c].as<std::string>();
                if (!headsign.empty()) {
                    stop_time_headsigns[st_id] = headsign;
                    id_to_stop_time_key[st_id] = st_key;
                }
            }

            // we check if we have some comments
            if (stop_time_comments.count(st_id)) {
                id_to_stop_time_key[st_id] = st_key;
            }
        }
    }
}

void EdReader::finish_stop_times(nt::Data& data) {
    auto get_st = [&](const idx_t id) -> const nt::StopTime& {
        const auto& st_key = id_to_stop_time_key.at(id);
        return vehicle_journey_map.at(st_key.first)->stop_time_list.at(st_key.second);
    };
    for (const auto& headsign : stop_time_headsigns) {
        data.pt_data->headsign_handler.affect_headsign_to_stop_time(get_st(headsign.first), headsign.second);
    }
    for (const auto& comments : stop_time_comments) {
        for (const auto& comment : comments.second) {
            data.pt_data->comments.add(get_st(comments.first), comment);
        }
    }
    release(id_to_stop_time_key);
    release(stop_time_headsigns);
    release(stop_time_comments);
}

template <typename Map>
size_t add_comment(nt::Data& data, const idx_t obj_id, const Map& map, const nt::Comment& comment) {
    const auto obj = find_or_default(obj_id, map);

    if (!obj) {
        return 1;
    }

    data.pt_data->comments.add(obj, comment);

    return 0;
}

void EdReader::fill_comments(nt::Data& data, pqxx::work& work) {
    // since the comments can be big we use shared_ptr to share them
    std::map<unsigned int, nt::Comment> comments_by_id;

    const std::string comment_request = "SELECT id, comment, type FROM navitia.comments;";
    pqxx::result comment_result = work.exec(comment_request);
    for (auto const_it = comment_result.begin(); const_it != comment_result.end(); ++const_it) {
        nt::Comment comment;
        const_it["comment"].to(comment.value);
        const_it["type"].to(comment.type);
        comments_by_id[const_it["id"].as<unsigned int>()] = comment;
    }

    const std::string pt_object_comment_request =
        "SELECT object_type, object_id, comment_id FROM navitia.ptobject_comments;";
    pqxx::result result = work.exec(pt_object_comment_request);

    size_t cpt_not_found(0);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        const std::string type_str = const_it["object_type"].as<std::string>();
        const auto obj_id = const_it["object_id"].as<idx_t>();
        const auto comment_id = const_it["comment_id"].as<unsigned int>();

        const auto it = comments_by_id.find(comment_id);
        if (it == comments_by_id.end()) {
            LOG4CPLUS_WARN(log, "impossible to find comment " << comment_id << " skipping comment for " << obj_id << "("
                                                              << type_str << ")");
            continue;
        }

        const auto& comment = it->second;

        if (type_str == "route") {
            cpt_not_found += add_comment(data, obj_id, route_map, comment);
        } else if (type_str == "line") {
            cpt_not_found += add_comment(data, obj_id, line_map, comment);
        } else if (type_str == "line_group") {
            cpt_not_found += add_comment(data, obj_id, line_group_map, comment);
        } else if (type_str == "stop_area") {
            cpt_not_found += add_comment(data, obj_id, stop_area_map, comment);
        } else if (type_str == "stop_point") {
            cpt_not_found += add_comment(data, obj_id, stop_point_map, comment);
        } else if (type_str == "stop_time") {
            // for stop time we store the comment on a temporary map
            // and we will store them when the stop time is read.
            // this way we don't have to store all stoptimes in a map
            stop_time_comments[obj_id].push_back(comment);
        } else if (type_str == "trip") {
            // as we need to create vjs after stop times, we need to store the comments
            vehicle_journey_comments[obj_id].push_back(comment);
        } else {
            LOG4CPLUS_WARN(log, "invalid type, skipping object comment: " << obj_id << "(" << type_str << ")");
        }
    }
    if (cpt_not_found) {
        LOG4CPLUS_WARN(log, cpt_not_found << " pt object not found for comments");
    }
}

void EdReader::fill_poi_types(navitia::type::Data& data, pqxx::work& work) {
    std::string request = "SELECT id, uri, name FROM georef.poi_type;";
    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto* poi_type = new navitia::georef::POIType();
        const_it["uri"].to(poi_type->uri);
        const_it["name"].to(poi_type->name);
        poi_type->idx = data.geo_ref->poitypes.size();
        data.geo_ref->poitypes.push_back(poi_type);
        this->poi_type_map[const_it["id"].as<idx_t>()] = poi_type;
    }
}

void EdReader::fill_pois(navitia::type::Data& data, pqxx::work& work) {
    std::string request =
        "SELECT poi.id, poi.weight, ST_X(poi.coord::geometry) as lon, "
        "ST_Y(poi.coord::geometry) as lat, poi.visible as visible, "
        "poi.name, poi.uri, poi.poi_type_id, poi.address_number, "
        "poi.address_name FROM georef.poi poi, "
        "georef.poi_type poi_type where poi.poi_type_id=poi_type.id;";
    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        std::string string_number;
        int int_number;
        auto* poi = new navitia::georef::POI();
        const_it["uri"].to(poi->uri);
        const_it["name"].to(poi->name);
        const_it["address_number"].to(string_number);
        int_number = str_to_int(string_number);
        if (int_number > -1) {
            poi->address_number = int_number;
        }
        const_it["address_name"].to(poi->address_name);
        poi->coord.set_lon(const_it["lon"].as<double>());
        poi->coord.set_lat(const_it["lat"].as<double>());
        poi->visible = const_it["visible"].as<bool>();
        const_it["weight"].to(poi->weight);
        navitia::georef::POIType* poi_type = this->poi_type_map[const_it["poi_type_id"].as<idx_t>()];
        if (poi_type != nullptr) {
            poi->poitype_idx = poi_type->idx;
        }
        this->poi_map[const_it["id"].as<idx_t>()] = poi;
        poi->idx = data.geo_ref->pois.size();
        data.geo_ref->pois.push_back(poi);
    }
}

void EdReader::fill_poi_properties(navitia::type::Data& /*unused*/, pqxx::work& work) {
    std::string request = "select poi_id, key, value from georef.poi_properties;";
    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        navitia::georef::POI* poi = this->poi_map[const_it["poi_id"].as<idx_t>()];
        if (poi != nullptr) {
            std::string key, value;
            const_it["key"].to(key);
            const_it["value"].to(value);
            poi->properties[key] = value;
        }
    }
}

void EdReader::fill_ways(navitia::type::Data& data, pqxx::work& work) {
    std::string request = "SELECT id, name, uri, type, visible FROM georef.way;";
    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto id = const_it["id"].as<idx_t>();

        if (way_to_ignore.find(id) != way_to_ignore.end()) {
            continue;
        }

        auto* way = new navitia::georef::Way;
        const_it["uri"].to(way->uri);
        const_it["name"].to(way->name);
        way->idx = data.geo_ref->ways.size();

        const_it["type"].to(way->way_type);
        way->visible = const_it["visible"].as<bool>();
        data.geo_ref->ways.push_back(way);
        this->way_map[const_it["id"].as<idx_t>()] = way;
    }
}

void EdReader::fill_house_numbers(navitia::type::Data& data, pqxx::work& work) {
    std::string request =
        "SELECT way_id, ST_X(coord::geometry) as lon, ST_Y(coord::geometry) as lat, number, left_side FROM "
        "georef.house_number where way_id IS NOT NULL;";
    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        std::string string_number;
        const_it["number"].to(string_number);
        int int_number = str_to_int(string_number);
        if (int_number != -1) {
            navitia::georef::HouseNumber hn;
            hn.number = int_number;
            hn.coord.set_lon(const_it["lon"].as<double>());
            hn.coord.set_lat(const_it["lat"].as<double>());
            navitia::georef::Way* way = this->way_map[const_it["way_id"].as<idx_t>()];
            if (way != nullptr) {
                way->add_house_number(hn);
            }
        }
    }
    for (auto way : data.geo_ref->ways) {
        way->sort_house_numbers();
    }
}
struct ComponentVertice {
    uint64_t db_id;
};
struct ComponentEdge {
    navitia::flat_enum_map<nt::Mode_e, bool> modes;
};

using ComponentGraph =
    boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, ComponentVertice, ComponentEdge>;
using component_edge_t = boost::graph_traits<ComponentGraph>::edge_descriptor;
using component_vertex_t = boost::graph_traits<ComponentGraph>::vertex_descriptor;

struct ModeFilter {
    nt::Mode_e mode = nt::Mode_e::Walking;
    const ComponentGraph* g = nullptr;
    ModeFilter(nt::Mode_e mode, const ComponentGraph* g) : mode(mode), g(g) {}
    ModeFilter() = default;

    bool operator()(const component_edge_t& e) const { return (*g)[e].modes[mode]; }
};
using filtered_graph = boost::filtered_graph<ComponentGraph, ModeFilter, boost::keep_all>;

static ComponentGraph make_graph(pqxx::work& work) {
    ComponentGraph graph;
    std::unordered_map<uint64_t, component_vertex_t> node_map;

    // vertex loading
    std::string request = "select id from georef.node;";
    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto id = const_it["id"].as<uint64_t>();
        auto v = ComponentVertice{id};
        node_map[id] = boost::add_vertex(v, graph);
    }
    // construction of a temporary graph
    request = "select source_node_id, target_node_id,";
    request += " pedestrian_allowed as walk, cycles_allowed as bike, cars_allowed as car from georef.edge;";
    result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto e = ComponentEdge();
        e.modes[nt::Mode_e::Walking] = const_it["walk"].as<bool>();
        e.modes[nt::Mode_e::Bike] = const_it["bike"].as<bool>();
        e.modes[nt::Mode_e::Car] = const_it["car"].as<bool>();
        uint64_t source = node_map[const_it["source_node_id"].as<uint64_t>()];
        uint64_t target = node_map[const_it["target_node_id"].as<uint64_t>()];
        boost::add_edge(source, target, e, graph);
    }
    return graph;
}

static std::vector<component_vertex_t> get_useless_nodes(const filtered_graph& graph,
                                                         const nt::Mode_e mode,
                                                         const double min_non_connected_graph_ratio) {
    std::vector<component_vertex_t> useless_nodes;
    auto log = log4cplus::Logger::getInstance("log");

    auto nb_vertices = boost::num_vertices(graph);
    std::vector<uint64_t> vertex_component(nb_vertices);
    boost::strong_components(graph, &vertex_component[0]);
    std::map<size_t, size_t> component_size;
    boost::optional<std::pair<size_t, size_t>> principal_component = {
        {std::numeric_limits<size_t>::max(), 0}};  // pair with id/number of vertex
    // we create the map to know the size of each component
    for (auto component : vertex_component) {
        component_size[component]++;

        size_t count = component_size[component];
        if (!principal_component || principal_component->second < count) {
            principal_component = {{component, count}};
        }
    }

    LOG4CPLUS_INFO(log, component_size.size() << " connexes components found for mode " << mode);

    if (!principal_component) {
        LOG4CPLUS_ERROR(log, "Impossible to find a main composent in graph for mode "
                                 << mode << ". Graph must be empty (nb vertices = " << nb_vertices << ")");
        return useless_nodes;
    }

    LOG4CPLUS_INFO(log, "the biggest has " << principal_component->second << " nodes");

    auto big_enough = [&](const size_t size) {
        return double(size) / principal_component->second >= min_non_connected_graph_ratio;
    };

    for (component_vertex_t vertex_idx = 0; vertex_idx != vertex_component.size(); ++vertex_idx) {
        auto comp = vertex_component[vertex_idx];

        auto nb_elt_in_component = component_size[comp];

        if (big_enough(nb_elt_in_component)) {
            continue;  // big enough, we skip
        }
        useless_nodes.push_back(vertex_idx);
    }
    LOG4CPLUS_INFO(log, "for mode " << mode << ", " << useless_nodes.size() << " useless nodes");
    for (const auto& comp_and_size : component_size) {
        if (big_enough(comp_and_size.second)) {
            LOG4CPLUS_INFO(log, "we kept a component with " << comp_and_size.second << " nodes");
        }
    }
    return useless_nodes;
}

void EdReader::fill_vector_to_ignore(pqxx::work& work, const double min_non_connected_graph_ratio) {
    const auto graph = make_graph(work);
    std::unordered_map<uint64_t, uint8_t> count_useless_mode_by_node;  // used to disable useless nodes

    const auto modes = {nt::Mode_e::Walking, nt::Mode_e::Bike, nt::Mode_e::Car};
    for (auto mode : modes) {
        auto mode_graph = filtered_graph(graph, ModeFilter(mode, &graph), {});
        auto useless_nodes = get_useless_nodes(mode_graph, mode, min_non_connected_graph_ratio);

        for (const auto n_idx : useless_nodes) {
            auto target_db_id = mode_graph[n_idx].db_id;
            count_useless_mode_by_node[target_db_id]++;
            // we remove the transportation mode for all the edges of this node
            BOOST_FOREACH (auto e_idx, boost::in_edges(n_idx, mode_graph)) {
                auto source_db_id = mode_graph[boost::source(e_idx, mode_graph)].db_id;
                this->edge_to_ignore_by_modes[mode].insert({source_db_id, target_db_id});
            }
        }
    }

    for (const auto& p : count_useless_mode_by_node) {
        if (p.second == modes.size()) {
            // if the node is useless in all modes, we can remove the node
            this->node_to_ignore.insert(p.first);
        }
    }

    // Note: for the moment we do not clean up the ways that do not have any edges anymore

    LOG4CPLUS_INFO(log, node_to_ignore.size() << " node to ignore");
    LOG4CPLUS_INFO(log, this->edge_to_ignore_by_modes[nt::Mode_e::Walking].size()
                            << " walking edges disabled, " << this->edge_to_ignore_by_modes[nt::Mode_e::Bike].size()
                            << " bike edges disabled, " << this->edge_to_ignore_by_modes[nt::Mode_e::Car].size()
                            << " car edges disabled ");
}

void EdReader::fill_vertex(navitia::type::Data& data, pqxx::work& work) {
    std::string request = "select id, ST_X(coord::geometry) as lon, ST_Y(coord::geometry) as lat from georef.node;";
    pqxx::result result = work.exec(request);
    uint64_t idx = 0;
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto id = const_it["id"].as<uint64_t>();

        if (node_to_ignore.find(id) != node_to_ignore.end()) {
            this->node_map[id] = std::numeric_limits<uint64_t>::max();
            continue;
        }

        navitia::georef::Vertex v;
        v.coord.set_lon(const_it["lon"].as<double>());
        v.coord.set_lat(const_it["lat"].as<double>());
        boost::add_vertex(v, data.geo_ref->graph);
        this->node_map[id] = idx;
        idx++;
    }
    data.geo_ref->init();
}

boost::optional<navitia::time_res_traits::sec_type> EdReader::get_duration(nt::Mode_e mode,
                                                                           double len,
                                                                           double speed,
                                                                           uint64_t source,
                                                                           uint64_t target) {
    try {
        // overflow check since we want to store that on a int32
        return boost::lexical_cast<navitia::time_res_traits::sec_type>(std::floor(len / speed));
    } catch (const boost::bad_lexical_cast&) {
        LOG4CPLUS_WARN(log, "edge length overflow for " << mode << " for source " << source << " target " << target
                                                        << " length: " << len << ", we ignore this edge");
        return boost::none;
    }
}

boost::optional<navitia::time_res_traits::sec_type> EdReader::get_duration(nt::Mode_e mode,
                                                                           double len,
                                                                           uint64_t source,
                                                                           uint64_t target) {
    try {
        // overflow check since we want to store that on a int32
        return boost::lexical_cast<navitia::time_res_traits::sec_type>(
            std::floor(len / double(ng::default_speed[mode])));
    } catch (const boost::bad_lexical_cast&) {
        LOG4CPLUS_WARN(log, "edge length overflow for " << mode << " for source " << source << " target " << target
                                                        << " length: " << len << ", we ignore this edge");
        return boost::none;
    }
}

void EdReader::fill_graph(navitia::type::Data& data, pqxx::work& work, bool export_georef_edges_geometries) {
    std::string request =
        "select e.source_node_id, target_node_id, e.way_id, "
        "ST_LENGTH(the_geog) AS leng, e.pedestrian_allowed as pede, "
        "e.cycles_allowed as bike,e.cars_allowed as car, car_speed";
    // Don't call ST_ASTEXT if not needed since it's slow
    if (export_georef_edges_geometries) {
        request += ", ST_ASTEXT(the_geog) AS geometry";
    }
    request += " from georef.edge e;";
    pqxx::result result = work.exec(request);
    size_t nb_edges_no_way = 0, nb_useless_edges = 0;
    size_t nb_walking_edges(0), nb_biking_edges(0), nb_driving_edges(0);

    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        navitia::georef::Way* way = this->way_map[const_it["way_id"].as<uint64_t>()];
        const auto source_node_id = const_it["source_node_id"].as<uint64_t>();
        const auto target_node_id = const_it["target_node_id"].as<uint64_t>();
        auto it_source = node_map.find(source_node_id);
        auto it_target = node_map.find(target_node_id);

        if (it_source == node_map.end() || it_target == node_map.end()) {
            continue;
        }

        uint64_t source = it_source->second;
        uint64_t target = it_target->second;

        if (source == std::numeric_limits<uint64_t>::max() || target == std::numeric_limits<uint64_t>::max()) {
            continue;
        }

        if (!way) {
            nb_edges_no_way++;
            continue;
        }

        navitia::georef::Edge e;
        auto len = const_it["leng"].as<double>();
        e.way_idx = way->idx;
        if (export_georef_edges_geometries) {
            nt::LineString geometry;
            boost::geometry::read_wkt(const_it["geometry"].as<std::string>(), geometry);
            if (!geometry.empty()) {
                e.geom_idx = way->geoms.size();
                way->geoms.push_back(geometry);
            }
        }
        auto edge_id = std::make_pair(source_node_id, target_node_id);
        bool walkable =
            const_it["pede"].as<bool>() && this->edge_to_ignore_by_modes[nt::Mode_e::Walking].count(edge_id) == 0;
        bool ridable =
            const_it["bike"].as<bool>() && this->edge_to_ignore_by_modes[nt::Mode_e::Bike].count(edge_id) == 0;
        bool carable = const_it["car"].as<bool>() && this->edge_to_ignore_by_modes[nt::Mode_e::Car].count(edge_id) == 0;

        if (!walkable && !ridable && !carable) {
            nb_useless_edges++;
            continue;
        }

        if (walkable) {
            if (auto dur = get_duration(nt::Mode_e::Walking, len, source, target)) {
                e.duration = navitia::seconds(*dur);
                boost::add_edge(source, target, e, data.geo_ref->graph);
                way->edges.emplace_back(source, target);
                nb_walking_edges++;
            }
        }
        if (ridable) {
            if (auto dur = get_duration(nt::Mode_e::Bike, len, source, target)) {
                e.duration = navitia::seconds(*dur);
                auto bike_source = data.geo_ref->offsets[nt::Mode_e::Bike] + source;
                auto bike_target = data.geo_ref->offsets[nt::Mode_e::Bike] + target;
                boost::add_edge(bike_source, bike_target, e, data.geo_ref->graph);
                way->edges.emplace_back(bike_source, bike_target);
                nb_biking_edges++;
            }
        }
        if (carable) {
            boost::optional<navitia::time_res_traits::sec_type> dur;
            if (!const_it["car_speed"].is_null()) {
                dur = get_duration(nt::Mode_e::Car, len, const_it["car_speed"].as<double>(), source, target);
            } else {
                dur = get_duration(nt::Mode_e::Car, len, source, target);
            }
            if (dur) {
                e.duration = navitia::seconds(*dur);
                auto car_source = data.geo_ref->offsets[nt::Mode_e::Car] + source;
                auto car_target = data.geo_ref->offsets[nt::Mode_e::Car] + target;
                boost::add_edge(car_source, car_target, e, data.geo_ref->graph);
                way->edges.emplace_back(car_source, car_target);
                nb_driving_edges++;
            }
        }
    }

    if (nb_edges_no_way) {
        LOG4CPLUS_WARN(log, nb_edges_no_way << " edges have an unknown way");
    }
    if (nb_useless_edges) {
        LOG4CPLUS_WARN(log, nb_useless_edges << " edges are not usable by any modes");
    }

    LOG4CPLUS_INFO(log, boost::num_edges(data.geo_ref->graph) << " edges added ");
    LOG4CPLUS_INFO(log, nb_walking_edges << " walking edges");
    LOG4CPLUS_INFO(log, nb_biking_edges << " biking edges");
    LOG4CPLUS_INFO(log, nb_driving_edges << " driving edges");
}

void EdReader::fill_graph_bss(navitia::type::Data& data, pqxx::work& work) {
    std::string request = "SELECT poi.id as id, ST_X(poi.coord::geometry) as lon,";
    request += "ST_Y(poi.coord::geometry) as lat";
    request += " FROM georef.poi poi, georef.poi_type poi_type";
    request += " where poi.poi_type_id=poi_type.id";
    request += " and poi_type.uri = 'poi_type:amenity:bicycle_rental'";

    pqxx::result result = work.exec(request);
    size_t cpt_bike_sharing(0);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        navitia::type::GeographicalCoord coord;
        coord.set_lon(const_it["lon"].as<double>());
        coord.set_lat(const_it["lat"].as<double>());
        if (data.geo_ref->add_bss_edges(coord)) {
            cpt_bike_sharing++;
        } else {
            LOG4CPLUS_WARN(log, "Impossible to find the nearest edge for the bike sharing station poi_id = "
                                    << const_it["id"].as<std::string>());
        }
    }
    LOG4CPLUS_INFO(log, cpt_bike_sharing << " bike sharing stations added");
}

void EdReader::fill_graph_parking(navitia::type::Data& data, pqxx::work& work) {
    std::string request =
        "SELECT poi.id as id, ST_X(poi.coord::geometry) as lon,"
        "ST_Y(poi.coord::geometry) as lat"
        " FROM georef.poi poi, georef.poi_type poi_type"
        " where poi.poi_type_id = poi_type.id"
        " and poi_type.uri = 'poi_type:amenity:parking'";

    pqxx::result result = work.exec(request);
    size_t cpt_parking = 0;
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        navitia::type::GeographicalCoord coord;
        coord.set_lon(const_it["lon"].as<double>());
        coord.set_lat(const_it["lat"].as<double>());

        if (data.geo_ref->add_parking_edges(coord)) {
            ++cpt_parking;
        } else {
            LOG4CPLUS_WARN(log, "Impossible to find the nearest edge for the parking poi_id = "
                                    << const_it["id"].as<std::string>());
        }
    }
    LOG4CPLUS_INFO(log, cpt_parking << " parkings added");
}

void EdReader::fill_synonyms(navitia::type::Data& data, pqxx::work& work) {
    std::string key, value;
    std::string request = "SELECT key, value FROM georef.synonym;";
    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        const_it["key"].to(key);
        const_it["value"].to(value);
        boost::to_lower(key);
        boost::to_lower(value);
        if (!value.empty()) {
            data.geo_ref->synonyms[key] = value;
        } else {
            data.geo_ref->ghostwords.insert(key);
        }
    }
}

// Fares:
void EdReader::fill_prices(navitia::type::Data& data, pqxx::work& work) {
    std::string request =
        "select ticket_key, ticket_title, ticket_comment, "
        "ticket_id, valid_from, valid_to, ticket_price, comments, "
        "currency from navitia.ticket, navitia.dated_ticket "
        "where ticket_id = ticket_key";

    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        nf::Ticket ticket;
        const_it["ticket_key"].to(ticket.key);
        const_it["ticket_title"].to(ticket.caption);
        const_it["ticket_comment"].to(ticket.comment);
        const_it["currency"].to(ticket.currency);
        const_it["ticket_price"].to(ticket.value.value);
        bg::date start = bg::from_string(const_it["valid_from"].as<std::string>());
        bg::date end = bg::from_string(const_it["valid_to"].as<std::string>());

        nf::DateTicket& date_ticket = data.fare->fare_map[ticket.key];

        date_ticket.add(start, end, ticket);
    }
}

void EdReader::fill_transitions(navitia::type::Data& data, pqxx::work& work) {
    // we build the transition graph
    std::map<nf::State, nf::Fare::vertex_t> state_map;
    nf::State begin;  // Start is an empty node (and the node is already is the fare graph, since it has been added in
                      // the constructor with the default ticket)
    state_map[begin] = data.fare->begin_v;

    std::string request =
        "select id, before_change, after_change, start_trip, "
        "end_trip, global_condition, ticket_id from navitia.transition ";
    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        nf::Transition transition;

        std::string before_str = const_it["before_change"].as<std::string>();
        std::string after_str = const_it["after_change"].as<std::string>();

        nf::State start = ed::connectors::parse_state(before_str);
        nf::State end = ed::connectors::parse_state(after_str);

        std::string start_trip_str = const_it["start_trip"].as<std::string>();
        std::string end_trip_str = const_it["end_trip"].as<std::string>();

        transition.start_conditions = ed::connectors::parse_conditions(start_trip_str);
        transition.end_conditions = ed::connectors::parse_conditions(end_trip_str);

        std::string cond_str = const_it["global_condition"].as<std::string>();
        transition.global_condition = ed::connectors::to_global_condition(cond_str);

        const_it["ticket_id"].to(transition.ticket_key);

        nf::Fare::vertex_t start_v, end_v;
        if (state_map.find(start) == state_map.end()) {
            start_v = boost::add_vertex(start, data.fare->g);
            state_map[start] = start_v;
        } else {
            start_v = state_map[start];
        }

        if (state_map.find(end) == state_map.end()) {
            end_v = boost::add_vertex(end, data.fare->g);
            state_map[end] = end_v;
        } else {
            end_v = state_map[end];
        }

        // add the edge to the fare graph
        boost::add_edge(start_v, end_v, transition, data.fare->g);
    }
}

void EdReader::fill_origin_destinations(navitia::type::Data& data, pqxx::work& work) {
    std::string request =
        "select od.id, origin_id, origin_mode, destination_id, destination_mode, "
        "ticket.id, ticket.od_id, ticket_id "
        "from navitia.origin_destination as od, navitia.od_ticket as ticket where od.id = ticket.od_id;";
    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        nf::OD_key::od_type origin_mode = ed::connectors::to_od_type(const_it["origin_mode"].as<std::string>());
        nf::OD_key origin(origin_mode, const_it["origin_id"].as<std::string>());

        nf::OD_key::od_type destination_mode =
            ed::connectors::to_od_type(const_it["destination_mode"].as<std::string>());
        nf::OD_key destination(destination_mode, const_it["destination_id"].as<std::string>());

        std::string ticket = const_it["ticket_id"].as<std::string>();
        data.fare->od_tickets[origin][destination].push_back(ticket);
    }
}

void EdReader::fill_calendars(navitia::type::Data& data, pqxx::work& work) {
    std::string request =
        "select cal.id, cal.name, cal.uri, "
        "wp.monday, wp.tuesday, wp.wednesday, "
        "wp.thursday,wp.friday, wp.saturday, wp.sunday "
        "from navitia.calendar  cal, navitia.week_pattern wp "
        "where cal.week_pattern_id = wp.id;";
    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto* cal = new navitia::type::Calendar(data.meta->production_date.begin());
        const_it["name"].to(cal->name);
        const_it["uri"].to(cal->uri);
        cal->week_pattern[navitia::Monday] = const_it["monday"].as<bool>();
        cal->week_pattern[navitia::Tuesday] = const_it["tuesday"].as<bool>();
        cal->week_pattern[navitia::Wednesday] = const_it["wednesday"].as<bool>();
        cal->week_pattern[navitia::Thursday] = const_it["thursday"].as<bool>();
        cal->week_pattern[navitia::Friday] = const_it["friday"].as<bool>();
        cal->week_pattern[navitia::Saturday] = const_it["saturday"].as<bool>();
        cal->week_pattern[navitia::Sunday] = const_it["sunday"].as<bool>();

        data.pt_data->calendars.push_back(cal);
        calendar_map[const_it["id"].as<idx_t>()] = cal;
    }
}

void EdReader::fill_periods(navitia::type::Data& /*unused*/, pqxx::work& work) {
    std::string request =
        "select per.calendar_id, per.begin_date, per.end_date "
        "from navitia.period per";
    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto cal_id = const_it["calendar_id"].as<idx_t>();
        navitia::type::Calendar* cal = this->calendar_map[cal_id];
        if (cal == nullptr) {
            LOG4CPLUS_WARN(log, "unable to find calendar " << cal_id);
            continue;
        }
        boost::gregorian::date start(bg::from_string(const_it["begin_date"].as<std::string>()));
        boost::gregorian::date end(bg::from_string(const_it["end_date"].as<std::string>()));
        cal->active_periods.emplace_back(start, end);
    }
}

void EdReader::fill_exception_dates(navitia::type::Data& /*unused*/, pqxx::work& work) {
    std::string request = "select id, datetime, type_ex, calendar_id ";
    request += "from navitia.exception_date;";
    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        navitia::type::Calendar* cal = this->calendar_map[const_it["calendar_id"].as<idx_t>()];
        if (cal != nullptr) {
            navitia::type::ExceptionDate exception_date;
            exception_date.date = bg::from_string(const_it["datetime"].as<std::string>());
            exception_date.type = navitia::type::to_exception_type(const_it["type_ex"].as<std::string>());
            cal->exceptions.push_back(exception_date);
        }
    }
}

void EdReader::fill_rel_calendars_lines(navitia::type::Data& /*unused*/, pqxx::work& work) {
    std::string request =
        "select rcl.calendar_id, rcl.line_id "
        "from navitia.rel_calendar_line  rcl;";
    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        auto cal_id = const_it["calendar_id"].as<idx_t>();
        navitia::type::Calendar* cal = this->calendar_map[cal_id];
        auto line_id = const_it["line_id"].as<idx_t>();
        navitia::type::Line* line = this->line_map[line_id];
        if ((cal != nullptr) && (line != nullptr)) {
            line->calendar_list.push_back(cal);
        } else {
            LOG4CPLUS_WARN(log, "impossible to find "
                                    << (cal == nullptr ? "calendar " + std::to_string(cal_id) + ", " : "")
                                    << (line == nullptr ? "line " + std::to_string(line_id) : ""));
        }
    }
}

void EdReader::build_rel_way_admin(navitia::type::Data& /*unused*/, pqxx::work& work) {
    std::string request = "select admin_id, way_id from georef.rel_way_admin;";
    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        navitia::georef::Way* way = this->way_map[const_it["way_id"].as<idx_t>()];
        if (way != nullptr) {
            navitia::georef::Admin* admin = this->admin_map[const_it["admin_id"].as<idx_t>()];
            if (admin != nullptr) {
                way->admin_list.push_back(admin);
            }
        }
    }
}

void EdReader::build_rel_admin_admin(navitia::type::Data& /*unused*/, pqxx::work& work) {
    std::string request = "select master_admin_id, admin_id from georef.rel_admin_admin;";
    pqxx::result result = work.exec(request);
    for (auto const_it = result.begin(); const_it != result.end(); ++const_it) {
        navitia::georef::Admin* admin_master = this->admin_map[const_it["master_admin_id"].as<idx_t>()];
        if (admin_master != nullptr) {
            navitia::georef::Admin* admin = this->admin_map[const_it["admin_id"].as<idx_t>()];
            if ((admin != nullptr)) {
                admin_master->admin_list.push_back(admin);
            }
        }
    }
}

void EdReader::check_coherence(navitia::type::Data& data) const {
    // check not associated lines
    size_t non_associated_lines(0);
    for (navitia::type::Line* line : data.pt_data->lines) {
        if (line->calendar_list.empty()) {
            LOG4CPLUS_TRACE(log, "the line " << line->uri << " is not associated with any calendar");
            non_associated_lines++;
        }
    }
    if (non_associated_lines) {
        LOG4CPLUS_WARN(log, non_associated_lines << " lines are not associated with any calendar (and "
                                                 << (data.pt_data->lines.size() - non_associated_lines)
                                                 << " are associated with at least one");
    }

    // Check if every vehicle journey with a next_vj has a backlink
    size_t nb_error_on_vj = 0;
    for (auto vj : data.pt_data->vehicle_journeys) {
        if (vj->next_vj && vj->next_vj->prev_vj != vj) {
            LOG4CPLUS_ERROR(log, "Vehicle journey " << vj->uri << " has a next_vj, but the back link is invalid");
            ++nb_error_on_vj;
        }
        if (vj->prev_vj && vj->prev_vj->next_vj != vj) {
            LOG4CPLUS_ERROR(log, "Vehicle journey " << vj->uri << " has a prev_vj, but the back link is invalid");
            ++nb_error_on_vj;
        }
    }
    if (nb_error_on_vj > 0) {
        LOG4CPLUS_INFO(log, "There was " << nb_error_on_vj
                                         << " vehicle journeys with consistency error over a total of "
                                         << data.pt_data->vehicle_journeys.size() << " vehicle journeys");
    }
}

}  // namespace ed
