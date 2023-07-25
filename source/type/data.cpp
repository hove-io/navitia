/* Copyright © 2001-2022, Hove and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Hove (www.hove.com).
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

#include "data.h"

#include "fare/fare.h"
#include "georef/georef.h"
#include "kraken/fill_disruption_from_database.h"
#include "lz4_filter/filter.h"
#include "pt_data.h"
#include "routing/dataraptor.h"
#include "type/meta_data.h"
#include "type/serialization.h"
#include "type/base_pt_objects.h"
#include "type/dataset.h"
#include "type/company.h"
#include "type/network.h"
#include "type/calendar.h"
#include "type/contributor.h"
#include "type/meta_vehicle_journey.h"
#include "type/physical_mode.h"
#include "type/commercial_mode.h"
#include "utils/functions.h"
#include "utils/serialization_atomic.h"
#include "utils/serialization_unique_ptr.h"
#include "utils/threadbuf.h"

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/container/container_fwd.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/serialization/variant.hpp>
#include <eos_portable_archive/portable_iarchive.hpp>
#include <eos_portable_archive/portable_oarchive.hpp>

#include <fstream>
#include <thread>
#include <regex>

namespace pt = boost::posix_time;

namespace navitia {
namespace type {

const unsigned int Data::data_version = 15;  //< *INCREMENT* every time serialized data are modified

Data::Data(size_t data_identifier)
    : _last_rt_data_loaded(boost::posix_time::not_a_date_time),
      disruption_error(false),
      data_identifier(data_identifier),
      meta(std::make_unique<MetaData>()),
      pt_data(std::make_unique<PT_Data>()),
      geo_ref(std::make_unique<navitia::georef::GeoRef>()),
      dataRaptor(std::make_unique<navitia::routing::dataRAPTOR>()),
      fare(std::make_unique<navitia::fare::Fare>()),
      find_admins([&](const GeographicalCoord& c, georef::AdminRtree& admin_tree) {
          return geo_ref->find_admins(c, admin_tree);
      }),
      last_load_succeeded(false) {
    loaded = false;
    is_connected_to_rabbitmq = false;
    is_realtime_loaded = false;
}

Data::~Data() = default;

template <class Archive>
void Data::save(Archive& ar, const unsigned int /*unused*/) const {
    ar& pt_data& geo_ref& meta& fare& last_load_at& loaded& last_load_succeeded& is_connected_to_rabbitmq&
        is_realtime_loaded;
}
template <class Archive>
void Data::load(Archive& ar, const unsigned int version) {
    this->version = version;
    if (this->version != data_version) {
        unsigned int v = data_version;  // won't link otherwise...
        auto msg =
            boost::format("Warning data version don't match with the data version of kraken %u (current version: %d)")
            % version % v;
        throw navitia::data::wrong_version(msg.str());
    }
    ar& pt_data& geo_ref& meta& fare& last_load_at& loaded& last_load_succeeded& is_connected_to_rabbitmq&
        is_realtime_loaded;
}
SPLIT_SERIALIZABLE(Data)

/**
 * @brief Load data (in nav.lz4).
 * 1. Uncompress lz4 file
 * 2. Read .nav
 * 3. Load in type::Data structure
 *
 * @param filename Lz4 data File name (file.nav.lz4)
 */
void Data::load_nav(const std::string& filename) {
    // Add logger
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    LOG4CPLUS_DEBUG(logger, "Start to load nav");

    if (filename.empty()) {
        LOG4CPLUS_ERROR(logger, "Data loading failed: Data path is empty");
        throw navitia::data::data_loading_error("Data loading failed: Data path is empty");
    }

    try {
        std::ifstream ifs(filename.c_str(), std::ios::in | std::ios::binary);
        ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        this->load(ifs);
        last_load_at = pt::microsec_clock::universal_time();
        last_load_succeeded = true;
        LOG4CPLUS_INFO(logger, boost::format("stopTimes : %d nb foot path : %d Nombre de stop points : %d")
                                   % pt_data->nb_stop_times() % pt_data->stop_point_connections.size()
                                   % pt_data->stop_points.size());
    } catch (const std::exception& ex) {
        LOG4CPLUS_ERROR(logger, "Data loading failed: " + std::string(ex.what()));
        throw navitia::data::data_loading_error("Data loading failed: " + std::string(ex.what()));
    } catch (...) {
        LOG4CPLUS_ERROR(logger, "Data loading failed");
        throw navitia::data::data_loading_error("Data loading failed");
    }
    LOG4CPLUS_DEBUG(logger, "Finished to load nav");
}

