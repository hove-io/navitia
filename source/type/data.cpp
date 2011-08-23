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
        case eVehicleJourney: tmp = vehicle_journeys[idx].get(target, *this); break;
        case eStopPoint: tmp = stop_points[idx].get(target, *this); break;
        case eStopArea: tmp = stop_areas[idx].get(target, *this); break;
        case eStopTime: tmp = stop_times[idx].get(target, *this); break;
        case eNetwork: tmp = networks[idx].get(target, *this); break;
        case eMode: tmp = modes[idx].get(target, *this); break;
        case eModeType: tmp = mode_types[idx].get(target, *this); break;
        case eCity: tmp = cities[idx].get(target, *this); break;
        //case eDistrict: tmp = di
        //case eDepartment: tmp = de
        //case eCompany: tmp =
        //case eVehicle: tmp =
        default: break;
        }
        result.insert(result.end(), tmp.begin(), tmp.end());
    }
    return result;
}

std::vector<idx_t> Data::get_all_index(Type_e type){
    switch(type){
    case eLine: return get_all_index<eLine>(); break;
    case eValidityPattern: return get_all_index<eLine>(); break;
    case eRoute: return get_all_index<eLine>(); break;
    case eVehicleJourney: return get_all_index<eLine>(); break;
    case eStopPoint: return get_all_index<eLine>(); break;
    case eStopArea: return get_all_index<eLine>(); break;
    case eStopTime: return get_all_index<eLine>(); break;
    case eNetwork: return get_all_index<eLine>(); break;
    case eMode: return get_all_index<eLine>(); break;
    case eModeType: return get_all_index<eLine>(); break;
    case eCity: return get_all_index<eLine>(); break;
    case eConnection: return get_all_index<eLine>(); break;
    case eRoutePoint: return get_all_index<eLine>(); break;
    case eDistrict: return get_all_index<eLine>(); break;
    case eDepartment: return get_all_index<eLine>(); break;
    case eCompany: return get_all_index<eLine>(); break;
    case eVehicle: return get_all_index<eLine>(); break;
    case eCountry: return get_all_index<eLine>(); break;
    case eUnknown: return std::vector<idx_t>(); break;
    }
}

template<> std::vector<Line> & Data::get_data<eLine>(){return lines;}
template<> std::vector<ValidityPattern> & Data::get_data<eValidityPattern>(){return validity_patterns;}
template<> std::vector<Route> & Data::get_data<eRoute>(){return routes;}
template<> std::vector<VehicleJourney> & Data::get_data<eVehicleJourney>(){return vehicle_journeys;}
template<> std::vector<StopPoint> & Data::get_data<eStopPoint>(){return stop_points;}
template<> std::vector<StopArea> & Data::get_data<eStopArea>(){return stop_areas;}
template<> std::vector<StopTime> & Data::get_data<eStopTime>(){return stop_times;}
template<> std::vector<Network> & Data::get_data<eNetwork>(){return networks;}
template<> std::vector<Mode> & Data::get_data<eMode>(){return modes;}
template<> std::vector<ModeType> & Data::get_data<eModeType>(){return mode_types;}
template<> std::vector<City> & Data::get_data<eCity>(){return cities;}
template<> std::vector<Connection> & Data::get_data<eConnection>(){return connections;}
template<> std::vector<RoutePoint> & Data::get_data<eRoutePoint>(){return route_points;}
template<> std::vector<District> & Data::get_data<eDistrict>(){return districts;}
template<> std::vector<Department> & Data::get_data<eDepartment>(){return departments;}
template<> std::vector<Company> & Data::get_data<eCompany>(){return companies;}
template<> std::vector<Vehicle> & Data::get_data<eVehicle>(){return vehicles;}
template<> std::vector<Country> & Data::get_data<eCountry>(){return countries;}

}} //namespace navitia::type
