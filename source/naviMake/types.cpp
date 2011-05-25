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
    nt::Mode mode_nt;
    mode_nt.id = mode.id;
    mode_nt.idx = mode.idx;
    mode_nt.external_code = mode.external_code;
    mode_nt.name = mode.name;
    mode_nt.mode_type_idx = mode.mode_type->idx;
    return mode_nt;

}


nt::ModeType ModeType::Transformer::operator()(const ModeType& mode_type){
    nt::ModeType mode_type_nt;
    mode_type_nt.id = mode_type.id;
    mode_type_nt.idx = mode_type.idx;
    mode_type_nt.external_code = mode_type.external_code;
    mode_type_nt.name = mode_type.name;
    return mode_type_nt;

}

nt::StopPoint StopPoint::Transformer::operator()(const StopPoint& stop_point){
    nt::StopPoint stop_point_nt;
    stop_point_nt.id = stop_point.id;
    stop_point_nt.idx = stop_point.idx;
    stop_point_nt.external_code = stop_point.external_code;
    stop_point_nt.name = stop_point.name;
    stop_point_nt.coord = stop_point.coord.transform();
    stop_point_nt.fare_zone = stop_point.fare_zone;

    stop_point_nt.address_name      = stop_point.address_name;
    stop_point_nt.address_number    = stop_point.address_number;       
    stop_point_nt.address_type_name = stop_point.address_type_name;
    
    if(stop_point.stop_area != NULL)
        stop_point_nt.stop_area_idx = stop_point.stop_area->idx;

    if(stop_point.mode != NULL)
        stop_point_nt.mode_idx = stop_point.mode->idx;

    if(stop_point.city != NULL)
        stop_point_nt.city_idx = stop_point.city->idx;

    return stop_point_nt;

}
