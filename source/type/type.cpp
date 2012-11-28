#include "type.h"
#include "pt_data.h"
#include <iostream>
#include <boost/assign.hpp>

namespace navitia { namespace type {

bool ValidityPattern::is_valid(int duration) const {
    if(duration < 0){

        std::cerr << "La date est avant le début de période(" << beginning_date << ")" << std::endl;
        return false;
    }
    else if(duration > 366){
        std::cerr << "La date dépasse la fin de période " << duration << " " << std::endl;
        return false;
    }
    return true;
}

int ValidityPattern::slide(boost::gregorian::date day) const {
    return (day - beginning_date).days();
}

void ValidityPattern::add(boost::gregorian::date day){
    long duration = slide(day);
    add(duration);
}

void ValidityPattern::add(int duration){
    if(is_valid(duration))
        days[duration] = true;
}

void ValidityPattern::remove(boost::gregorian::date date){
    long duration = slide(date);
    remove(duration);
}

void ValidityPattern::remove(int day){
    if(is_valid(day))
        days[day] = false;
}

std::string ValidityPattern::str() const {
    return days.to_string();
}

bool ValidityPattern::check(boost::gregorian::date day) const {
    long duration = slide(day);
    return ValidityPattern::check(duration);
}

bool ValidityPattern::check(unsigned int day) const {
//    BOOST_ASSERT(is_valid(day));
    return days[day];
}

bool ValidityPattern::check2(unsigned int day) const {
//    BOOST_ASSERT(is_valid(day));
    if(day == 0)
        return days[day] || days[day+1];
    else
        return days[day-1] || days[day] || days[day+1];
}

bool ValidityPattern::uncheck2(unsigned int day) const {
//    BOOST_ASSERT(is_valid(day));
    if(day == 0)
        return !days[day] && !days[day+1];
    else
        return !days[day-1] && !days[day] && !days[day+1];
}

double GeographicalCoord::distance_to(const GeographicalCoord &other) const{
    static const double EARTH_RADIUS_IN_METERS = 6372797.560856;
    double longitudeArc = (this->lon() - other.lon()) * DEG_TO_RAD;
    double latitudeArc  = (this->lat() - other.lat()) * DEG_TO_RAD;
    double latitudeH = sin(latitudeArc * 0.5);
    latitudeH *= latitudeH;
    double lontitudeH = sin(longitudeArc * 0.5);
    lontitudeH *= lontitudeH;
    double tmp = cos(this->lat()*DEG_TO_RAD) * cos(other.lat()*DEG_TO_RAD);
    return EARTH_RADIUS_IN_METERS * 2.0 * asin(sqrt(latitudeH + tmp*lontitudeH));
}

bool operator==(const GeographicalCoord & a, const GeographicalCoord & b){
    return a.distance_to(b) < 0.1; // soit 0.1m
}

std::pair<GeographicalCoord, float> GeographicalCoord::project(GeographicalCoord segment_start, GeographicalCoord segment_end) const{
    std::pair<GeographicalCoord, float> result;

    double dlon = segment_end._lon - segment_start._lon;
    double dlat = segment_end._lat - segment_start._lat;
    double length = ::sqrt(dlon * dlon + dlat * dlat);
    double u;

    // On gère le cas où le segment est particulièrement court, et donc ça peut poser des problèmes (à cause de la division par length²)
    if(length < 1.0){ // moins de un, on projette sur une extrémité
        if(this->distance_to(segment_start) < this->distance_to(segment_end))
            u = 0;
        else
            u = 1;
    } else {
        u = ((this->_lon - segment_start._lon)*dlon + (this->_lat - segment_start._lat)*dlat )/
                (length * length);
    }

    // Les deux cas où le projeté tombe en dehors
    if(u < 0)
        result = std::make_pair(segment_start, this->distance_to(segment_start));
    else if(u > 1)
        result = std::make_pair(segment_end, this->distance_to(segment_end));
    else {
        result.first._lon = segment_start._lon + u * (segment_end._lon - segment_start._lon);
        result.first._lat = segment_start._lat + u * (segment_end._lat - segment_start._lat);
        result.second = this->distance_to(result.first);
    }

    return result;
}


std::ostream & operator<<(std::ostream & os, const GeographicalCoord & coord){
    os << coord.lon() << ";" << coord.lat();
    return os;
}

static_data * static_data::instance = 0;
static_data * static_data::get() {
    if (instance == 0) {
        static_data* temp = new static_data();

        boost::assign::insert(temp->types_string)
                (Type_e::eValidityPattern, "validity_pattern")
                (Type_e::eLine, "line")
                (Type_e::eRoute, "route")
                (Type_e::eVehicleJourney, "vehicle_journey")
                (Type_e::eStopPoint, "stop_point")
                (Type_e::eStopArea, "stop_area")
                (Type_e::eStopTime, "stop_time")
                (Type_e::eNetwork, "network")
                (Type_e::eMode, "mode")
                (Type_e::eModeType, "mode_type")
                (Type_e::eCity, "city")
                (Type_e::eConnection, "connection")
                (Type_e::eRoutePoint, "route_point")
                (Type_e::eDistrict, "district")
                (Type_e::eDepartment, "department")
                (Type_e::eCompany, "company")
                (Type_e::eVehicle, "vehicle")
                (Type_e::eCountry, "country")
                (Type_e::eWay, "way")
                (Type_e::eCoord, "coord")
                (Type_e::eAddress, "address");

        instance = temp;

    }
    return instance;
}

std::string static_data::getListNameByType(Type_e type){
    return instance->types_string.left.at(type) + "_list";
}

Type_e static_data::typeByCaption(const std::string & type_str) {
    return instance->types_string.right.at(type_str);
}

std::string static_data::captionByType(Type_e type){
    return instance->types_string.left.at(type);
}

std::vector<idx_t> Country::get(Type_e type, const PT_Data &)  const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::eDistrict: return district_list; break;
    default: break;
    }
    return result;
}

