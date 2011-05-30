#include "types.h"
#include <iostream>
#include <boost/assign.hpp>
#include <boost/foreach.hpp>

using namespace navimake::types;

bool ValidityPattern::is_valid(int duration){
    if(duration < 0){
        std::cerr << "La date est avant le début de période" << std::endl;
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
        return this->name < other.name;
    }else{
        return *(this->line) < *(other.line);
    }
}



bool StopArea::operator<(const StopArea& other) const {
    //@TODO géré la gestion de la city
    return this->name < other.name;
}



bool StopPoint::operator<(const StopPoint& other) const {
    if(this->stop_area == other.stop_area){
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


bool Connection::operator<(const Connection& other) const{
    return *(this->departure_stop_point) < *(other.departure_stop_point);
}


nt::GeographicalCoord GeographicalCoord::transform()const {
    return nt::GeographicalCoord(x, y);
}

navitia::type::StopArea StopArea::Transformer::operator()(const StopArea& stop_area){
    navitia::type::StopArea sa;
    sa.id = stop_area.id;
    sa.idx = stop_area.idx;
    sa.external_code = stop_area.external_code;
    sa.coord = stop_area.coord.transform();
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
    nt_stop_point.coord = stop_point.coord.transform();
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
    nt_city.coord = city.coord.transform();
    nt_city.main_postal_code = city.main_postal_code;
    nt_city.use_main_stop_area_property = city.use_main_stop_area_property;
    nt_city.main_city = city.main_city;
    if(city.department != NULL)
        nt_city.department_idx = city.department->idx;

    
    return nt_city;
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