void Data::load(std::istream& ifs) {
    boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
    in.push(LZ4Decompressor(2048 * 500), 8192 * 500, 8192 * 500);
    in.push(ifs);
    eos::portable_iarchive ia(in);
    ia >> *this;
}

/**
 * @brief Load disruptions from database.
 * Disruptions are stored in Bdd.
 *
 * @param database Database connection string
 * @param contributors Disruptions contributors name list
 */
void Data::load_disruptions(const std::string& database,
                            int chaos_batch_size,
                            const std::vector<std::string>& contributors) {
    // Add logger
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    LOG4CPLUS_DEBUG(logger, "Start to load disruptions");

    try {
        DisruptionDatabaseReader reader(*pt_data, *meta);
        fill_disruption_from_database(database, meta->production_date, reader, contributors, chaos_batch_size);
        disruption_error = false;
    } catch (const pqxx::broken_connection& ex) {
        LOG4CPLUS_WARN(logger, "Unable to connect to disruptions database: " << std::string(ex.what()));
        disruption_error = true;
        throw navitia::data::disruptions_broken_connection("Unable to connect to disruptions database: "
                                                           + std::string(ex.what()));
    } catch (const pqxx::pqxx_exception& ex) {
        LOG4CPLUS_ERROR(logger, "Disruptions loading error: " << std::string(ex.base().what()));
        disruption_error = true;
        throw navitia::data::disruptions_loading_error("Disruptions loading error: " + std::string(ex.base().what()));
    } catch (const std::exception& ex) {
        LOG4CPLUS_ERROR(logger, "Disruptions loading error: " << std::string(ex.what()));
        disruption_error = true;
        throw navitia::data::disruptions_loading_error("Disruptions loading error: " + std::string(ex.what()));
    } catch (...) {
        LOG4CPLUS_ERROR(logger, "Disruptions loading error");
        disruption_error = true;
        throw navitia::data::disruptions_loading_error("Disruptions loading error");
    }
    LOG4CPLUS_DEBUG(logger, "Finished to load disruptions");
}

/**
 * @brief Build Data Raptor
 *
 * @param cache_size Selected LRU size to optimize cache miss
 */
void Data::build_raptor(size_t cache_size) {
    // Add logger
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    LOG4CPLUS_DEBUG(logger, "Start to build data Raptor");
    dataRaptor->load(*this->pt_data, cache_size);
    LOG4CPLUS_DEBUG(logger, "Finished to build data Raptor");
}

void Data::warmup(const Data& other) {
    this->dataRaptor->warmup(*other.dataRaptor);
}

void Data::save(const std::string& filename) const {
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    boost::filesystem::path p(filename);
    boost::filesystem::path dir = p.parent_path();
    try {
        boost::filesystem::is_directory(p);
    } catch (const boost::filesystem::filesystem_error& e) {
        if (e.code() == boost::system::errc::permission_denied)
            LOG4CPLUS_ERROR(logger, "Search permission is denied for " << p);
        else
            LOG4CPLUS_ERROR(logger, "is_directory(" << p << ") failed with " << e.code().message());
        throw navitia::exception("Unable to write file");
    }
    std::ofstream ofs(filename.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    ofs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        this->save(ofs);
    } catch (const boost::filesystem::filesystem_error& e) {
        if (e.code() == boost::system::errc::permission_denied)
            LOG4CPLUS_ERROR(logger, "Writing permission is denied for " << p);
        else if (e.code() == boost::system::errc::file_too_large)
            LOG4CPLUS_ERROR(logger, "The file " << filename << " is too large");
        else if (e.code() == boost::system::errc::interrupted)
            LOG4CPLUS_ERROR(logger, "Writing was interrupted for " << p);
        else if (e.code() == boost::system::errc::no_buffer_space)
            LOG4CPLUS_ERROR(logger, "No buffer space while writing " << p);
        else if (e.code() == boost::system::errc::not_enough_memory)
            LOG4CPLUS_ERROR(logger, "Not enough memory while writing " << p);
        else if (e.code() == boost::system::errc::no_space_on_device)
            LOG4CPLUS_ERROR(logger, "No space on device while writing " << p);
        else if (e.code() == boost::system::errc::operation_not_permitted)
            LOG4CPLUS_ERROR(logger, "Operation not permitted while writing " << p);
        LOG4CPLUS_ERROR(logger, e.what());
        throw navitia::exception("Unable to write file");
    } catch (const std::ofstream::failure& e) {
        throw navitia::exception(std::string("Unable to write file: ") + e.what());
    }
}

