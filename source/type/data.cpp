#include "data.h"

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <fstream>
#include "filter.h"

namespace navitia { namespace type {
void Data::build_index(){
  //  stoppoint_of_stoparea.create(stop_areas, stop_points, &StopPoint::stop_area_idx);
  //  stop_area_by_name.create(stop_areas, &StopArea::name);
    
}

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

void Data::load_lz(const std::string & filename) {
    std::ifstream ifs(filename.c_str(),  std::ios::in | std::ios::binary);
    boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
    in.push(FastLZDecompressor(2048*500),8192*500, 8192*500);
    in.push(ifs);
    boost::archive::binary_iarchive ia(ifs);
    ia >> *this;
}

}} //namespace navitia::type
