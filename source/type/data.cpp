#include "data.h"

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <fstream>
#include "filter.h"
#include "eos_portable_archive/portable_iarchive.hpp"
#include "eos_portable_archive/portable_oarchive.hpp"

namespace navitia { namespace type {

template<> std::vector<StopPoint> & Data::get() {return stop_points;}
template<> std::vector<StopArea> & Data::get() {return stop_areas;}
template<> std::vector<VehicleJourney> & Data::get() {return vehicle_journeys;}
template<> std::vector<Line> & Data::get() {return lines;}
template<> std::vector<ValidityPattern> & Data::get() {return validity_patterns;}
template<> std::vector<Route> & Data::get() {return routes;}

void Data::save(const std::string & filename) {
    std::ofstream ofs(filename.c_str());
    boost::archive::text_oarchive oa(ofs);
    oa << *this;
}

void Data::load(const std::string & filename) {
    std::ifstream ifs(filename.c_str());
    boost::archive::text_iarchive ia(ifs);
    ia >> *this;
}

void Data::save_bin(const std::string & filename) {
    std::ofstream ofs(filename.c_str(),  std::ios::out | std::ios::binary);
    boost::archive::binary_oarchive oa(ofs);
    oa << *this;
}

void Data::load_bin(const std::string & filename) {
    std::ifstream ifs(filename.c_str(),  std::ios::in | std::ios::binary);
    boost::archive::binary_iarchive ia(ifs);
    ia >> *this;
}

void Data::load_flz(const std::string & filename) {
    std::ifstream ifs(filename.c_str(),  std::ios::in | std::ios::binary);
    boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
    in.push(FastLZDecompressor(2048*500),8192*500, 8192*500);
    in.push(ifs);
    eos::portable_iarchive ia(in);
    ia >> *this;
}

void Data::save_flz(const std::string & filename) {
    std::ofstream ofs(filename.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
    boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
    out.push(FastLZCompressor(2048*500), 1024*500, 1024*500);
    out.push(ofs);
    eos::portable_oarchive oa(out);
    oa << *this;
}

/*enum Type_e {eValidityPattern, eLine, eRoute, eVehicleJourney, eStopPoint, eStopArea, eStopTime,
             eNetwork, eMode, eModeType, eCity, eConnection, eRoutePoint, eDistrict, eDepartment,
             eCompany, eVehicle};*/
std::vector<idx_t> Data::get_target_by_source(Type_e source, Type_e target, std::vector<idx_t> source_idx){
    std::vector<idx_t> result;
    result.reserve(source_idx.size());
    BOOST_FOREACH(idx_t idx, source_idx) {
        std::vector<idx_t> tmp;
        switch(source) {
        case eLine: tmp = lines[idx].get(target, *this); break;
        case eRoute: tmp = routes[idx].get(target, *this); break;
        default: break;
        }
        result.insert(result.end(), tmp.begin(), tmp.end());
    }
    return result;
}

}} //namespace navitia::type
