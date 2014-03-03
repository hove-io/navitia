#include "data.h"

#include <fstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include "third_party/eos_portable_archive/portable_iarchive.hpp"
#include "third_party/eos_portable_archive/portable_oarchive.hpp"
#include "lz4_filter/filter.h"
#include "utils/functions.h"
#include "utils/exception.h"

namespace pt = boost::posix_time;

namespace navitia { namespace type {

Data& Data::operator=(Data&& other){
    version = other.version;
    loaded.store(other.loaded.load());
    meta = other.meta;
    pt_data = std::move(other.pt_data);
    geo_ref = other.geo_ref;
    dataRaptor = other.dataRaptor;
    last_load = other.last_load;
    last_load_at = other.last_load_at;
    is_connected_to_rabbitmq.store(other.is_connected_to_rabbitmq.load());

    return *this;
}


bool Data::load(const std::string & filename) {
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    try {
        this->load_lz4(filename);
        this->build_raptor();
        last_load_at = pt::microsec_clock::local_time();
        last_load = true;
        loaded = true;
    } catch(const std::exception& ex) {
        LOG4CPLUS_ERROR(logger, boost::format("le chargement des données à échoué: %s") % ex.what());
        last_load = false;
    } catch(...) {
        LOG4CPLUS_ERROR(logger, "le chargement des données à échoué");
        last_load = false;
    }
    return this->last_load;
}

void Data::load_lz4(const std::string & filename) {
    std::ifstream ifs(filename.c_str(),  std::ios::in | std::ios::binary);
    boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
    in.push(LZ4Decompressor(2048*500),8192*500, 8192*500);
    in.push(ifs);
    eos::portable_iarchive ia(in);
    ia >> *this;
}

void Data::save(const std::string & filename){
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    this->save_lz4(filename);
}

void Data::save_lz4(const std::string & filename) {
    auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
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
    try {
        std::ofstream ofs(filename.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
        boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
        out.push(LZ4Compressor(2048*500), 1024*500, 1024*500);
        out.push(ofs);
        eos::portable_oarchive oa(out);
        oa << *this;
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
    }
}

void Data::build_uri(){
#define CLEAR_EXT_CODE(type_name, collection_name) this->pt_data.collection_name##_map.clear();
ITERATE_NAVITIA_PT_TYPES(CLEAR_EXT_CODE)
    this->pt_data.build_uri();
    geo_ref.build_pois();
    geo_ref.build_poitypes();
}

void Data::build_proximity_list(){
    this->pt_data.build_proximity_list();
    this->geo_ref.build_proximity_list();
    int nb_matched = this->geo_ref.project_stop_points(this->pt_data.stop_points);
    std::cout << "Nombre de stop_points accrochés au filaire de voirie : "
              << nb_matched << " sur "
              << this->pt_data.stop_points.size() << std::endl;
}


void Data::build_autocomplete(){
    pt_data.build_autocomplete(geo_ref);
    geo_ref.build_autocomplete_list();
    pt_data.compute_score_autocomplete(geo_ref);
}

void Data::build_raptor() {
    dataRaptor.load(this->pt_data);
}

ValidityPattern* Data::get_or_create_validity_pattern(ValidityPattern* ref_validity_pattern, const uint32_t time){

    if(time > 24*3600) {
        std::bitset<366> tmp_days = ref_validity_pattern->days;
        tmp_days <<= 1;
        auto find_vp_predicate = [&](ValidityPattern* vp1) { return ((tmp_days == vp1->days) && (ref_validity_pattern->beginning_date == vp1->beginning_date));};
        auto it = std::find_if(this->pt_data.validity_patterns.begin(),
                            this->pt_data.validity_patterns.end(), find_vp_predicate);
        if(it != this->pt_data.validity_patterns.end()) {
            return *(it);
        } else {
            ValidityPattern* tmp_vp = new ValidityPattern(ref_validity_pattern->beginning_date, tmp_days.to_string());
            tmp_vp->idx = this->pt_data.validity_patterns.size();
            this->pt_data.validity_patterns.push_back(tmp_vp);
            return tmp_vp;
        }
    }
    return ref_validity_pattern;
}

void Data::build_midnight_interchange(){
    for(VehicleJourney* vj : this->pt_data.vehicle_journeys){
        for(StopTime* stop : vj->stop_time_list){
            // Théorique
            stop->departure_validity_pattern = get_or_create_validity_pattern(vj->validity_pattern, stop->departure_time);
            stop->arrival_validity_pattern = get_or_create_validity_pattern(vj->validity_pattern, stop->arrival_time);
            // Adapté
            stop->departure_adapted_validity_pattern = get_or_create_validity_pattern(vj->adapted_validity_pattern, stop->departure_time);
            stop->arrival_adapted_validity_pattern = get_or_create_validity_pattern(vj->adapted_validity_pattern, stop->arrival_time);
        }
    }
}

#define GET_DATA(type_name, collection_name)\
template<> std::vector<type_name*> & \
Data::get_data<type_name>() {\
    return this->pt_data.collection_name;\
}\
template<> std::vector<type_name *> \
Data::get_data<type_name>() const {\
    return this->pt_data.collection_name;\
}
ITERATE_NAVITIA_PT_TYPES(GET_DATA)

template<> std::vector<georef::POI*> &
Data::get_data<georef::POI>() {
    return this->geo_ref.pois;
}
template<> std::vector<georef::POI*>
Data::get_data<georef::POI>() const {
    return this->geo_ref.pois;
}

template<> std::vector<georef::POIType*> &
Data::get_data<georef::POIType>() {
    return this->geo_ref.poitypes;
}
template<> std::vector<georef::POIType*>
Data::get_data<georef::POIType>() const {
    return this->geo_ref.poitypes;
}

template<> std::vector<StopPointConnection*> &
Data::get_data<StopPointConnection>() {
    return this->pt_data.stop_point_connections;
}
template<> std::vector<StopPointConnection*>
Data::get_data<StopPointConnection>() const {
    return this->pt_data.stop_point_connections;
}


std::vector<idx_t> Data::get_all_index(Type_e type) const {
    size_t num_elements = 0;
    switch(type){
    #define GET_NUM_ELEMENTS(type_name, collection_name)\
    case Type_e::type_name:\
        num_elements = this->pt_data.collection_name.size();break;
    ITERATE_NAVITIA_PT_TYPES(GET_NUM_ELEMENTS)
    case Type_e::POI: num_elements = this->geo_ref.pois.size(); break;
    case Type_e::POIType: num_elements = this->geo_ref.poitypes.size(); break;
    case Type_e::Connection:
        num_elements = this->pt_data.stop_point_connections.size(); break;
    default:  break;
    }
    std::vector<idx_t> indexes(num_elements);
    for(size_t i=0; i < num_elements; i++)
        indexes[i] = i;
    return indexes;
}



std::vector<idx_t>
Data::get_target_by_source(Type_e source, Type_e target,
                           std::vector<idx_t> source_idx) const {
    std::vector<idx_t> result;
    result.reserve(source_idx.size());
    for(idx_t idx : source_idx) {
        std::vector<idx_t> tmp;
        tmp = get_target_by_one_source(source, target, idx);
        result.insert(result.end(), tmp.begin(), tmp.end());
    }
    return result;
}

std::vector<idx_t>
Data::get_target_by_one_source(Type_e source, Type_e target,
                               idx_t source_idx) const {
    std::vector<idx_t> result;
    if(source_idx == invalid_idx)
        return result;
    if(source == target){
        result.push_back(source_idx);
        return result;
    }
    switch(source) {
    #define GET_INDEXES(type_name, collection_name)\
        case Type_e::type_name:\
            result = pt_data.collection_name[source_idx]->get(target, pt_data);\
            break;
    ITERATE_NAVITIA_PT_TYPES(GET_INDEXES)
        case Type_e::POI:
            result = geo_ref.pois[source_idx]->get(target, geo_ref);
        case Type_e::POIType:
            result = geo_ref.poitypes[source_idx]->get(target, geo_ref);
        default: break;
    }
    return result;
}

Type_e Data::get_type_of_id(const std::string & id) const {
    if(id.size()>6 && id.substr(0,6) == "coord:")
        return Type_e::Coord;
    if(id.size()>6 && id.substr(0,8) == "address:")
        return Type_e::Address;
    if(id.size()>6 && id.substr(0,6) == "admin:")
        return Type_e::Admin;
    #define GET_TYPE(type_name, collection_name) \
    auto collection_name##_map = pt_data.collection_name##_map;\
    if(collection_name##_map.find(id) != collection_name##_map.end())\
        return Type_e::type_name;
    ITERATE_NAVITIA_PT_TYPES(GET_TYPE)
    if(geo_ref.poitype_map.find(id) != geo_ref.poitype_map.end())
        return Type_e::POIType;
    if(geo_ref.poi_map.find(id) != geo_ref.poi_map.end())
        return Type_e::POI;
    if(geo_ref.way_map.find(id) != geo_ref.way_map.end())
        return Type_e::Address;
    if(geo_ref.admin_map.find(id) != geo_ref.admin_map.end())
        return Type_e::Admin;
    return Type_e::Unknown;
}


}} //namespace navitia::type