std::vector<idx_t> City::get(Type_e type, const PT_Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::eStopArea: return stop_area_list; break;
    case Type_e::eDepartment: result.push_back(department_idx); break;
    default: break;
    }
    return result;
}

std::vector<idx_t> StopArea::get(Type_e type, const PT_Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::eStopPoint: return this->stop_point_list; break;
    case Type_e::eCity: result.push_back(city_idx); break;
    default: break;
    }
    return result;
}

std::vector<idx_t> Network::get(Type_e type, const PT_Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::eLine: return line_list; break;
    default: break;
    }
    return result;
}


std::vector<idx_t> Company::get(Type_e type, const PT_Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::eLine: return line_list; break;
    default: break;
    }
    return result;
}

std::vector<idx_t> ModeType::get(Type_e type, const PT_Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::eLine: return line_list; break;
    case Type_e::eMode: return mode_list; break;
    default: break;
    }
    return result;
}

std::vector<idx_t> Mode::get(Type_e type, const PT_Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::eModeType: result.push_back(mode_type_idx); break;
    default: break;
    }
    return result;
}

std::vector<idx_t> Line::get(Type_e type, const PT_Data & data) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::eModeType: result.push_back(mode_type_idx); break;
    case Type_e::eMode: return mode_list; break;
    case Type_e::eCompany: return company_list; break;
    case Type_e::eNetwork: result.push_back(network_idx); break;
    case Type_e::eRoute: for(const Route & route : data.routes) {
            if(route.line_idx == idx)
                result.push_back(route.idx);
        }
        break;
    default: break;
    }
    return result;
}


std::vector<idx_t> Route::get(Type_e type, const PT_Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::eLine: result.push_back(line_idx); break;
    case Type_e::eModeType: result.push_back(mode_type_idx); break;
    case Type_e::eRoutePoint: return route_point_list; break;
    case Type_e::eVehicleJourney: return vehicle_journey_list; break;
    default: break;
    }
    return result;
}


std::vector<idx_t> VehicleJourney::get(Type_e type, const PT_Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::eRoute: result.push_back(route_idx); break;
    case Type_e::eCompany: result.push_back(company_idx); break;
    case Type_e::eMode: result.push_back(mode_idx); break;
    case Type_e::eVehicle: result.push_back(vehicle_idx); break;
    case Type_e::eValidityPattern: result.push_back(validity_pattern_idx); break;
    default: break;
    }
    return result;
}

std::vector<idx_t> RoutePoint::get(Type_e type, const PT_Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::eRoute: result.push_back(route_idx); break;
    case Type_e::eStopPoint: result.push_back(stop_point_idx); break;
    default: break;
    }
    return result;
}

std::vector<idx_t> StopPoint::get(Type_e type, const PT_Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::eStopArea: result.push_back(stop_area_idx); break;
    case Type_e::eCity: result.push_back(city_idx); break;
    case Type_e::eMode: result.push_back(mode_idx); break;
    case Type_e::eNetwork: result.push_back(network_idx); break;
    case Type_e::eRoutePoint: return route_point_list; break;
    default: break;
    }
    return result;
}


EntryPoint::EntryPoint(const std::string &uri) : external_code(uri) {
       size_t pos = uri.find(":");
       if(pos == std::string::npos)
           type = Type_e::eUnknown;
       else {
           type = static_data::get()->typeByCaption(uri.substr(0,pos));
       }

       if(type == Type_e::eCoord){
           size_t pos2 = uri.find(":", pos+1);
           try{
               if(pos2 != std::string::npos) {
                   this->coordinates.set_lon(boost::lexical_cast<double>(uri.substr(pos+1, pos2 - pos - 1)));
                   this->coordinates.set_lat(boost::lexical_cast<double>(uri.substr(pos2+1)));
               }
           }catch(boost::bad_lexical_cast){
               this->coordinates.set_lon(0);
               this->coordinates.set_lat(0);
           }
       }
   }
}} //namespace navitia::type
