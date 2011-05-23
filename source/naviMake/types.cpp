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
