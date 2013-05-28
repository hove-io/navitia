#include "data.h"

#include <fstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>

#include "third_party/eos_portable_archive/portable_iarchive.hpp"
#include "third_party/eos_portable_archive/portable_oarchive.hpp"
#include "lz4_filter/filter.h"

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

    return *this;
}

void Data::set_admins(){
    // les points d'arrêts
    for(StopPoint & stop_point : pt_data.stop_points){
        std::vector<navitia::type::idx_t> admins=geo_ref.find_admins(stop_point.coord);
        for(navitia::type::idx_t idx : admins){
            stop_point.admin_list.push_back(idx);
        }
    }
    // les zones d'arrêts
    for(StopArea & stop_area : pt_data.stop_areas){
        std::vector<navitia::type::idx_t> admins=geo_ref.find_admins(stop_area.coord);
        for(navitia::type::idx_t idx : admins){
            stop_area.admin_list.push_back(idx);
        }
    }
    // POI
    for (navitia::georef::POI& poi : geo_ref.pois){
        std::vector<navitia::type::idx_t> admins=geo_ref.find_admins(poi.coord);
        for(navitia::type::idx_t idx : admins){
            poi.admin_list.push_back(idx);
        }
    }
}

bool Data::load(const std::string & filename) {
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    try {
        this->load_lz4(filename);
        this->build_raptor();
        last_load_at = pt::microsec_clock::local_time();
        last_load = true;
        loaded = true;        
    } catch(std::exception& ex) {
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
    std::ofstream ofs(filename.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
    boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
    out.push(LZ4Compressor(2048*500), 1024*500, 1024*500);
    out.push(ofs);
    eos::portable_oarchive oa(out);
    oa << *this;
}

void Data::build_uri(){
    this->pt_data.build_uri();
    geo_ref.normalize_extcode_way();
    geo_ref.normalize_extcode_admin();
}

void Data::build_proximity_list(){
    this->pt_data.build_proximity_list();
    this->geo_ref.build_proximity_list();
    int nb_matched = this->geo_ref.project_stop_points(this->pt_data.stop_points);
    std::cout << "Nombre de stop_points accrochés au filaire de voirie : " << nb_matched << " sur " << this->pt_data.stop_points.size() << std::endl;
}


void Data::build_autocomplete(){
    pt_data.build_autocomplete(geo_ref);
    geo_ref.build_autocomplete_list();
}

void Data::build_raptor() {
    dataRaptor.load(this->pt_data);
}

#define GET_DATA(type_name, collection_name)\
    template<> std::vector<type_name> & Data::get_data<type_name>() {return this->pt_data.collection_name;}\
    template<> std::vector<type_name> const & Data::get_data<type_name>() const {return this->pt_data.collection_name;}

ITERATE_NAVITIA_PT_TYPES(GET_DATA)

    template<> std::vector<georef::POI> & Data::get_data<georef::POI>() {return this->geo_ref.pois;}\
    template<> std::vector<georef::POI> const & Data::get_data<georef::POI>() const {return this->geo_ref.pois;}

    template<> std::vector<georef::POIType> & Data::get_data<georef::POIType>() {return this->geo_ref.poitypes;}\
    template<> std::vector<georef::POIType> const & Data::get_data<georef::POIType>() const {return this->geo_ref.poitypes;}


std::vector<idx_t> Data::get_all_index(Type_e type) const {
    size_t num_elements = 0;
    switch(type){
#define GET_NUM_ELEMENTS(type_name, collection_name) case Type_e::type_name: num_elements = this->pt_data.collection_name.size();break;
    ITERATE_NAVITIA_PT_TYPES(GET_NUM_ELEMENTS)
    case Type_e::POI: num_elements = this->geo_ref.pois.size(); break;
    case Type_e::POIType: num_elements = this->geo_ref.poitypes.size(); break;
    default:  break;
    }
    std::vector<idx_t> indexes(num_elements);
    for(size_t i=0; i < num_elements; i++)
        indexes[i] = i;
    return indexes;
}



std::vector<idx_t> Data::get_target_by_source(Type_e source, Type_e target, std::vector<idx_t> source_idx) const {
    std::vector<idx_t> result;
    result.reserve(source_idx.size());
    for(idx_t idx : source_idx) {
        std::vector<idx_t> tmp;
        tmp = get_target_by_one_source(source, target, idx);
        result.insert(result.end(), tmp.begin(), tmp.end());
    }
    return result;
}

std::vector<idx_t> Data::get_target_by_one_source(Type_e source, Type_e target, idx_t source_idx) const {
    std::vector<idx_t> result;
    if(source_idx == invalid_idx)
        return result;
    if(source == target){
        result.push_back(source_idx);
        return result;
    }
    switch(source) {
#define GET_INDEXES(type_name, collection_name) case Type_e::type_name: result = pt_data.collection_name[source_idx].get(target, pt_data);break;
    ITERATE_NAVITIA_PT_TYPES(GET_INDEXES)
        case Type_e::POI: result = geo_ref.pois[source_idx].get(target, geo_ref);
        case Type_e::POIType: result = geo_ref.poitypes[source_idx].get(target, geo_ref);
        default: break;
    }
    return result;
}

}} //namespace navitia::type
