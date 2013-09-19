#include "type.h"
#include "pt_data.h"
#include <iostream>
#include <boost/assign.hpp>
#include "utils/functions.h"

namespace navitia { namespace type {

std::string VehicleJourney::get_direction() const {
    std::string to_return;
    try {
        if ((this->journey_pattern != nullptr) && (!this->journey_pattern->journey_pattern_point_list.empty()))
            to_return = this->journey_pattern->journey_pattern_point_list.back()->stop_point->name;
    }catch(...){

    }
    return to_return;
}

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
    double length_sqr = dlon * dlon + dlat * dlat;
    double u;

    // On gère le cas où le segment est particulièrement court, et donc ça peut poser des problèmes (à cause de la division par length²)
    if(length_sqr < 1e-11){ // moins de un mètre, on projette sur une extrémité
        if(this->distance_to(segment_start) < this->distance_to(segment_end))
            u = 0;
        else
            u = 1;
    } else {
        u = ((this->_lon - segment_start._lon)*dlon + (this->_lat - segment_start._lat)*dlat )/
                length_sqr;
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
                (Type_e::ValidityPattern, "validity_pattern")
                (Type_e::Line, "line")
                (Type_e::JourneyPattern, "journey_pattern")
                (Type_e::VehicleJourney, "vehicle_journey")
                (Type_e::StopPoint, "stop_point")
                (Type_e::StopArea, "stop_area")
                (Type_e::Network, "network")
                (Type_e::PhysicalMode, "physical_mode")
                (Type_e::CommercialMode, "commercial_mode")
                (Type_e::Connection, "connection")
                (Type_e::JourneyPatternPoint, "journey_pattern_point")
                (Type_e::Company, "company")
                (Type_e::Way, "way")
                (Type_e::Coord, "coord")
                (Type_e::Address, "address")
                (Type_e::Route, "route")
                (Type_e::POI, "poi")
                (Type_e::POIType, "poi_type");

        boost::assign::insert(temp->modes_string)
                (Mode_e::Walking, "walking")
                (Mode_e::Bike, "bike")
                (Mode_e::Car, "car")
                (Mode_e::Vls, "vls");
        instance = temp;

    }
    return instance;
}

Type_e static_data::typeByCaption(const std::string & type_str) {
    return instance->types_string.right.at(type_str);
}

std::string static_data::captionByType(Type_e type){
    return instance->types_string.left.at(type);
}

Mode_e static_data::modeByCaption(const std::string & mode_str) {
    return instance->modes_string.right.at(mode_str);
}


template<typename T> std::vector<idx_t> indexes(std::vector<T*> elements){
    std::vector<idx_t> result;
    for(T* element : elements){
        result.push_back(element->idx);
    }
    return result;
}

std::vector<idx_t> StopArea::get(Type_e type, const PT_Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::StopPoint: return indexes(this->stop_point_list); break;
    default: break;
    }
    return result;
}

std::vector<idx_t> Network::get(Type_e type, const PT_Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::Line: return indexes(line_list); break;
    default: break;
    }
    return result;
}


std::vector<idx_t> Company::get(Type_e type, const PT_Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::Line: return indexes(line_list); break;
    default: break;
    }
    return result;
}

std::vector<idx_t> CommercialMode::get(Type_e type, const PT_Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::Line: return indexes(line_list); break;
    default: break;
    }
    return result;
}

std::vector<idx_t> PhysicalMode::get(Type_e type, const PT_Data & data) const {
    std::vector<idx_t> result;
    switch(type) {
        case Type_e::VehicleJourney:
            for(auto vj : data.vehicle_journeys) {
                if(vj->journey_pattern->physical_mode == this)
                    result.push_back(vj->idx);
            }
            break;
    default: break;
    }
    return result;
}

