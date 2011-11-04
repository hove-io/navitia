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
#include <boost/foreach.hpp>

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
        tmp = get_target_by_one_source(source, target, idx);
        result.insert(result.end(), tmp.begin(), tmp.end());

    }
    return result;
}

std::vector<idx_t> Data::get_target_by_one_source(Type_e source, Type_e target, idx_t source_idx){
    std::vector<idx_t> result;
    if(source == target){
        result.push_back(source_idx);
        return result;
    }
    switch(source) {
        case eLine: result = lines[source_idx].get(target, *this); break;
        case eRoute: result = routes[source_idx].get(target, *this); break;
        case eVehicleJourney: result = vehicle_journeys[source_idx].get(target, *this); break;
        case eStopPoint: result = stop_points[source_idx].get(target, *this); break;
        case eStopArea: result = stop_areas[source_idx].get(target, *this); break;
        case eStopTime: result = stop_times[source_idx].get(target, *this); break;
        case eNetwork: result = networks[source_idx].get(target, *this); break;
        case eMode: result = modes[source_idx].get(target, *this); break;
        case eModeType: result = mode_types[source_idx].get(target, *this); break;
        case eCity: result = cities[source_idx].get(target, *this); break;
        case eDistrict: result = districts[source_idx].get(target, *this); break;
        case eDepartment: result = departments[source_idx].get(target, *this); break;
        case eCompany: result = companies[source_idx].get(target, *this); break;
        case eVehicle: result = vehicles[source_idx].get(target, *this); break;
        case eValidityPattern: result = validity_patterns[source_idx].get(target, *this); break;
        case eConnection: result = connections[source_idx].get(target, *this); break;
        case eCountry: result = countries[source_idx].get(target, *this); break;
        case eRoutePoint: result = route_points[source_idx].get(target, *this); break;
        case eUnknown: break;
    }
    return result;
}

std::vector<idx_t> Data::get_all_index(Type_e type){
    switch(type){
    case eLine: return get_all_index<eLine>(); break;
    case eValidityPattern: return get_all_index<eValidityPattern>(); break;
    case eRoute: return get_all_index<eRoute>(); break;
    case eVehicleJourney: return get_all_index<eVehicleJourney>(); break;
    case eStopPoint: return get_all_index<eStopPoint>(); break;
    case eStopArea: return get_all_index<eStopArea>(); break;
    case eStopTime: return get_all_index<eStopTime>(); break;
    case eNetwork: return get_all_index<eNetwork>(); break;
    case eMode: return get_all_index<eMode>(); break;
    case eModeType: return get_all_index<eModeType>(); break;
    case eCity: return get_all_index<eCity>(); break;
    case eConnection: return get_all_index<eConnection>(); break;
    case eRoutePoint: return get_all_index<eRoutePoint>(); break;
    case eDistrict: return get_all_index<eDistrict>(); break;
    case eDepartment: return get_all_index<eDepartment>(); break;
    case eCompany: return get_all_index<eCompany>(); break;
    case eVehicle: return get_all_index<eVehicle>(); break;
    case eCountry: return get_all_index<eCountry>(); break;
    case eUnknown:  break;
    }
    return std::vector<idx_t>();
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


void Data::build_first_letter(){
    BOOST_FOREACH(StopArea sa, stop_areas){
        this->stop_area_first_letter.add_string(sa.name + " " + cities[sa.city_idx].name, sa.idx);
    }
    BOOST_FOREACH(City city, cities){
        this->city_first_letter.add_string(city.name, city.idx);
    }
    BOOST_FOREACH(auto way, street_network.ways){
        street_network.fl.add_string(way.name + " " + cities[way.city_idx].name, way.idx);
    }

    this->stop_area_first_letter.build();
    this->city_first_letter.build();
    this->street_network.fl.build();

}

}} //namespace navitia::type
