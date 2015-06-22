/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
This file is part of Navitia,
    the software to build cool stuff with public transport.
 
Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!
  
LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
   
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.
   
You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
  
Stay tuned using
twitter @navitia 
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "types.h"

using namespace ed::types;

bool ValidityPattern::is_valid(int duration){
    if(duration < 0){
        return false;
    }
    else if(duration > 366){
        return false;
    }
    return true;
}

bool ValidityPattern::operator==(const ValidityPattern& other)const{
    return ((this->beginning_date == other.beginning_date) && (this->days == other.days));
}

void ValidityPattern::add(boost::gregorian::date day){
    long duration = (day - beginning_date).days();
    if(is_valid(duration))
        add(duration);
}

void ValidityPattern::add(int duration){
    if(is_valid(duration))
        days[duration] = true;
}

void ValidityPattern::add(boost::gregorian::date start, boost::gregorian::date end, std::bitset<7> active_days){
    for(long i=0; i < (end - start).days(); ++i){
        boost::gregorian::date current_date = start + boost::gregorian::days(i);
        if(active_days[(6 + current_date.day_of_week()) % 7]){
            add(current_date);
        }else{
            remove(current_date);
        }
    };
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
    assert(day >= 0);
    return days[day];
}

int ValidityPattern::slide(boost::gregorian::date day) const {
    return (day - beginning_date).days();
}

bool ValidityPattern::check(boost::gregorian::date day) const {
    long duration = slide(day);
    return check(duration);
}

bool CommercialMode::operator<(const CommercialMode& other) const {
    return this->name < other.name || (this->name == other.name && this < &other);
}

bool PhysicalMode::operator<(const PhysicalMode& other) const {
    return this->name < other.name || (this->name == other.name && this < &other);
}

bool Line::operator<(const Line& other) const {
    if(this->commercial_mode == NULL && other.commercial_mode != NULL){
        return true;
    }else if(other.commercial_mode == NULL && this->commercial_mode != NULL){
        return false;
    }
    if(this->commercial_mode == other.commercial_mode){
        BOOST_ASSERT(this->uri != other.uri);
        return this->uri < other.uri;
    }else{
        return *(this->commercial_mode) < *(other.commercial_mode);
    }
}

bool LineGroup::operator<(const LineGroup& other) const {
    if (this->name != other.name) {
        return this->name < other.name;
    }
    return this->idx < other.idx;
}

bool Route::operator<(const Route& other) const {
    if(this->line == other.line){
        BOOST_ASSERT(this->uri != other.uri);
        return this->uri <  other.uri;
    }else{
        return *(this->line) < *(other.line);
    }
}

bool JourneyPattern::operator<(const JourneyPattern& other) const {
    if(this->route == other.route){
        BOOST_ASSERT(this->uri != other.uri);
        return this->uri <  other.uri;
    }else{
        return *(this->route) < *(other.route);
    }
}

bool JourneyPatternPoint::operator<(const JourneyPatternPoint& other) const {
    if(this->journey_pattern == other.journey_pattern){
        BOOST_ASSERT(this->order != other.order);
        return this->order < other.order;
    }else{
        return *(this->journey_pattern) < *(other.journey_pattern);
    }
}

Calendar::Calendar(boost::gregorian::date beginning_date) : validity_pattern(beginning_date) {}

void Calendar::build_validity_pattern(boost::gregorian::date_period production_period) {
    //initialisation of the validity pattern from the active periods and the exceptions
    for (boost::gregorian::date_period period : this->period_list) {
        auto intersection_period = production_period.intersection(period);
        if (intersection_period.is_null()) {
            continue;
        }
        validity_pattern.add(intersection_period.begin(), intersection_period.end(), week_pattern);
    }

    for (navitia::type::ExceptionDate exd : this->exceptions) {
        if (!production_period.contains(exd.date)) {
            continue;
        }
        if (exd.type == navitia::type::ExceptionDate::ExceptionType::sub) {
            validity_pattern.remove(exd.date);
        } else if (exd.type == navitia::type::ExceptionDate::ExceptionType::add) {
            validity_pattern.add(exd.date);
        }
    }
}

bool StopArea::operator<(const StopArea& other) const {
    BOOST_ASSERT(this->uri != other.uri);
    //@TODO géré la gestion de la city
    return this->uri < other.uri;
}



bool StopPoint::operator<(const StopPoint& other) const {
    if(!this->stop_area)
        return false;
    else if(!other.stop_area)
        return true;
    else if(this->stop_area == other.stop_area){
        BOOST_ASSERT(this->uri != other.uri);
        return this->uri < other.uri;
    }else{
        return *(this->stop_area) < *(other.stop_area);
    }
}


bool VehicleJourney::operator<(const VehicleJourney& other) const {
    if(this->journey_pattern == other.journey_pattern){
        // On compare les pointeurs pour avoir un ordre total (fonctionnellement osef du tri, mais techniquement c'est important)
        return this->stop_time_list.front() < other.stop_time_list.front();
    }else{
        return *(this->journey_pattern) < *(other.journey_pattern);
    }
}

bool ValidityPattern::operator <(const ValidityPattern &other) const {
    //Il faut retablir l'insertion unique des pointeurs dans les connecteurs
    //BOOST_ASSERT(this->days.to_string() != other.days.to_string());
    //return this->days.to_string() < other.days.to_string();
    return this < &other;
}

bool StopPointConnection::operator<(const StopPointConnection& other) const{
    if (this->departure == other.departure) {
        if(this->destination == other.destination) {
            return this < &other;
        } else {
            return *(this->destination) < *(other.destination);
        }
    } else {
        return *(this->departure) < *(other.departure);
    }
}

bool StopTime::operator<(const StopTime& other) const {
    if(this->vehicle_journey == other.vehicle_journey){
        BOOST_ASSERT(this->journey_pattern_point->order != other.journey_pattern_point->order);
        return this->journey_pattern_point->order < other.journey_pattern_point->order;
    } else {
        return *(this->vehicle_journey) < *(other.vehicle_journey);
    }
}
