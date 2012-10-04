#include "types.h"
#include <iostream>
#include <boost/assign.hpp>
#include <boost/foreach.hpp>

using namespace navimake::types;

bool ValidityPattern::is_valid(int duration){
    if(duration < 0){
        std::cerr << "La date est avant le début de période " << beginning_date << " " << duration <<  std::endl;
        return false;
    }
    else if(duration > 366){
        std::cerr << "La date dépasse la fin de période" << std::endl;
        return false;
    }
    return true;
}

void ValidityPattern::add(boost::gregorian::date day){
    long duration = (day - beginning_date).days();
    add(duration);
}

void ValidityPattern::add(int duration){
    if(is_valid(duration))
        days[duration] = true;
}



void ValidityPattern::remove(boost::gregorian::date date){
    long duration = (date - beginning_date).days();
    remove(duration);
}

void ValidityPattern::remove(int day){
    if(is_valid(day))
        days[day] = false;
}

bool ValidityPattern::check(int day) const {
    return days[day];
}

bool District::operator<(const District& other) const {
    if(this->country == other.country || this->country == NULL){
        return this->name < other.name;
    }else{
        return *this->country < *other.country;
    }
}


bool ModeType::operator<(const ModeType& other) const {
    return this->name < other.name;
}

bool Mode::operator<(const Mode& other) const {
    if(this->mode_type == other.mode_type || this->mode_type == NULL){
        return this->name < other.name;
    }else{
        return *(this->mode_type) < *(other.mode_type);
    }
}

bool Line::operator<(const Line& other) const {
    if(this->mode_type == NULL && other.mode_type != NULL){
        return true;
    }else if(other.mode_type == NULL && this->mode_type != NULL){
        return false;
    }
    if(this->mode_type == other.mode_type){
        return this->name < other.name;
    }else{
        return *(this->mode_type) < *(other.mode_type);
    }
}



bool Route::operator<(const Route& other) const {
    if(this->line == other.line){
        return this->external_code <  other.external_code;
    }else{
        return *(this->line) < *(other.line);
    }
}

bool RoutePoint::operator<(const RoutePoint& other) const {
    if(this->route == other.route){
        return this->order < other.order;
    }else{
        return *(this->route) < *(other.route);
    }

}


bool StopArea::operator<(const StopArea& other) const {
    //@TODO géré la gestion de la city
    return this->name < other.name;
}



bool StopPoint::operator<(const StopPoint& other) const {
    if(!this->stop_area)
        return false;
    else if(!other.stop_area)
        return true;
    else if(this->stop_area == other.stop_area){
        return this->name < other.name;
    }else{
        return *(this->stop_area) < *(other.stop_area);
    }
}


bool VehicleJourney::operator<(const VehicleJourney& other) const {
    if(this->route == other.route){
        return this->name < other.name;
    }else{
        return *(this->route) < *(other.route);
    }
}

bool ValidityPattern::operator <(const ValidityPattern &other) const {
    int a, b;
    a = 0;
    b = 0;
    for(int i=0; i<7; ++i) {
        a += pow(this->days[i], i);
        b += pow(other.check(i), i);
    }

    return a < b;
}

bool Connection::operator<(const Connection& other) const{
    return *(this->departure_stop_point) < *(other.departure_stop_point);
}
bool StopTime::operator<(const StopTime& other) const {

    return this->route_point->route < other.route_point->route
            || ((this->route_point->route == other.route_point->route) && (this->vehicle_journey< other.vehicle_journey
            || ((this->vehicle_journey == other.vehicle_journey) && (this->order< other.order))));
}



bool Department::operator <(const Department & other) const {
    return this->district < other.district || ((this->district == other.district) && (this->name < other.name));
}

navitia::type::StopArea StopArea::Transformer::operator()(const StopArea& stop_area){
    navitia::type::StopArea sa;
    sa.id = stop_area.id;
    sa.idx = stop_area.idx;
    sa.external_code = stop_area.external_code;
    sa.coord = stop_area.coord;
    sa.comment = stop_area.comment;
    sa.name = stop_area.name;

    sa.additional_data = stop_area.additional_data;
    sa.properties = stop_area.properties;
    return sa;
}


