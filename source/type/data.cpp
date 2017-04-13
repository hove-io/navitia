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

#include "data.h"

#include <fstream>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/weak_ptr.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/container/container_fwd.hpp>
#include <thread>

#include "third_party/eos_portable_archive/portable_iarchive.hpp"
#include "third_party/eos_portable_archive/portable_oarchive.hpp"
#include "lz4_filter/filter.h"
#include "utils/functions.h"
#include "utils/exception.h"
#include "utils/threadbuf.h"

#include "pt_data.h"
#include "routing/dataraptor.h"
#include "georef/georef.h"
#include "fare/fare.h"
#include "type/meta_data.h"
#include "kraken/fill_disruption_from_database.h"

namespace pt = boost::posix_time;

namespace navitia { namespace type {

wrong_version::~wrong_version() noexcept {}

const unsigned int Data::data_version = 64; //< *INCREMENT* every time serialized data are modified

Data::Data(size_t data_identifier) :
    data_identifier(data_identifier),
    meta(std::make_unique<MetaData>()),
    pt_data(std::make_unique<PT_Data>()),
    geo_ref(std::make_unique<navitia::georef::GeoRef>()),
    dataRaptor(std::make_unique<navitia::routing::dataRAPTOR>()),
    fare(std::make_unique<navitia::fare::Fare>()),
    find_admins(
            [&](const GeographicalCoord &c){
            return geo_ref->find_admins(c);
            })
{
    loaded = false;
    is_connected_to_rabbitmq = false;
    is_realtime_loaded = false;
}

Data::~Data(){}

bool Data::load(const std::string& filename,
        const boost::optional<std::string>& chaos_database,
        const std::vector<std::string>& contributors) {
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    loading = true;
    try {
        std::ifstream ifs(filename.c_str(), std::ios::in | std::ios::binary);
        ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        this->load(ifs);
        last_load_at = pt::microsec_clock::universal_time();
        last_load = true;
        loaded = true;
        LOG4CPLUS_INFO(logger, boost::format("stopTimes : %d nb foot path : %d Nombre de stop points : %d")
                       % pt_data->nb_stop_times()
                       % pt_data->stop_point_connections.size()
                       % pt_data->stop_points.size()
            );
        if (chaos_database) {
            fill_disruption_from_database(*chaos_database, *pt_data, *meta, contributors);
        }
        build_raptor();
    } catch(const wrong_version& ex) {
        LOG4CPLUS_ERROR(logger, "Cannot load data: " << ex.what());
        last_load = false;
    } catch(const std::exception& ex) {
        LOG4CPLUS_ERROR(logger, "Data loading failed: " << ex.what());
        last_load = false;
    } catch(...) {
        LOG4CPLUS_ERROR(logger, "Data loading failed");
        last_load = false;
	}
    loading = false;
    return this->last_load;
}

void Data::load(std::istream& ifs) {
    boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
    in.push(LZ4Decompressor(2048*500),8192*500, 8192*500);
    in.push(ifs);
    eos::portable_iarchive ia(in);
    ia >> *this;
}


void Data::save(const std::string& filename) const {
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    boost::filesystem::path p(filename);
    boost::filesystem::path dir = p.parent_path();
    try {
       boost::filesystem::is_directory(p);
    } catch(const boost::filesystem::filesystem_error& e)
    {
       if(e.code() == boost::system::errc::permission_denied)
           LOG4CPLUS_ERROR(logger, "Search permission is denied for " << p);
       else
           LOG4CPLUS_ERROR(logger, "is_directory(" << p << ") failed with "
                     << e.code().message());
       throw navitia::exception("Unable to write file");
    }
    std::ofstream ofs(filename.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
    ofs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try{
        this->save(ofs);
    } catch(const boost::filesystem::filesystem_error &e) {
        if(e.code() == boost::system::errc::permission_denied)
            LOG4CPLUS_ERROR(logger, "Writing permission is denied for " << p);
        else if(e.code() == boost::system::errc::file_too_large)
            LOG4CPLUS_ERROR(logger, "The file " << filename << " is too large");
        else if(e.code() == boost::system::errc::interrupted)
            LOG4CPLUS_ERROR(logger, "Writing was interrupted for " << p);
        else if(e.code() == boost::system::errc::no_buffer_space)
            LOG4CPLUS_ERROR(logger, "No buffer space while writing " << p);
        else if(e.code() == boost::system::errc::not_enough_memory)
            LOG4CPLUS_ERROR(logger, "Not enough memory while writing " << p);
        else if(e.code() == boost::system::errc::no_space_on_device)
            LOG4CPLUS_ERROR(logger, "No space on device while writing " << p);
        else if(e.code() == boost::system::errc::operation_not_permitted)
            LOG4CPLUS_ERROR(logger, "Operation not permitted while writing " << p);
        LOG4CPLUS_ERROR(logger, e.what());
       throw navitia::exception("Unable to write file");
    }catch(const std::ofstream::failure& e){
       throw navitia::exception(std::string("Unable to write file: ") + e.what());
    }
}

void Data::save(std::ostream& ofs) const {
    boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
    out.push(LZ4Compressor(2048*500), 1024*500, 1024*500);
    out.push(ofs);
    eos::portable_oarchive oa(out);
    oa << *this;
}

void Data::build_uri(){
#define CLEAR_EXT_CODE(type_name, collection_name) this->pt_data->collection_name##_map.clear();
ITERATE_NAVITIA_PT_TYPES(CLEAR_EXT_CODE)
    this->pt_data->build_uri();
    geo_ref->build_pois_map();
    geo_ref->build_poitypes_map();
    geo_ref->build_admin_map();
}

void Data::build_proximity_list(){
    this->pt_data->build_proximity_list();
    this->geo_ref->build_proximity_list();
    this->geo_ref->project_stop_points(this->pt_data->stop_points);
}

void  Data::build_administrative_regions() {
    auto log = log4cplus::Logger::getInstance("ed::Data");

    // set admins to stop points
    int cpt_no_projected = 0;
    for (type::StopPoint* stop_point : pt_data->stop_points) {
        if (!stop_point->admin_list.empty()) {
            continue;
        }
        const auto &admins = find_admins(stop_point->coord);
        boost::push_back(stop_point->admin_list, admins);
        if (admins.empty()) ++cpt_no_projected;
    }
    if (cpt_no_projected)
        LOG4CPLUS_WARN(log, cpt_no_projected << "/" << pt_data->stop_points.size()
                       << " stop_points are not associated with any admins");

    // set admins to poi
    cpt_no_projected = 0;
    int cpt_no_initialized = 0;
    for (georef::POI* poi: geo_ref->pois) {
        if (!poi->coord.is_initialized()) {
            cpt_no_initialized++;
            continue;
        }
        if (!poi->admin_list.empty()) {
            continue;
        }
        const auto &admins = find_admins(poi->coord);
        boost::push_back(poi->admin_list, admins);
        if (admins.empty()) {
            ++cpt_no_projected;
        }
    }
    if (cpt_no_projected)
        LOG4CPLUS_WARN(log, cpt_no_projected << "/" << geo_ref->pois.size()
                        << " pois are not associated with any admins");
    if (cpt_no_initialized)
        LOG4CPLUS_WARN(log, cpt_no_initialized << "/" << geo_ref->pois.size()
                        << " pois with coordinates not initialized");

    this->pt_data->build_admins_stop_areas();

    for (const auto* sa: pt_data->stop_areas)
        for (auto admin: sa->admin_list)
            if (!admin->from_original_dataset)
                admin->main_stop_areas.push_back(sa);
}

void Data::build_autocomplete(){
    pt_data->build_autocomplete(*geo_ref);
    geo_ref->build_autocomplete_list();
    pt_data->compute_score_autocomplete(*geo_ref);
}

void Data::build_raptor(size_t cache_size) {
    LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"),
                    "Start to build dataRaptor");
    dataRaptor->load(*this->pt_data, cache_size);
    LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("log"),
                    "Finished to build dataRaptor");
}

ValidityPattern* Data::get_similar_validity_pattern(ValidityPattern* vp) const{
    auto find_vp_predicate = [&](ValidityPattern* vp1) { return ((*vp) == (*vp1));};
    auto it = std::find_if(this->pt_data->validity_patterns.begin(),
                        this->pt_data->validity_patterns.end(), find_vp_predicate);
    if(it != this->pt_data->validity_patterns.end()) {
        return *(it);
    } else {
        return nullptr;
    }
}

using list_cal_bitset = std::vector<std::pair<const Calendar*, ValidityPattern::year_bitset>>;

list_cal_bitset
find_matching_calendar(const Data&, const std::string& name, const ValidityPattern& validity_pattern,
                        const std::vector<Calendar*>& calendar_list, double relative_threshold) {
    list_cal_bitset res;
    //for the moment we keep lot's of trace, but they will be removed after a while
    auto log = log4cplus::Logger::getInstance("kraken::type::Data::Calendar");
    LOG4CPLUS_TRACE(log, "meta vj " << name << " :" << validity_pattern.days.to_string());

    for (const auto calendar : calendar_list) {
        // sometimes a calendar can be empty (for example if it's validity period does not
        // intersect the data's validity period)
        // we do not filter those calendar since it's a user input, but we do not match them
        if (! calendar->validity_pattern.days.any()) {
            continue;
        }
        auto diff = get_difference(calendar->validity_pattern.days, validity_pattern.days);
        size_t nb_diff = diff.count();

        LOG4CPLUS_TRACE(log, "cal " << calendar->uri << " :" << calendar->validity_pattern.days.to_string());

        //we associate the calendar to the vj if the diff are below a relative threshold
        //compared to the number of active days in the calendar
        size_t threshold = std::round(relative_threshold * calendar->validity_pattern.days.count());
        LOG4CPLUS_TRACE(log, "**** diff: " << nb_diff << " and threshold: " << threshold << (nb_diff <= threshold ? ", we keep it!!":""));

        if (nb_diff > threshold) {
            continue;
        }
        res.push_back({calendar, diff});
    }

    return res;
}

void Data::complete(){
    auto logger = log4cplus::Logger::getInstance("log");
    pt::ptime start;
    int admin, sort, autocomplete;

    build_grid_validity_pattern();
    //build_associated_calendar(); read from database
    
    start = pt::microsec_clock::local_time();
    LOG4CPLUS_INFO(logger, "Building administrative regions");
    build_administrative_regions();
    admin = (pt::microsec_clock::local_time() - start).total_milliseconds();

    aggregate_odt();

    build_relations();

    compute_labels();

    start = pt::microsec_clock::local_time();
    pt_data->sort();
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

static ValidityPattern get_union_validity_pattern(const MetaVehicleJourney& meta_vj) {
    ValidityPattern validity;
    for (const auto& vj: meta_vj.get_base_vj()) {
        if (validity.beginning_date.is_not_a_date()) {
            validity.beginning_date = vj->base_validity_pattern()->beginning_date;
        } else {
            if (validity.beginning_date != vj->base_validity_pattern()->beginning_date) {
                throw navitia::exception("the beginning date of the meta_vj are not all the same");
            }
        }
        validity.days |= vj->base_validity_pattern()->days;
    }
    return validity;
}

void Data::build_associated_calendar() {
    auto log = log4cplus::Logger::getInstance("kraken::type::Data");
    std::multimap<ValidityPattern, AssociatedCalendar*> associated_vp;
    size_t nb_not_matched_vj(0);
    size_t nb_matched(0);
    for(auto& meta_vj : this->pt_data->meta_vjs) {
        // we check the theoric vj of a meta vj
        // because we start from the postulate that the theoric VJs are the same VJ
        // split because of dst (day saving time)
        // because of that we try to match the calendar with the union of all theoric vj validity pattern
        ValidityPattern meta_vj_validity_pattern = get_union_validity_pattern(*meta_vj);

        //some check can be done on any theoric vj, we do them on the first
        auto& first_vj = meta_vj->get_base_vj().front();
        const auto& calendar_list = first_vj->route->line->calendar_list;
        if (calendar_list.empty()) {
            LOG4CPLUS_TRACE(log, "the line of the vj " << first_vj->uri << " is associated to no calendar");
            nb_not_matched_vj++;
            continue;
        }

        //we check if we already computed the associated val for this validity pattern
        //since a validity pattern can be shared by many vj
        auto it = associated_vp.find(meta_vj_validity_pattern);
        if (it != associated_vp.end()) {
            for (; it->first == meta_vj_validity_pattern; ++it) {
                meta_vj->associated_calendars.insert({it->second->calendar->uri, it->second});
            }
            continue;
        }

        auto close_cal = find_matching_calendar(*this, meta_vj->uri, meta_vj_validity_pattern, calendar_list);

        if (close_cal.empty()) {
            LOG4CPLUS_TRACE(log, "the meta vj " << meta_vj->uri << " has been attached to no calendar");
            nb_not_matched_vj++;
            continue;
        }
        nb_matched++;

        std::stringstream cal_uri;
        for (auto cal_bit_set: close_cal) {
            auto associated_calendar = new AssociatedCalendar();
            pt_data->associated_calendars.push_back(associated_calendar);

            associated_calendar->calendar = cal_bit_set.first;
            //we need to create the associated exceptions
            for (size_t i = 0; i < cal_bit_set.second.size(); ++i) {
                if (! cal_bit_set.second[i]) {
                    continue; //cal_bit_set.second is the resulting differences, so 0 means no exception
                }
                ExceptionDate ex;
                ex.date = meta_vj_validity_pattern.beginning_date + boost::gregorian::days(i);
                //if the vj is active this day it's an addition, else a removal
                ex.type = (meta_vj_validity_pattern.days[i] ? ExceptionDate::ExceptionType::add : ExceptionDate::ExceptionType::sub);
                associated_calendar->exceptions.push_back(ex);
            }

            meta_vj->associated_calendars.insert({associated_calendar->calendar->uri, associated_calendar});
            associated_vp.insert({meta_vj_validity_pattern, associated_calendar});
            cal_uri << associated_calendar->calendar->uri << " ";
        }

        LOG4CPLUS_DEBUG(log, "the meta vj " << meta_vj->uri << " has been attached to " << cal_uri.str());
    }

    LOG4CPLUS_INFO(log, nb_matched << " vehicle journeys have been matched to at least one calendar");
    if (nb_not_matched_vj) {
        LOG4CPLUS_WARN(log, "no calendar found for " << nb_not_matched_vj << " vehicle journey");
    }
}

/*
    > Fill dataset_list for route and stoppoint
    > Fill vehiclejourney_list for dataset
    > These lists are used by ptref
*/
static void build_datasets(navitia::type::VehicleJourney* vj){
    if(!vj->dataset) { return; }
    vj->route->dataset_list.insert(vj->dataset);
    vj->dataset->vehiclejourney_list.insert(vj);
    for(navitia::type::StopTime& st : vj->stop_time_list){
        if(st.stop_point){
            st.stop_point->dataset_list.insert(vj->dataset);
        }
    }
    if(vj->route && vj->route->line && vj->route->line->network){
        vj->route->line->network->dataset_list.insert(vj->dataset);
    }
}

void Data::build_relations(){
    // physical_mode_list of line
    for (auto* vj: pt_data->vehicle_journeys) {
        build_datasets(vj);
        if (! vj->physical_mode || ! vj->route || ! vj->route->line) { continue; }
        if (!navitia::contains(vj->route->line->physical_mode_list, vj->physical_mode)){
            vj->route->line->physical_mode_list.push_back(vj->physical_mode);
        }
    }
}

void Data::aggregate_odt(){
    // TODO ODT NTFSv0.3: remove that when we stop to support NTFSv0.1
    //
    // cf http://confluence.canaltp.fr/pages/viewpage.action?pageId=3147700 (we really should put that public)
    // for some ODT kind, we have to fill the Admin structure with the ODT stop points
    std::unordered_map<georef::Admin*, std::set<const nt::StopPoint*>> odt_stops_by_admin;
    for (const auto* route: pt_data->routes) {
        if (! route->get_odt_properties().is_zonal()) {
            continue;
        }
        // we add it for the ODT type where the vehicle comes directly to the user
        route->for_each_vehicle_journey([&](const VehicleJourney& vj) {
            if (in(vj.vehicle_journey_type, {VehicleJourneyType::adress_to_stop_point,
                     VehicleJourneyType::odt_point_to_point} )) {
                for (const auto& st: vj.stop_time_list) {
                    for (auto* admin: st.stop_point->admin_list) {
                        odt_stops_by_admin[admin].insert(st.stop_point);
                    }
                }
            }
            return true;
        });
    }

    //we first store the stops in a set not to have dupplicates
    for (const auto& p: odt_stops_by_admin) {
        for (const auto& sp: p.second) {
            p.first->odt_stop_points.push_back(sp);
        }
    }
}

void Data::build_grid_validity_pattern() {
    for(Calendar* cal : this->pt_data->calendars){
        cal->build_validity_pattern(meta->production_date);
    }
}

void Data::compute_labels() {
    //labels are to be used as better name

    //for stop points, stop areas and poi we want to add the admin name
    for (auto sp: pt_data->stop_points) {
        sp->label = sp->name + get_admin_name(sp);
    }
    for (auto sa: pt_data->stop_areas) {
        sa->label = sa->name + get_admin_name(sa);
    }
    for (auto poi: geo_ref->pois) {
        poi->label = poi->name + get_admin_name(poi);
    }
    //for admin we want the post code
    for (auto admin: geo_ref->admins) {
        std::string post_code;
        post_code = admin->get_range_postal_codes();
        if (post_code.empty()) {
            admin->label = admin->name;
        } else {
            admin->label = admin->name + " (" + post_code + ")";
        }
    }
}

#define GET_DATA(type_name, collection_name)\
template<> const std::vector<type_name*>& \
Data::get_data<type_name>() const {\
    return this->pt_data->collection_name;\
} \
template<> std::vector<type_name*>& \
Data::get_data<type_name>() {\
    return this->pt_data->collection_name;\
}
ITERATE_NAVITIA_PT_TYPES(GET_DATA)
#undef GET_DATA

template<> const std::vector<georef::POI*>&
Data::get_data<georef::POI>() const {
    return this->geo_ref->pois;
}
template<> const std::vector<georef::POIType*>&
Data::get_data<georef::POIType>() const {
    return this->geo_ref->poitypes;
}
template<> const std::vector<StopPointConnection*>&
Data::get_data<StopPointConnection>() const {
    return this->pt_data->stop_point_connections;
}
template<> const ObjFactory<MetaVehicleJourney>&
Data::get_data<MetaVehicleJourney>() const {
    return this->pt_data->meta_vjs;
}

// JP and JPP can't work with automatic build clause
template<> const std::vector<routing::JourneyPattern*>&
Data::get_data<routing::JourneyPattern>() const {
    static const std::vector<routing::JourneyPattern*> res;
    return res;
}
template<> const std::vector<routing::JourneyPatternPoint*>&
Data::get_data<routing::JourneyPatternPoint>() const {
    static const std::vector<routing::JourneyPatternPoint*> res;
    return res;
}

template<> const std::vector<boost::weak_ptr<type::disruption::Impact>>&
Data::get_data<type::disruption::Impact>() const {
    return pt_data->disruption_holder.get_weak_impacts();
}

#define GET_ASSOCIATIVE_DATA(type_name, collection_name)\
template<> const ContainerTrait<type_name>::associative_type& \
Data::get_assoc_data<type_name>() const {\
    return this->pt_data->collection_name##_map;\
}
ITERATE_NAVITIA_PT_TYPES(GET_ASSOCIATIVE_DATA)
#undef GET_ASSOCIATIVE_DATA

template<> const ContainerTrait<georef::POI>::associative_type&
Data::get_assoc_data<georef::POI>() const {
    return this->geo_ref->poi_map;
}
template<> const ContainerTrait<georef::POIType>::associative_type&
Data::get_assoc_data<georef::POIType>() const {
    return this->geo_ref->poitype_map;
}
template<> const ContainerTrait<StopPointConnection>::associative_type&
Data::get_assoc_data<StopPointConnection>() const {
    return this->pt_data->stop_point_connections;
}
template<> const ContainerTrait<MetaVehicleJourney>::associative_type&
Data::get_assoc_data<MetaVehicleJourney>() const {
    return this->pt_data->meta_vjs;
}

// JP and JPP can't work with automatic build clause
template<> const ContainerTrait<routing::JourneyPattern>::associative_type&
Data::get_assoc_data<routing::JourneyPattern>() const {
    static const ContainerTrait<routing::JourneyPattern>::associative_type res;
    return res;
}
template<> const ContainerTrait<routing::JourneyPatternPoint>::associative_type&
Data::get_assoc_data<routing::JourneyPatternPoint>() const {
    static const ContainerTrait<routing::JourneyPatternPoint>::associative_type res;
    return res;
}

template<> const ContainerTrait<type::disruption::Impact>::associative_type&
Data::get_assoc_data<type::disruption::Impact>() const {
    return pt_data->disruption_holder.get_weak_impacts();
}

size_t Data::get_nb_obj(Type_e type) const {
    switch(type){
    #define GET_NUM_ELEMENTS(type_name, collection_name)\
    case Type_e::type_name:\
        return this->pt_data->collection_name.size();
    ITERATE_NAVITIA_PT_TYPES(GET_NUM_ELEMENTS)
    case Type_e::JourneyPattern: return dataRaptor->jp_container.nb_jps();
    case Type_e::JourneyPatternPoint: return dataRaptor->jp_container.nb_jpps();
    case Type_e::POI: return this->geo_ref->pois.size();
    case Type_e::POIType: return this->geo_ref->poitypes.size();
    case Type_e::Connection:
        return this->pt_data->stop_point_connections.size();
    case Type_e::MetaVehicleJourney: return this->pt_data->meta_vjs.size();
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
    for(size_t i=0; i < num_elements; i++) {
        indexes.insert(i);
    }

    return indexes;
}

Indexes
Data::get_target_by_source(Type_e source, Type_e target,
                           Indexes source_idx) const {
    Indexes result;
    result.reserve(source_idx.size());
    for(idx_t idx : source_idx) {
        Indexes tmp = get_target_by_one_source(source, target, idx);
        result.insert(/*boost::container::ordered_unique_range_t(),
                        // Note the tag does not work on old boost version,
                        //   put it back when we stop boost 1.49 support*/
                        tmp.begin(), tmp.end());
    }
    return result;
}

Indexes
Data::get_target_by_one_source(Type_e source, Type_e target,
                               idx_t source_idx) const {
    Indexes result;
    if(source_idx == invalid_idx)
        return result;
    if(source == target){
        result.insert(source_idx);
        return result;
    }
    const auto& jp_container = dataRaptor->jp_container;
    if (target == Type_e::JourneyPattern) {
        switch (source) {
        case Type_e::Route:
            for (const auto& jpp: jp_container.get_jps_from_route()[routing::RouteIdx(source_idx)]) {
                result.insert(jpp.val); //TODO use bulk insert ?
            }
            break;
        case Type_e::VehicleJourney:
            result.insert(jp_container.get_jp_from_vj()[routing::VjIdx(source_idx)].val);
            break;
        case Type_e::JourneyPatternPoint:
            result.insert(jp_container.get(routing::JppIdx(source_idx)).jp_idx.val);
            break;
        default: break;
        }
        return result;
    }
    if (target == Type_e::JourneyPatternPoint) {
        switch (source) {
        case Type_e::StopPoint:
            for (const auto& jpp: dataRaptor->jpps_from_sp[routing::SpIdx(source_idx)]) {
                result.insert(jpp.idx.val); //TODO use bulk insert ?
            }
            break;
        case Type_e::JourneyPattern:
            for (const auto& jpp_idx: jp_container.get(routing::JpIdx(source_idx)).jpps) {
                result.insert(jpp_idx.val); //TODO use bulk insert ?
            }
            break;
        default: break;
        }
        return result;
    }
    switch(source) {
    case Type_e::JourneyPattern: {
        const auto& jp = jp_container.get(routing::JpIdx(source_idx));
        switch(target) {
        case Type_e::Route: result.insert(jp.route_idx.val); break;
        case Type_e::JourneyPatternPoint: /* already done */ break;
        case Type_e::VehicleJourney:
            for (const auto& vj: jp.discrete_vjs) { result.insert(vj->idx); } //TODO use bulk insert ?
            for (const auto& vj: jp.freq_vjs) { result.insert(vj->idx); } //TODO use bulk insert ?
            break;
        default: break;
        }
        break;
    }
    case Type_e::JourneyPatternPoint:
        switch(target) {
        case Type_e::JourneyPattern: /* already done */ break;
        case Type_e::StopPoint:
            result.insert(jp_container.get(routing::JppIdx(source_idx)).sp_idx.val);
            break;
        default: break;
        }
        break;
#define GET_INDEXES(type_name, collection_name) \
    case Type_e::type_name:                                         \
        result = pt_data->collection_name[source_idx]->get(target, *pt_data); \
        break;
    ITERATE_NAVITIA_PT_TYPES(GET_INDEXES)
    case Type_e::POI:
        result = geo_ref->pois[source_idx]->get(target, *geo_ref);
        break;
    case Type_e::MetaVehicleJourney:
        result = pt_data->meta_vjs[Idx<MetaVehicleJourney>(source_idx)]->get(target, *pt_data);
        break;
    case Type_e::POIType:
        result = geo_ref->poitypes[source_idx]->get(target, *geo_ref);
        break;
    default: break;
    }
    return result;
}

Type_e Data::get_type_of_id(const std::string & id) const {
    if(type::EntryPoint::is_coord(id))
        return Type_e::Coord;
    if(id.size()>8 && id.substr(0,8) == "address:")
        return Type_e::Address;
    if(id.size()>6 && id.substr(0,6) == "admin:")
        return Type_e::Admin;
    if(id.size()>10 && id.substr(0,10) == "stop_area:")
        return Type_e::StopArea;
    #define GET_TYPE(type_name, collection_name) \
    const auto &collection_name##_map = pt_data->collection_name##_map;\
    if(collection_name##_map.count(id) != 0 )\
        return Type_e::type_name;
    ITERATE_NAVITIA_PT_TYPES(GET_TYPE)
    if(geo_ref->poitype_map.find(id) != geo_ref->poitype_map.end())
        return Type_e::POIType;
    if(geo_ref->poi_map.find(id) != geo_ref->poi_map.end())
        return Type_e::POI;
    if(geo_ref->way_map.find(id) != geo_ref->way_map.end())
        return Type_e::Address;
    if(geo_ref->admin_map.find(id) != geo_ref->admin_map.end())
        return Type_e::Admin;
    return Type_e::Unknown;
}

namespace {
struct Pipe {
    threadbuf sbuf;
    std::ostream out;
    std::istream in;
    Pipe(): out(&sbuf), in(&sbuf) {}
    Pipe(const Pipe&) = delete;
    Pipe& operator=(const Pipe&) = delete;
    ~Pipe() {sbuf.close();}
};
} // anonymous namespace

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
    Pipe p;
    std::thread write([&]() {boost::archive::binary_oarchive oa(p.out); oa << from;});
    { boost::archive::binary_iarchive ia(p.in); ia >> *this; }
    write.join();
}

}} //namespace navitia::type

BOOST_CLASS_VERSION(navitia::type::Data, navitia::type::Data::data_version)
