#include "data.h"

#include <fstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/foreach.hpp>

#include "third_party/eos_portable_archive/portable_iarchive.hpp"
#include "third_party/eos_portable_archive/portable_oarchive.hpp"
#include "lz4_filter/filter.h"

namespace pt = boost::posix_time;

namespace navitia { namespace type {

Data& Data::operator=(Data&& other){
    version = other.version;
    loaded = other.loaded;
    meta = other.meta;
    Alias_List = other.Alias_List;
    pt_data = std::move(other.pt_data);
    //street_network = other.street_network;
    geo_ref = other.geo_ref;
    dataRaptor = other.dataRaptor;
    last_load = other.last_load;
    last_load_at = other.last_load_at;

    return *this;
}



void Data::set_cities(){
   // BOOST_FOREACH(navitia::streetnetwork::Way way, street_network.ways){
     BOOST_FOREACH(navitia::georef::Way way, geo_ref.ways){
        auto city_it = pt_data.city_map.find(way.city);
        if(city_it != pt_data.city_map.end()){
            way.city_idx = city_it->second;
        }else{
            way.city_idx = invalid_idx;
        }
    }
}

void Data::load_lz4(const std::string & filename) {
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    try{
        last_load_at = pt::microsec_clock::local_time();
        std::ifstream ifs(filename.c_str(),  std::ios::in | std::ios::binary);
        boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
        in.push(LZ4Decompressor(2048*500),8192*500, 8192*500);
        in.push(ifs);
        eos::portable_iarchive ia(in);
        ia >> *this;
        for(size_t i = 0; i < this->pt_data.stop_times.size(); ++i)
            this->pt_data.stop_times[i].idx = i;
        last_load = true;
    }catch(std::exception& ex){
        LOG4CPLUS_ERROR(logger, boost::format("le chargement des données à échoué: %s") % ex.what());
        last_load = false;
        throw;
    }catch(...){
        LOG4CPLUS_ERROR(logger, "le chargement des données à échoué");
        last_load = false;
        throw;
    }
}

void Data::lz4(const std::string & filename) {
    std::ofstream ofs(filename.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
    boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
    out.push(LZ4Compressor(2048*500), 1024*500, 1024*500);
    out.push(ofs);
    eos::portable_oarchive oa(out);
    oa << *this;
}

void Data::build_external_code(){
    this->pt_data.build_external_code();
}

void Data::build_proximity_list(){
    this->pt_data.build_proximity_list();
    //this->street_network.build_proximity_list();
    this->geo_ref.build_proximity_list();
    this->geo_ref.project_stop_points(this->pt_data.stop_points);
}


void Data::build_first_letter(){
    pt_data.build_first_letter();

    //BOOST_FOREACH(auto way, street_network.ways){
    BOOST_FOREACH(auto way, geo_ref.ways){
        if(way.city_idx < pt_data.cities.size())
            //street_network.fl.add_string(way.name + " " + pt_data.cities[way.city_idx].name, way.idx);
            geo_ref.fl.add_string(way.name + " " + pt_data.cities[way.city_idx].name, way.idx);
        else
            //street_network.fl.add_string(way.name, way.idx);
            geo_ref.fl.add_string(way.name, way.idx);
    }
    //this->street_network.fl.build();
    this->geo_ref.fl.build();
}

void Data::build_raptor() {
    dataRaptor.load(this->pt_data);
}

}} //namespace navitia::type