nt::Mode Mode::Transformer::operator()(const Mode& mode){
    nt::Mode nt_mode;
    nt_mode.id = mode.id;
    nt_mode.idx = mode.idx;
    nt_mode.external_code = mode.external_code;
    nt_mode.name = mode.name;
    nt_mode.mode_type_idx = mode.mode_type->idx;
    return nt_mode;

}


nt::ModeType ModeType::Transformer::operator()(const ModeType& mode_type){
    nt::ModeType nt_mode_type;
    nt_mode_type.id = mode_type.id;
    nt_mode_type.idx = mode_type.idx;
    nt_mode_type.external_code = mode_type.external_code;
    nt_mode_type.name = mode_type.name;
    return nt_mode_type;
}

nt::StopPoint StopPoint::Transformer::operator()(const StopPoint& stop_point){
    nt::StopPoint nt_stop_point;
    nt_stop_point.id = stop_point.id;
    nt_stop_point.idx = stop_point.idx;
    nt_stop_point.external_code = stop_point.external_code;
    nt_stop_point.name = stop_point.name;
    nt_stop_point.coord = stop_point.coord;
    nt_stop_point.fare_zone = stop_point.fare_zone;

    nt_stop_point.address_name      = stop_point.address_name;
    nt_stop_point.address_number    = stop_point.address_number;       
    nt_stop_point.address_type_name = stop_point.address_type_name;
    
    if(stop_point.stop_area != NULL)
        nt_stop_point.stop_area_idx = stop_point.stop_area->idx;


    if(stop_point.mode != NULL)
        nt_stop_point.mode_idx = stop_point.mode->idx;

    if(stop_point.city != NULL)
        nt_stop_point.city_idx = stop_point.city->idx;

    return nt_stop_point;

}


nt::Line Line::Transformer::operator()(const Line& line){
    navitia::type::Line nt_line;
    nt_line.id = line.id;
    nt_line.idx = line.idx;
    nt_line.external_code = line.external_code;
    nt_line.name = line.name;
    nt_line.code = line.code;
    nt_line.color = line.color;
    nt_line.sort = line.sort;
    nt_line.backward_name = line.backward_name;
    nt_line.forward_name = line.forward_name;
    nt_line.additional_data = line.additional_data;

    if(line.mode_type != NULL)
        nt_line.mode_type_idx = line.mode_type->idx;

    if(line.network != NULL)
        nt_line.network_idx = line.network->idx;

    if(line.backward_direction != NULL)
        nt_line.backward_direction_idx = line.backward_direction->idx;

    if(line.forward_direction != NULL)
        nt_line.forward_direction_idx = line.forward_direction->idx;

    
    return nt_line;
}

nt::City City::Transformer::operator()(const City& city){
    navitia::type::City nt_city;
    nt_city.id = city.id;
    nt_city.idx = city.idx;
    nt_city.external_code = city.external_code;
    nt_city.name = city.name;
    nt_city.comment = city.comment;
    nt_city.coord = city.coord;
    nt_city.main_postal_code = city.main_postal_code;
    nt_city.use_main_stop_area_property = city.use_main_stop_area_property;
    nt_city.main_city = city.main_city;
    if(city.department != NULL)
        nt_city.department_idx = city.department->idx;
    else
        nt_city.department_idx = nt::invalid_idx;
    
    return nt_city;
}

nt::Department Department::Transformer::operator()(const Department& department){
    navitia::type::Department nt_department;
    nt_department.id = department.id;
    nt_department.idx = department.idx;
    nt_department.external_code = department.external_code;
    nt_department.name = department.name;

    if(department.district != NULL)
        nt_department.district_idx = department.district->idx;
    else
        nt_department.district_idx = nt::invalid_idx;

    return nt_department;
}


nt::District District::Transformer::operator()(const District& district){
    navitia::type::District nt_district;
    nt_district.id = district.id;
    nt_district.idx = district.idx;
    nt_district.external_code = district.external_code;
    nt_district.name = district.name;

    return nt_district;
}