void Data::save(std::ostream& ofs) const {
    boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
    out.push(LZ4Compressor(2048 * 500), 1024 * 500, 1024 * 500);
    out.push(ofs);
    eos::portable_oarchive oa(out);
    oa << *this;
}

void Data::build_uri() {
#define CLEAR_EXT_CODE(type_name, collection_name) this->pt_data->collection_name##_map.clear();
    ITERATE_NAVITIA_PT_TYPES(CLEAR_EXT_CODE)
    this->pt_data->build_uri();
    geo_ref->build_pois_map();
    geo_ref->build_poitypes_map();
    geo_ref->build_admin_map();
}

void Data::build_proximity_list() {
    this->pt_data->build_proximity_list();
    this->geo_ref->build_proximity_list();
    this->geo_ref->project_stop_points_and_access_points(this->pt_data->stop_points);
}

void Data::build_administrative_regions() {
    auto log = log4cplus::Logger::getInstance("ed::Data");
    georef::AdminRtree admin_tree = georef::build_admins_tree(geo_ref->admins);

    // set admins to stop points
    int cpt_no_projected = 0;
    for (type::StopPoint* stop_point : pt_data->stop_points) {
        if (!stop_point->admin_list.empty()) {
            continue;
        }
        const auto& admins = find_admins(stop_point->coord, admin_tree);
        boost::push_back(stop_point->admin_list, admins);
        if (admins.empty()) {
            ++cpt_no_projected;
        }
    }
    if (cpt_no_projected)
        LOG4CPLUS_WARN(log, cpt_no_projected << "/" << pt_data->stop_points.size()
                                             << " stop_points are not associated with any admins");

    // set admins to poi
    cpt_no_projected = 0;
    int cpt_no_initialized = 0;
    for (georef::POI* poi : geo_ref->pois) {
        if (!poi->coord.is_initialized()) {
            cpt_no_initialized++;
            continue;
        }
        if (!poi->admin_list.empty()) {
            continue;
        }
        const auto& admins = find_admins(poi->coord, admin_tree);
        boost::push_back(poi->admin_list, admins);
        if (admins.empty()) {
            ++cpt_no_projected;
        }
    }
    if (cpt_no_projected)
        LOG4CPLUS_WARN(log,
                       cpt_no_projected << "/" << geo_ref->pois.size() << " pois are not associated with any admins");
    if (cpt_no_initialized)
        LOG4CPLUS_WARN(log,
                       cpt_no_initialized << "/" << geo_ref->pois.size() << " pois with coordinates not initialized");

    this->pt_data->build_admins_stop_areas();

    for (const auto* sa : pt_data->stop_areas) {
        for (auto admin : sa->admin_list) {
            if (!admin->from_original_dataset) {
                admin->main_stop_areas.push_back(sa);
            }
        }
    }
}

void Data::build_autocomplete() {
    geo_ref->build_autocomplete_list();
    build_autocomplete_partial();
}

void Data::build_autocomplete_partial() {
    pt_data->build_autocomplete(*geo_ref);
    pt_data->compute_score_autocomplete(*geo_ref);
}

