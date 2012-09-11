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

void Data::set_cities(){
    BOOST_FOREACH(navitia::streetnetwork::Way way, street_network.ways){
        auto city_it = pt_data.city_map.find(way.city);
        if(city_it != pt_data.city_map.end()){
            way.city_idx = city_it->second;
        }else{
            way.city_idx = invalid_idx;
        }
    }
}

void Data::save(const std::string & filename) {
    std::ofstream ofs(filename.c_str());
    boost::archive::text_oarchive oa(ofs);
    oa << *this;
}

void Data::load(const std::string & filename) {
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    try{
        last_load_at = pt::microsec_clock::local_time();
        std::ifstream ifs(filename.c_str());
        boost::archive::text_iarchive ia(ifs);
        ia >> *this;
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

void Data::save_bin(const std::string & filename) {
    std::ofstream ofs(filename.c_str(),  std::ios::out | std::ios::binary);
    boost::archive::binary_oarchive oa(ofs);
    oa << *this;
}

void Data::load_bin(const std::string & filename) {
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    try{
        last_load_at = pt::microsec_clock::local_time();
        std::ifstream ifs(filename.c_str(),  std::ios::in | std::ios::binary);
        boost::archive::binary_iarchive ia(ifs);
        ia >> *this;
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
    this->street_network.build_proximity_list();
}


void Data::build_first_letter(){
    pt_data.build_first_letter();

    BOOST_FOREACH(auto way, street_network.ways){
        if(way.city_idx < pt_data.cities.size())
            street_network.fl.add_string(way.name + " " + pt_data.cities[way.city_idx].name, way.idx);
        else
            street_network.fl.add_string(way.name, way.idx);
    }
    this->street_network.fl.build();
}

void Data::build_raptor() {
    dataRaptor.load(this->pt_data);
}

}} //namespace navitia::type