nt::Network Network::Transformer::operator()(const Network& network){
    nt::Network nt_network;
    nt_network.id = network.id;
    nt_network.idx = network.idx;
    nt_network.external_code = network.external_code;
    nt_network.name = network.name;

    nt_network.address_name      = network.address_name;
    nt_network.address_number    = network.address_number;       
    nt_network.address_type_name = network.address_type_name;
    nt_network.fax = network.fax;
    nt_network.phone_number = network.phone_number;
    nt_network.mail = network.mail;
    nt_network.website = network.website;
    return nt_network;
}

nt::Route Route::Transformer::operator()(const Route& route){
    nt::Route nt_route;
    nt_route.id = route.id;
    nt_route.idx = route.idx;
    nt_route.external_code = route.external_code;
    nt_route.name = route.name;
    nt_route.is_frequence = route.is_frequence;
    nt_route.is_forward = route.is_forward;
    nt_route.is_adapted = route.is_adapted;
    
    if(route.line != NULL)
        nt_route.line_idx = route.line->idx;

    if(route.mode != NULL && route.mode->mode_type != NULL)
        nt_route.mode_type_idx = route.mode->mode_type->idx;

    return nt_route;
}

nt::StopTime StopTime::Transformer::operator()(const StopTime& stop){
    nt::StopTime nt_stop;
    nt_stop.idx = stop.idx;
    nt_stop.arrival_time = stop.arrival_time;
    nt_stop.departure_time = stop.departure_time;
    nt_stop.order = stop.order;
    nt_stop.zone = stop.zone;
    nt_stop.properties[nt::StopTime::ODT] = stop.ODT;
    nt_stop.properties[nt::StopTime::DROP_OFF] = stop.drop_off_allowed;
    nt_stop.properties[nt::StopTime::PICK_UP] = stop.pick_up_allowed;

    nt_stop.route_point_idx = stop.route_point->idx;
    nt_stop.vehicle_journey_idx = stop.vehicle_journey->idx;
    return nt_stop;

}

nt::Connection Connection::Transformer::operator()(const Connection& connection){
    nt::Connection nt_connection;
    nt_connection.id = connection.id;
    nt_connection.idx = connection.idx;
    nt_connection.external_code = connection.external_code;
    nt_connection.departure_stop_point_idx = connection.departure_stop_point->idx;
    nt_connection.destination_stop_point_idx = connection.destination_stop_point->idx;
    nt_connection.duration = connection.duration;
    nt_connection.max_duration = connection.max_duration;
    return nt_connection;
}

nt::RoutePoint RoutePoint::Transformer::operator()(const RoutePoint& route_point){
    nt::RoutePoint nt_route_point;
    nt_route_point.id = route_point.id;
    nt_route_point.idx = route_point.idx;
    nt_route_point.external_code = route_point.external_code;
    nt_route_point.order = route_point.order;
    nt_route_point.main_stop_point = route_point.main_stop_point;
    nt_route_point.fare_section = route_point.fare_section;
    
    nt_route_point.stop_point_idx = route_point.stop_point->idx;
    nt_route_point.route_idx = route_point.route->idx;
    return nt_route_point;
}

nt::VehicleJourney VehicleJourney::Transformer::operator()(const VehicleJourney& vj){
    nt::VehicleJourney nt_vj;
    nt_vj.id = vj.id;
    nt_vj.idx = vj.idx;
    nt_vj.name = vj.name;
    nt_vj.external_code = vj.external_code;
    nt_vj.comment = vj.comment;
    nt_vj.is_adapted = vj.is_adapted;

    if(vj.company != NULL)
        nt_vj.company_idx = vj.company->idx;

    if(vj.mode != NULL)
        nt_vj.mode_idx = vj.mode->idx;

    nt_vj.route_idx = vj.route->idx;

    if(vj.validity_pattern != NULL)
        nt_vj.validity_pattern_idx = vj.validity_pattern->idx;

    return nt_vj;
}
nt::ValidityPattern ValidityPattern::Transformer::operator()(const ValidityPattern& vp){
    nt::ValidityPattern nt_vp;

    nt_vp.id = vp.id;
    nt_vp.idx = vp.idx;
    nt_vp.external_code = vp.external_code;
    nt_vp.beginning_date = vp.beginning_date;

    for(int i=0;i< 366;++i)
        if(vp.days[i])
            nt_vp.add(i);
        else
            nt_vp.remove(i);

    return nt_vp;
}