ValidityPattern* Data::get_similar_validity_pattern(ValidityPattern* vp) const {
    auto find_vp_predicate = [&](ValidityPattern* vp1) { return ((*vp) == (*vp1)); };
    auto it = std::find_if(this->pt_data->validity_patterns.begin(), this->pt_data->validity_patterns.end(),
                           find_vp_predicate);
    if (it != this->pt_data->validity_patterns.end()) {
        return *(it);
    }
    return nullptr;
}

void Data::complete() {
    auto logger = log4cplus::Logger::getInstance("log");
    pt::ptime start;
    int admin, sort, autocomplete;

    build_grid_validity_pattern();

    start = pt::microsec_clock::local_time();
    LOG4CPLUS_INFO(logger, "Building administrative regions");
    build_administrative_regions();
    admin = (pt::microsec_clock::local_time() - start).total_milliseconds();

    aggregate_odt();

    build_relations();

    compute_labels();

    start = pt::microsec_clock::local_time();
    pt_data->sort_and_index();
    sort = (pt::microsec_clock::local_time() - start).total_milliseconds();

    start = pt::microsec_clock::local_time();
    LOG4CPLUS_INFO(logger, "Building proximity list");
    build_proximity_list();
    LOG4CPLUS_INFO(logger, "Building uri maps");
    build_uri();
    LOG4CPLUS_INFO(logger, "Building autocomplete");
    build_autocomplete();
    autocomplete = (pt::microsec_clock::local_time() - start).total_milliseconds();

    LOG4CPLUS_INFO(logger, "\t Building admins: " << admin << "ms");
    LOG4CPLUS_INFO(logger, "\t Sorting data: " << sort << "ms");
    LOG4CPLUS_INFO(logger, "\t Building autocomplete " << autocomplete << "ms");
}

/*
    > Fill dataset_list for route and stoppoint
    > Fill vehiclejourney_list for dataset
    > These lists are used by ptref
*/
static void build_datasets(navitia::type::VehicleJourney* vj) {
    if (!vj->dataset) {
        return;
    }
    vj->route->dataset_list.insert(vj->dataset);
    vj->dataset->vehiclejourney_list.insert(vj);
    for (navitia::type::StopTime& st : vj->stop_time_list) {
        if (st.stop_point) {
            st.stop_point->dataset_list.insert(vj->dataset);
        }
    }
    if (vj->route && vj->route->line && vj->route->line->network) {
        vj->route->line->network->dataset_list.insert(vj->dataset);
    }
}

/*
 * Build relations between routes and stop points.
 *
 * @param vj The vehicle journey to browse
 */
static void build_route_and_stop_point_relations(navitia::type::VehicleJourney* vj) {
    for (navitia::type::StopTime& st : vj->stop_time_list) {
        if (st.stop_point) {
            vj->route->stop_point_list.insert(st.stop_point);
            vj->route->stop_area_list.insert(st.stop_point->stop_area);
            st.stop_point->route_list.insert(vj->route);
            if (st.stop_point->stop_area) {
                st.stop_point->stop_area->route_list.insert(vj->route);
            }
        }
    }
}

static void build_companies(navitia::type::VehicleJourney* vj) {
    if (vj->company) {
        if (boost::range::find(vj->route->line->company_list, vj->company) == vj->route->line->company_list.end()) {
            vj->route->line->company_list.push_back(vj->company);
        }
        if (boost::range::find(vj->company->line_list, vj->route->line) == vj->company->line_list.end()) {
            vj->company->line_list.push_back(vj->route->line);
        }
    }
}

void Data::build_relations() {
    // physical_mode_list of line
    for (auto* vj : pt_data->vehicle_journeys) {
        build_datasets(vj);
        build_route_and_stop_point_relations(vj);
        if (!vj->physical_mode || !vj->route || !vj->route->line) {
            continue;
        }
        build_companies(vj);
        if (!navitia::contains(vj->route->line->physical_mode_list, vj->physical_mode)) {
            vj->route->line->physical_mode_list.push_back(vj->physical_mode);
        }
    }
}

