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
    Alias_List = other.Alias_List;
    pt_data = std::move(other.pt_data);
    geo_ref = other.geo_ref;
    dataRaptor = other.dataRaptor;
    last_load = other.last_load;
    last_load_at = other.last_load_at;

    return *this;
}


void Data::set_cities(){
     for(navitia::georef::Way & way : geo_ref.ways) {
        auto city_it = pt_data.cities_map.find("city:"+way.city);
        if(city_it != pt_data.cities_map.end()) {
            way.city_idx = city_it->second;
        } else {
            way.city_idx = invalid_idx;
        }
    }
}

bool Data::load(const std::string & filename) {
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    try {
        this->load_lz4(filename);
        this->build_raptor();
        last_load_at = pt::microsec_clock::local_time();
        for(size_t i = 0; i < this->pt_data.stop_times.size(); ++i)
            this->pt_data.stop_times[i].idx = i;
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
}

void Data::build_proximity_list(){
    this->pt_data.build_proximity_list();
    this->geo_ref.build_proximity_list();
    this->geo_ref.project_stop_points(this->pt_data.stop_points);
}


void Data::build_autocomplete(){
    pt_data.build_autocomplete();

    for(auto way : geo_ref.ways){
        if(way.city_idx < pt_data.cities.size())
            geo_ref.fl.add_string(way.name + " " + pt_data.cities[way.city_idx].name, way.idx);
        else
            geo_ref.fl.add_string(way.name, way.idx);
    }
    this->geo_ref.fl.build();
}

void Data::build_raptor() {
    dataRaptor.load(this->pt_data);
}

}} //namespace navitia::type
