#include "pt_data.h"

namespace navitia{namespace type {


template<> std::vector<StopPoint> & PT_Data::get() {return stop_points;}
template<> std::vector<StopArea> & PT_Data::get() {return stop_areas;}
template<> std::vector<VehicleJourney> & PT_Data::get() {return vehicle_journeys;}
template<> std::vector<Line> & PT_Data::get() {return lines;}
template<> std::vector<ValidityPattern> & PT_Data::get() {return validity_patterns;}
template<> std::vector<Route> & PT_Data::get() {return routes;}

std::vector<idx_t> PT_Data::get_target_by_source(Type_e source, Type_e target, std::vector<idx_t> source_idx){
    std::vector<idx_t> result;
    result.reserve(source_idx.size());
    BOOST_FOREACH(idx_t idx, source_idx) {
        std::vector<idx_t> tmp;
        tmp = get_target_by_one_source(source, target, idx);
        result.insert(result.end(), tmp.begin(), tmp.end());

    }
    return result;
}

std::vector<idx_t> PT_Data::get_target_by_one_source(Type_e source, Type_e target, idx_t source_idx){
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

std::vector<idx_t> PT_Data::get_all_index(Type_e type){
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

template<> std::vector<Line> & PT_Data::get_data<eLine>(){return lines;}
template<> std::vector<ValidityPattern> & PT_Data::get_data<eValidityPattern>(){return validity_patterns;}
template<> std::vector<Route> & PT_Data::get_data<eRoute>(){return routes;}
template<> std::vector<VehicleJourney> & PT_Data::get_data<eVehicleJourney>(){return vehicle_journeys;}
template<> std::vector<StopPoint> & PT_Data::get_data<eStopPoint>(){return stop_points;}
template<> std::vector<StopArea> & PT_Data::get_data<eStopArea>(){return stop_areas;}
template<> std::vector<StopTime> & PT_Data::get_data<eStopTime>(){return stop_times;}
template<> std::vector<Network> & PT_Data::get_data<eNetwork>(){return networks;}
template<> std::vector<Mode> & PT_Data::get_data<eMode>(){return modes;}
template<> std::vector<ModeType> & PT_Data::get_data<eModeType>(){return mode_types;}
template<> std::vector<City> & PT_Data::get_data<eCity>(){return cities;}
template<> std::vector<Connection> & PT_Data::get_data<eConnection>(){return connections;}
template<> std::vector<RoutePoint> & PT_Data::get_data<eRoutePoint>(){return route_points;}
template<> std::vector<District> & PT_Data::get_data<eDistrict>(){return districts;}
template<> std::vector<Department> & PT_Data::get_data<eDepartment>(){return departments;}
template<> std::vector<Company> & PT_Data::get_data<eCompany>(){return companies;}
template<> std::vector<Vehicle> & PT_Data::get_data<eVehicle>(){return vehicles;}
template<> std::vector<Country> & PT_Data::get_data<eCountry>(){return countries;}


void PT_Data::build_first_letter(){
    BOOST_FOREACH(StopArea sa, this->stop_areas){
        if(sa.city_idx < this->cities.size())
            this->stop_area_first_letter.add_string(sa.name + " " + cities[sa.city_idx].name, sa.idx);
        else
            this->stop_area_first_letter.add_string(sa.name, sa.idx);
    }

    this->stop_area_first_letter.build();

    BOOST_FOREACH(StopPoint sp, this->stop_points){
        if(sp.city_idx < this->cities.size())
            this->stop_point_first_letter.add_string(sp.name + " " + cities[sp.city_idx].name, sp.idx);
        else
            this->stop_point_first_letter.add_string(sp.name, sp.idx);
    }

    this->stop_point_first_letter.build();


    BOOST_FOREACH(City city, cities){
        this->city_first_letter.add_string(city.name, city.idx);
    }
    this->city_first_letter.build();
}

void PT_Data::build_proximity_list() {
    BOOST_FOREACH(City city, this->cities){
        this->city_proximity_list.add(city.coord, city.idx);
    }
    this->city_proximity_list.build();

    BOOST_FOREACH(StopArea stop_area, this->stop_areas){
        this->stop_area_proximity_list.add(stop_area.coord, stop_area.idx);
    }
    this->stop_area_proximity_list.build();

    BOOST_FOREACH(StopPoint stop_point, this->stop_points){
        this->stop_point_proximity_list.add(stop_point.coord, stop_point.idx);
    }
    this->stop_point_proximity_list.build();
}

void PT_Data::build_external_code() {
    BOOST_FOREACH(Line line, this->lines){line_map[line.external_code] = line.idx;}
    BOOST_FOREACH(Route route, this->routes){route_map[route.external_code] = route.idx;}
    BOOST_FOREACH(VehicleJourney vj, this->vehicle_journeys){vehicle_journey_map[vj.external_code] = vj.idx;}
    BOOST_FOREACH(StopArea sa, this->stop_areas){stop_area_map[sa.external_code] = sa.idx;}
    BOOST_FOREACH(StopPoint sp, this->stop_points){stop_point_map[sp.external_code] = sp.idx;}
    BOOST_FOREACH(Network network, this->networks){network_map[network.external_code] = network.idx;}
    BOOST_FOREACH(Mode mode, this->modes){mode_map[mode.external_code] = mode.idx;}
    BOOST_FOREACH(ModeType mode_type, this->mode_types){mode_type_map[mode_type.external_code] = mode_type.idx;}
    BOOST_FOREACH(City city, this->cities){city_map[city.external_code] = city.idx;}
    BOOST_FOREACH(District district, this->districts){district_map[district.external_code] = district.idx;}
    BOOST_FOREACH(Department department, this->departments){department_map[department.external_code] = department.idx;}
    BOOST_FOREACH(Company company, this->companies){company_map[company.external_code] = company.idx;}
    BOOST_FOREACH(Country country, this->countries){country_map[country.external_code] = country.idx;}
}

}}