void Data::aggregate_odt() {
    // TODO ODT NTFSv0.3: remove that when we stop to support NTFSv0.1
    //
    // cf http://confluence.canaltp.fr/pages/viewpage.action?pageId=3147700 (we really should put that public)
    // for some ODT kind, we have to fill the Admin structure with the ODT stop points
    std::unordered_map<georef::Admin*, std::set<const nt::StopPoint*>> odt_stops_by_admin;
    for (const auto* route : pt_data->routes) {
        if (!route->get_odt_properties().is_zonal()) {
            continue;
        }
        // we add it for the ODT type where the vehicle comes directly to the user
        route->for_each_vehicle_journey([&](const VehicleJourney& vj) {
            if (contains({VehicleJourneyType::adress_to_stop_point, VehicleJourneyType::odt_point_to_point},
                         vj.vehicle_journey_type)) {
                for (const auto& st : vj.stop_time_list) {
                    for (auto* admin : st.stop_point->admin_list) {
                        odt_stops_by_admin[admin].insert(st.stop_point);
                    }
                }
            }
            return true;
        });
    }

    // we first store the stops in a set not to have duplicates
    for (const auto& p : odt_stops_by_admin) {
        for (const auto& sp : p.second) {
            p.first->odt_stop_points.push_back(sp);
        }
    }
}

void Data::fill_stop_point_address(
    const std::unordered_map<std::string, navitia::type::Address*>& address_by_address_id) {
    idx_t without_address = 0;
    size_t addresses_from_ntfs_counter = 0;
    bool ntfs_addresses_allowed = false;
    if (!address_by_address_id.empty()) {
        ntfs_addresses_allowed = true;
        LOG4CPLUS_INFO(log4cplus::Logger::getInstance("logger"),
                       "addresses from ntfs are present: " << address_by_address_id.size() << " nb addresses");
    }
    for (auto sp : pt_data->stop_points) {
        if (!sp->coord.is_initialized()) {
            ++without_address;
            continue;
        }
        // parse address from ntfs files
        if (ntfs_addresses_allowed && !sp->address_id.empty()) {
            auto it = address_by_address_id.find(sp->address_id);
            if (it != address_by_address_id.end()) {
                auto* way = new navitia::georef::Way;
                size_t hn_int = 0;
                try {
                    hn_int = boost::lexical_cast<int>(it->second->house_number);
                    way->name = it->second->street_name;
                } catch (const boost::bad_lexical_cast&) {
                    if (it->second->street_name.empty()) {
                        way->name = "";
                    } else if (it->second->house_number.empty()) {
                        way->name = it->second->street_name;
                    } else {
                        // if the house number is more than a number, we add house number to the name
                        way->name = it->second->house_number + " " + it->second->street_name;
                    }
                }

                // create address with way and house number
                navitia::georef::HouseNumber hn(sp->coord.lon(), sp->coord.lat(), hn_int);
                way->add_house_number(hn);
                way->admin_list = sp->admin_list;
                sp->address = new navitia::georef::Address(way, sp->coord, hn_int);
                addresses_from_ntfs_counter++;
            }
        } else {
            try {
                auto addr = geo_ref->nearest_addr(sp->coord);
                sp->address = new navitia::georef::Address(addr.second, sp->coord, addr.first);
            } catch (const navitia::proximitylist::NotFound&) {
                LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("logger"), "unable to find a way from coord ["
                                                                              << sp->coord.lon() << "-"
                                                                              << sp->coord.lat() << "]");
                ++without_address;
            }
        }
    }
    if (ntfs_addresses_allowed) {
        LOG4CPLUS_INFO(log4cplus::Logger::getInstance("logger"),
                       "addresses from ntfs - " << addresses_from_ntfs_counter << " nb match with stop point");
    }
    if (without_address) {
        LOG4CPLUS_WARN(log4cplus::Logger::getInstance("logger"), without_address << " StopPoints without address");
    }
}

void Data::build_grid_validity_pattern() {
    for (Calendar* cal : this->pt_data->calendars) {
        cal->build_validity_pattern(meta->production_date);
    }
}