std::vector<idx_t> Line::get(Type_e type, const PT_Data&) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::CommercialMode: result.push_back(commercial_mode->idx); break;
    case Type_e::Company: return indexes(company_list); break;
    case Type_e::Network: result.push_back(network->idx); break;
    case Type_e::Route: return indexes(route_list); break;
    default: break;
    }
    return result;
}

std::vector<idx_t> Route::get(Type_e type, const PT_Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::Line: result.push_back(line->idx); break;
    case Type_e::JourneyPattern: return indexes(journey_pattern_list); break;
    default: break;
    }
    return result;
}

std::vector<idx_t> JourneyPattern::get(Type_e type, const PT_Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::Route: result.push_back(route->idx); break;
    case Type_e::CommercialMode: result.push_back(commercial_mode->idx); break;
    case Type_e::JourneyPatternPoint: return indexes(journey_pattern_point_list); break;
    case Type_e::VehicleJourney: return indexes(vehicle_journey_list); break;
    default: break;
    }
    return result;
}


std::vector<idx_t> VehicleJourney::get(Type_e type, const PT_Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::JourneyPattern: result.push_back(journey_pattern->idx); break;
    case Type_e::Company: result.push_back(company->idx); break;
    case Type_e::PhysicalMode: result.push_back(journey_pattern->physical_mode->idx); break;
    case Type_e::ValidityPattern: result.push_back(validity_pattern->idx); break;
    default: break;
    }
    return result;
}

std::vector<idx_t> JourneyPatternPoint::get(Type_e type, const PT_Data &) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::JourneyPattern: result.push_back(journey_pattern->idx); break;
    case Type_e::StopPoint: result.push_back(stop_point->idx); break;
    default: break;
    }
    return result;
}

std::vector<idx_t> StopPoint::get(Type_e type, const PT_Data & data) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::StopArea: result.push_back(stop_area->idx); break;
    case Type_e::JourneyPatternPoint: return indexes(journey_pattern_point_list); break;
    case Type_e::Connection:
        for(const StopPointConnection* conn : data.stop_point_connections) {
            if(conn->departure->idx == this->idx) {
                result.push_back(conn->idx);
            }
        }
            break;
    default: break;
    }
    return result;
}

template<>
std::vector<idx_t> Connection<StopPoint>::get(Type_e type, const PT_Data & ) const {
    std::vector<idx_t> result;
    switch(type) {
    case Type_e::StopPoint:
        result.push_back(this->departure->idx);
        result.push_back(this->destination->idx);
        break;
    default: break;
    }
    return result;
}
template<>
bool Connection<JourneyPatternPoint>::operator<(const Connection<JourneyPatternPoint> & other) const { return this < &other; }
template<>
bool Connection<StopPoint>::operator<(const Connection<StopPoint> & other) const { return this < &other; }

EntryPoint::EntryPoint(const Type_e type, const std::string &uri) : type(type), uri(uri) {
   // Gestion des adresses
   if (type == Type_e::Address){
       std::vector<std::string> vect;
       vect = split_string(uri, ":");
       if(vect.size() == 3){
           this->uri = vect[0] + ":" + vect[1];
           this->house_number = str_to_int(vect[2]);
       }
   }
   if(type == Type_e::Coord){
       size_t pos2 = uri.find(":", 6);
       try{
           if(pos2 != std::string::npos) {
               this->coordinates.set_lon(boost::lexical_cast<double>(uri.substr(6, pos2 - 6)));
               this->coordinates.set_lat(boost::lexical_cast<double>(uri.substr(pos2+1)));
           }
       }catch(boost::bad_lexical_cast){
           this->coordinates.set_lon(0);
           this->coordinates.set_lat(0);
       }
   }
}

void StreetNetworkParams::set_filter(const std::string &param_uri){
    size_t pos = param_uri.find(":");
    if(pos == std::string::npos)
        type_filter = Type_e::Unknown;
    else {
        uri_filter = param_uri;
        type_filter = static_data::get()->typeByCaption(param_uri.substr(0,pos));
    }
}

}} //namespace navitia::type
