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
        boost::gregorian::date current_date = beginning_date + boost::gregorian::days(i);
        if(active_days[current_date.day_of_week()]){
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
    return days[day];
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

navitia::type::StopArea* StopArea::get_navitia_type() const {
    navitia::type::StopArea* sa = new navitia::type::StopArea();
    sa->idx = this->idx;
    sa->uri = this->uri;
    sa->coord = this->coord;
    sa->comment = this->comment;
    sa->name = this->name;

    sa->set_properties(this->properties());
    return sa;
}

nt::Contributor* Contributor::get_navitia_type() const {
    nt::Contributor* nt_contributor = new nt::Contributor();
    nt_contributor->idx = this->idx;
    nt_contributor->uri = this->uri;
    nt_contributor->name = this->name;
    return nt_contributor;
}

nt::PhysicalMode* PhysicalMode::get_navitia_type() const {
    nt::PhysicalMode* nt_mode = new nt::PhysicalMode();
    nt_mode->idx = this->idx;
    nt_mode->uri = this->uri;
    nt_mode->name = this->name;
    return nt_mode;
}


nt::CommercialMode* CommercialMode::get_navitia_type() const {
    nt::CommercialMode* nt_commercial_mode = new nt::CommercialMode();
    nt_commercial_mode->idx = this->idx;
    nt_commercial_mode->uri = this->uri;
    nt_commercial_mode->name = this->name;
    return nt_commercial_mode;
}

nt::Company* Company::get_navitia_type() const {
    nt::Company* nt_company = new nt::Company();
    nt_company->idx = this->idx;
    nt_company->name = this->name;
    nt_company->uri = this->uri;
    nt_company->address_name = this->address_name;
    nt_company->address_number = this->address_number;
    nt_company->address_type_name = this->address_type_name;
    nt_company->phone_number = this->phone_number;
    nt_company->mail = this->mail;
    nt_company->website = this->website;
    nt_company->fax = this->fax;
    return nt_company;
}

nt::StopPoint* StopPoint::get_navitia_type() const {
    nt::StopPoint* nt_stop_point = new nt::StopPoint();
    nt_stop_point->idx = this->idx;
    nt_stop_point->uri = this->uri;
    nt_stop_point->name = this->name;
    nt_stop_point->coord = this->coord;
    nt_stop_point->fare_zone = this->fare_zone;

    if(this->stop_area != NULL)
        nt_stop_point->stop_area->idx = this->stop_area->idx;

    if(this->network != NULL)
        nt_stop_point->network->idx = this->network->idx;

    return nt_stop_point;
}


nt::Line* Line::get_navitia_type() const {
    navitia::type::Line* nt_line = new nt::Line();
    nt_line->idx = this->idx;
    nt_line->uri = this->uri;
    nt_line->name = this->name;
    nt_line->code = this->code;
    nt_line->color = this->color;
    nt_line->sort = this->sort;
    nt_line->backward_name = this->backward_name;
    nt_line->forward_name = this->forward_name;
    nt_line->additional_data = this->additional_data;

    if(this->commercial_mode != NULL)
        nt_line->commercial_mode->idx = this->commercial_mode->idx;

    if(this->network != NULL)
        nt_line->network->idx = this->network->idx;

    return nt_line;
}


nt::Route* Route::get_navitia_type() const {
    navitia::type::Route* nt_route = new nt::Route();
    nt_route->idx = this->idx;
    nt_route->uri = this->uri;
    nt_route->name = this->name;
    if(this->line != NULL)
        nt_route->line->idx = this->line->idx;

    return nt_route;
}

nt::Network* Network::get_navitia_type() const {
    nt::Network* nt_network = new nt::Network();
    nt_network->idx = this->idx;
    nt_network->uri = this->uri;
    nt_network->name = this->name;

    nt_network->address_name      = this->address_name;
    nt_network->address_number    = this->address_number;
    nt_network->address_type_name = this->address_type_name;
    nt_network->fax = this->fax;
    nt_network->phone_number = this->phone_number;
    nt_network->mail = this->mail;
    nt_network->website = this->website;
    return nt_network;
}

nt::JourneyPattern* JourneyPattern::get_navitia_type() const {
    nt::JourneyPattern* nt_journey_pattern = new nt::JourneyPattern();
    nt_journey_pattern->idx = this->idx;
    nt_journey_pattern->uri = this->uri;
    nt_journey_pattern->name = this->name;
    nt_journey_pattern->is_frequence = this->is_frequence;

    if(this->route != NULL)
        nt_journey_pattern->route->idx = this->route->idx;

    if(this->physical_mode != NULL)
    {
        nt_journey_pattern->commercial_mode->idx = this->physical_mode->idx;
        nt_journey_pattern->physical_mode->idx = this->physical_mode->idx;
    }

    return nt_journey_pattern;
}

nt::StopTime* StopTime::get_navitia_type() const {
    nt::StopTime* nt_stop = new nt::StopTime();
    nt_stop->arrival_time = this->arrival_time;
    nt_stop->departure_time = this->departure_time;
    nt_stop->properties[nt::StopTime::ODT] = this->ODT;
    nt_stop->properties[nt::StopTime::DROP_OFF] = this->drop_off_allowed;
    nt_stop->properties[nt::StopTime::PICK_UP] = this->pick_up_allowed;
    nt_stop->properties[nt::StopTime::IS_FREQUENCY] = this->is_frequency;
    nt_stop->properties[nt::StopTime::WHEELCHAIR_BOARDING] = this->wheelchair_boarding;

    nt_stop->local_traffic_zone = this->local_traffic_zone;

    //@TODO bah y a plus qu'a...
    //nt_stop->journey_pattern_point = this->journey_pattern_point->idx;
    //nt_stop->vehicle_journey_idx = this->vehicle_journey->idx;
    return nt_stop;

}

nt::StopPointConnection* StopPointConnection::get_navitia_type() const {
    nt::StopPointConnection* nt_connection = new nt::StopPointConnection();
    nt_connection->idx = this->idx;
    nt_connection->uri = this->uri;
    nt_connection->departure->idx = this->departure->idx;
    nt_connection->destination->idx = this->destination->idx;
    nt_connection->duration = this->duration;
    nt_connection->max_duration = this->max_duration;
    nt_connection->set_properties(this->properties());
    return nt_connection;
}

nt::JourneyPatternPoint* JourneyPatternPoint::get_navitia_type() const {
    nt::JourneyPatternPoint* nt_journey_pattern_point = new nt::JourneyPatternPoint();
    nt_journey_pattern_point->idx = this->idx;
    nt_journey_pattern_point->uri = this->uri;
    nt_journey_pattern_point->order = this->order;

    nt_journey_pattern_point->stop_point->idx = this->stop_point->idx;
    nt_journey_pattern_point->journey_pattern->idx = this->journey_pattern->idx;
    return nt_journey_pattern_point;
}

nt::VehicleJourney* VehicleJourney::get_navitia_type() const {
    nt::VehicleJourney* nt_vj = new nt::VehicleJourney();
    nt_vj->idx = this->idx;
    nt_vj->name = this->name;
    nt_vj->uri = this->uri;
    nt_vj->comment = this->comment;
    nt_vj->start_time = this->start_time;
    nt_vj->end_time = this->end_time;
    nt_vj->headway_secs = this->headway_secs;

    if(this->company != NULL)
        nt_vj->company->idx = this->company->idx;

    nt_vj->journey_pattern->idx = this->journey_pattern->idx;

    if(this->validity_pattern != NULL)
        nt_vj->validity_pattern->idx = this->validity_pattern->idx;

    nt_vj->set_vehicles(this->vehicles());

    nt_vj->is_adapted = this->is_adapted;

    if(this->adapted_validity_pattern != NULL)
        nt_vj->adapted_validity_pattern->idx = this->adapted_validity_pattern->idx;

    if(this->theoric_vehicle_journey != NULL){
        nt_vj->theoric_vehicle_journey->idx = this->theoric_vehicle_journey->idx;
    }

    /*for(auto* avj : this->adapted_vehicle_journey_list){
        nt_vj->adapted_vehicle_journey_list.push_back(avj);
    }*/

    return nt_vj;
}

nt::Calendar* Calendar::get_navitia_type() const{
    nt::Calendar* nt_cal = new nt::Calendar();
    nt_cal->comment = this->comment;
    nt_cal->idx = this->idx;
    nt_cal->name = this->name;

    return nt_cal;
}

nt::ValidityPattern* ValidityPattern::get_navitia_type() const {
    nt::ValidityPattern* nt_vp = new nt::ValidityPattern();

    nt_vp->idx = this->idx;
    nt_vp->uri = this->uri;
    nt_vp->beginning_date = this->beginning_date;

    for(int i=0;i< 366;++i)
        if(this->days[i])
            nt_vp->add(i);
        else
            nt_vp->remove(i);

    return nt_vp;
}