void Data::compute_labels() {
    // labels are to be used as better name

    // for stop points, stop areas and poi we want to add the admin name
    for (auto sp : pt_data->stop_points) {
        sp->label = sp->name + get_admin_name(sp);
    }
    for (auto sa : pt_data->stop_areas) {
        sa->label = sa->name + get_admin_name(sa);
    }
    for (auto poi : geo_ref->pois) {
        poi->label = poi->name + get_admin_name(poi);
    }
    // for admin we want the post code
    for (auto admin : geo_ref->admins) {
        std::string post_code;
        post_code = admin->get_range_postal_codes();
        if (post_code.empty()) {
            admin->label = admin->name;
        } else {
            admin->label = admin->name + " (" + post_code + ")";
        }
    }
}

#define GET_DATA(type_name, collection_name)                           \
    template <>                                                        \
    const std::vector<type_name*>& Data::get_data<type_name>() const { \
        return this->pt_data->collection_name;                         \
    }                                                                  \
    template <>                                                        \
    std::vector<type_name*>& Data::get_data<type_name>() {             \
        return this->pt_data->collection_name;                         \
    }
ITERATE_NAVITIA_PT_TYPES(GET_DATA)
#undef GET_DATA

template <>
const std::vector<georef::POI*>& Data::get_data<georef::POI>() const {
    return this->geo_ref->pois;
}
template <>
const std::vector<georef::POIType*>& Data::get_data<georef::POIType>() const {
    return this->geo_ref->poitypes;
}
template <>
const std::vector<StopPointConnection*>& Data::get_data<StopPointConnection>() const {
    return this->pt_data->stop_point_connections;
}
template <>
const ObjFactory<MetaVehicleJourney>& Data::get_data<MetaVehicleJourney>() const {
    return this->pt_data->meta_vjs;
}

// JP and JPP can't work with automatic build clause
template <>
const std::vector<routing::JourneyPattern*>& Data::get_data<routing::JourneyPattern>() const {
    static const std::vector<routing::JourneyPattern*> res;
    return res;
}
template <>
const std::vector<routing::JourneyPatternPoint*>& Data::get_data<routing::JourneyPatternPoint>() const {
    static const std::vector<routing::JourneyPatternPoint*> res;
    return res;
}

template <>
const std::vector<boost::weak_ptr<type::disruption::Impact>>& Data::get_data<type::disruption::Impact>() const {
    return pt_data->disruption_holder.get_weak_impacts();
}

#define GET_ASSOCIATIVE_DATA(type_name, collection_name)                                         \
    template <>                                                                                  \
    const ContainerTrait<type_name>::associative_type& Data::get_assoc_data<type_name>() const { \
        return this->pt_data->collection_name##_map;                                             \
    }
ITERATE_NAVITIA_PT_TYPES(GET_ASSOCIATIVE_DATA)
#undef GET_ASSOCIATIVE_DATA

template <>
const ContainerTrait<georef::POI>::associative_type& Data::get_assoc_data<georef::POI>() const {
    return this->geo_ref->poi_map;
}
template <>
const ContainerTrait<georef::POIType>::associative_type& Data::get_assoc_data<georef::POIType>() const {
    return this->geo_ref->poitype_map;
}
template <>
const ContainerTrait<StopPointConnection>::associative_type& Data::get_assoc_data<StopPointConnection>() const {
    return this->pt_data->stop_point_connections;
}
template <>
const ContainerTrait<MetaVehicleJourney>::associative_type& Data::get_assoc_data<MetaVehicleJourney>() const {
    return this->pt_data->meta_vjs;
}

// JP and JPP can't work with automatic build clause
template <>
const ContainerTrait<routing::JourneyPattern>::associative_type& Data::get_assoc_data<routing::JourneyPattern>() const {
    static const ContainerTrait<routing::JourneyPattern>::associative_type res;
    return res;
}
template <>
const ContainerTrait<routing::JourneyPatternPoint>::associative_type&
Data::get_assoc_data<routing::JourneyPatternPoint>() const {
    static const ContainerTrait<routing::JourneyPatternPoint>::associative_type res;
    return res;
}

template <>
const ContainerTrait<type::disruption::Impact>::associative_type& Data::get_assoc_data<type::disruption::Impact>()
    const {
    return pt_data->disruption_holder.get_weak_impacts();
}

size_t Data::get_nb_obj(Type_e type) const {
    switch (type) {
#define GET_NUM_ELEMENTS(type_name, collection_name) \
    case Type_e::type_name:                          \
        return this->pt_data->collection_name.size();
        ITERATE_NAVITIA_PT_TYPES(GET_NUM_ELEMENTS)
        case Type_e::JourneyPattern:
            return dataRaptor->jp_container.nb_jps();
        case Type_e::JourneyPatternPoint:
            return dataRaptor->jp_container.nb_jpps();
        case Type_e::POI:
            return this->geo_ref->pois.size();
        case Type_e::POIType:
            return this->geo_ref->poitypes.size();
        case Type_e::Connection:
            return this->pt_data->stop_point_connections.size();
        case Type_e::MetaVehicleJourney:
            return this->pt_data->meta_vjs.size();
        case Type_e::Impact:
            return pt_data->disruption_holder.get_weak_impacts().size();
        default:
            LOG4CPLUS_ERROR(log4cplus::Logger::getInstance("data"), "unknow collection, returing 0");
    }
    return 0;
}

Indexes Data::get_all_index(Type_e type) const {
    auto num_elements = get_nb_obj(type);
    Indexes indexes;
    indexes.reserve(num_elements);
    for (size_t i = 0; i < num_elements; i++) {
        indexes.insert(i);
    }

    return indexes;
}

std::set<idx_t> Data::get_target_by_source(Type_e source, Type_e target, const std::set<idx_t>& source_idx) const {
    std::set<idx_t> result;
    for (idx_t idx : source_idx) {
        Indexes tmp = get_target_by_one_source(source, target, idx);
        // TODO: Use set's merge when we pass to c++17
        result.insert(tmp.begin(), tmp.end());
    }
    return result;
}

Indexes Data::get_target_by_one_source(Type_e source, Type_e target, idx_t source_idx) const {
    Indexes result;
    if (source_idx == invalid_idx) {
        return result;
    }
    if (source == target) {
        result.insert(source_idx);
        return result;
    }
    const auto& jp_container = dataRaptor->jp_container;
    if (target == Type_e::JourneyPattern) {
        switch (source) {
            case Type_e::Route:
                for (const auto& jpp : jp_container.get_jps_from_route()[routing::RouteIdx(source_idx)]) {
                    result.insert(jpp.val);  // TODO use bulk insert ?
                }
                break;
            case Type_e::VehicleJourney:
                result.insert(jp_container.get_jp_from_vj()[routing::VjIdx(source_idx)].val);
                break;
            case Type_e::PhysicalMode:
                for (const auto& jpp : jp_container.get_jps_from_phy_mode()[routing::PhyModeIdx(source_idx)]) {
                    result.insert(jpp.val);  // TODO use bulk insert ?
                }
                break;
            case Type_e::JourneyPatternPoint:
                result.insert(jp_container.get(routing::JppIdx(source_idx)).jp_idx.val);
                break;
            default:
                break;
        }
        return result;
    }
    if (target == Type_e::JourneyPatternPoint) {
        switch (source) {
            case Type_e::PhysicalMode:
                for (const auto& jpp : jp_container.get_jpps_from_phy_mode()[routing::PhyModeIdx(source_idx)]) {
                    result.insert(jpp.val);
                }
                break;
            case Type_e::StopPoint:
                for (const auto& jpp : dataRaptor->jpps_from_sp[routing::SpIdx(source_idx)]) {
                    result.insert(jpp.idx.val);  // TODO use bulk insert ?
                }
                break;
            case Type_e::JourneyPattern:
                for (const auto& jpp_idx : jp_container.get(routing::JpIdx(source_idx)).jpps) {
                    result.insert(jpp_idx.val);  // TODO use bulk insert ?
                }
                break;
            default:
                break;
        }
        return result;
    }
    switch (source) {
        case Type_e::JourneyPattern: {
            const auto& jp = jp_container.get(routing::JpIdx(source_idx));
            switch (target) {
                case Type_e::Route:
                    result.insert(jp.route_idx.val);
                    break;
                case Type_e::JourneyPatternPoint: /* already done */
                    break;
                case Type_e::VehicleJourney:
                    for (const auto& vj : jp.discrete_vjs) {
                        result.insert(vj->idx);
                    }  // TODO use bulk insert ?
                    for (const auto& vj : jp.freq_vjs) {
                        result.insert(vj->idx);
                    }  // TODO use bulk insert ?
                    break;
                case Type_e::PhysicalMode:
                    result.insert(jp.phy_mode_idx.val);
                    break;
                default:
                    break;
            }
            break;
        }
        case Type_e::JourneyPatternPoint:
            switch (target) {
                case Type_e::JourneyPattern: /* already done */
                    break;
                case Type_e::StopPoint:
                    result.insert(jp_container.get(routing::JppIdx(source_idx)).sp_idx.val);
                    break;
                default:
                    break;
            }
            break;
#define GET_INDEXES(type_name, collection_name)                               \
    case Type_e::type_name:                                                   \
        result = pt_data->collection_name[source_idx]->get(target, *pt_data); \
        break;
            ITERATE_NAVITIA_PT_TYPES(GET_INDEXES)
        case Type_e::Connection:
            result = pt_data->stop_point_connections[source_idx]->get(target, *pt_data);
            break;
        case Type_e::POI:
            result = geo_ref->pois[source_idx]->get(target, *geo_ref);
            break;
        case Type_e::MetaVehicleJourney:
            result = pt_data->meta_vjs[Idx<MetaVehicleJourney>(source_idx)]->get(target, *pt_data);
            break;
        case Type_e::POIType:
            result = geo_ref->poitypes[source_idx]->get(target, *geo_ref);
            break;
        case Type_e::Impact: {
            auto impact = pt_data->disruption_holder.get_impact(source_idx);
            if (impact) {
                result = impact->get(target, *pt_data);
            }
            break;
        }
        default:
            break;
    }
    return result;
}

Type_e Data::get_type_of_id(const std::string& id) const {
    if (type::EntryPoint::is_coord(id)) {
        return Type_e::Coord;
    }
    if (id.size() > 8 && id.substr(0, 8) == "address:") {
        return Type_e::Address;
    }
    if (id.size() > 6 && id.substr(0, 6) == "admin:") {
        return Type_e::Admin;
    }
    if (id.size() > 10 && id.substr(0, 10) == "stop_area:") {
        return Type_e::StopArea;
    }
#define GET_TYPE(type_name, collection_name)                            \
    const auto& collection_name##_map = pt_data->collection_name##_map; \
    if (collection_name##_map.count(id) != 0)                           \
        return Type_e::type_name;
    ITERATE_NAVITIA_PT_TYPES(GET_TYPE)
    if (geo_ref->poitype_map.find(id) != geo_ref->poitype_map.end()) {
        return Type_e::POIType;
    }
    if (geo_ref->poi_map.find(id) != geo_ref->poi_map.end()) {
        return Type_e::POI;
    }
    if (geo_ref->way_map.find(id) != geo_ref->way_map.end()) {
        return Type_e::Address;
    }
    if (geo_ref->admin_map.find(id) != geo_ref->admin_map.end()) {
        return Type_e::Admin;
    }
    return Type_e::Unknown;
}

// We want to do a deep clone of a Data.  The problem is that there is a
// lot of pointers that point to each other, and thus writing a copy
// assignment operator is really tricky.
//
// But we already have a framework that allow this deep clone: boost
// serialize.  Maybe we can write a dedicated Archive that clone the
// object, but we didn't find any easy way to do this.  Thus, we
// stream the source object in a binary_oarchive, and then stream it
// in our object.  To avoid having the whole binary_oarchive in
// memory, we construct a pipe between 2 threads.
void Data::clone_from(const Data& from) {
    CloneHelper cloner;
    cloner(from, *this);
}

void Data::set_last_rt_data_loaded(const boost::posix_time::ptime& p) const {
    this->_last_rt_data_loaded.store(p);
}

const boost::posix_time::ptime Data::last_rt_data_loaded() const {
    return this->_last_rt_data_loaded.load();
}

}  // namespace type
}  // namespace navitia

BOOST_CLASS_VERSION(navitia::type::Data, navitia::type::Data::data_version)
