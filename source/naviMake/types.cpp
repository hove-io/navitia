#include "types.h"

using namespace navimake::types;

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

bool District::operator<(const District& other) const {
    if(this->country == other.country || this->country == NULL){
        return this->name < other.name;
    }else{
        return *this->country < *other.country;
    }
}


bool CommercialMode::operator<(const CommercialMode& other) const {
    return this->name < other.name;
}

bool PhysicalMode::operator<(const PhysicalMode& other) const {
    return this->name < other.name;
}

bool Line::operator<(const Line& other) const {
    if(this->commercial_mode == NULL && other.commercial_mode != NULL){
        return true;
    }else if(other.commercial_mode == NULL && this->commercial_mode != NULL){
        return false;
    }
    if(this->commercial_mode == other.commercial_mode){
        return this->uri < other.uri;
    }else{
        return *(this->commercial_mode) < *(other.commercial_mode);
    }
}

bool Route::operator<(const Route& other) const {
    if(this->line == other.line){
        return this->uri <  other.uri;
    }else{
        return *(this->line) < *(other.line);
    }
}

bool JourneyPattern::operator<(const JourneyPattern& other) const {
    if(this->route == other.route){
        return this->uri <  other.uri;
    }else{
        return *(this->route) < *(other.route);
    }
}

bool JourneyPatternPoint::operator<(const JourneyPatternPoint& other) const {
    if(this->journey_pattern == other.journey_pattern){
        return this->order < other.order;
    }else{
        return *(this->journey_pattern) < *(other.journey_pattern);
    }

}


bool StopArea::operator<(const StopArea& other) const {
    //@TODO géré la gestion de la city
    return this->uri < other.uri;
}



bool StopPoint::operator<(const StopPoint& other) const {
    if(!this->stop_area)
        return false;
    else if(!other.stop_area)
        return true;
    else if(this->stop_area == other.stop_area){
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
    return this->days.to_string() < other.days.to_string();
}

bool Connection::operator<(const Connection& other) const{
    return *(this->departure_stop_point) < *(other.departure_stop_point);
}

bool JourneyPatternPointConnection::operator<(const JourneyPatternPointConnection& other) const {
    return *(this->departure_journey_pattern_point) < *(other.departure_journey_pattern_point);
}
bool StopTime::operator<(const StopTime& other) const {
    if(this->vehicle_journey == other.vehicle_journey){
        return this->journey_pattern_point->order < other.journey_pattern_point->order;
    } else {
        return *this->vehicle_journey < *other.vehicle_journey;
    }
}



bool Department::operator <(const Department & other) const {
    return this->district < other.district || ((this->district == other.district) && (this->name < other.name));
}

navitia::type::StopArea StopArea::get_navitia_type() const {
    navitia::type::StopArea sa;
    sa.id = this->id;
    sa.idx = this->idx;
    sa.uri = this->uri;
    sa.coord = this->coord;
    sa.comment = this->comment;
    sa.name = this->name;
    sa.wheelchair_boarding = this->wheelchair_boarding;

    sa.additional_data = this->additional_data;
    sa.properties = this->properties;
    return sa;
}


nt::PhysicalMode PhysicalMode::get_navitia_type() const {
    nt::PhysicalMode nt_mode;
    nt_mode.id = this->id;
    nt_mode.idx = this->idx;
    nt_mode.uri = this->uri;
    nt_mode.name = this->name;
    return nt_mode;
}


nt::CommercialMode CommercialMode::get_navitia_type() const {
    nt::CommercialMode nt_commercial_mode;
    nt_commercial_mode.id = this->id;
    nt_commercial_mode.idx = this->idx;
    nt_commercial_mode.uri = this->uri;
    nt_commercial_mode.name = this->name;
    return nt_commercial_mode;
}

nt::Company Company::get_navitia_type() const {
    nt::Company nt_company;
    nt_company.id = this->id;
    nt_company.idx = this->idx;
    nt_company.name = this->name;
    nt_company.uri = this->uri;
    nt_company.address_name = this->address_name;
    nt_company.address_number = this->address_number;
    nt_company.address_type_name = this->address_type_name;
    nt_company.phone_number = this->phone_number;
    nt_company.mail = this->mail;
    nt_company.website = this->website;
    nt_company.fax = this->fax;
    return nt_company;
}

nt::StopPoint StopPoint::get_navitia_type() const {
    nt::StopPoint nt_stop_point;
    nt_stop_point.id = this->id;
    nt_stop_point.idx = this->idx;
    nt_stop_point.uri = this->uri;
    nt_stop_point.name = this->name;
    nt_stop_point.coord = this->coord;
    nt_stop_point.fare_zone = this->fare_zone;

    nt_stop_point.address_name      = this->address_name;
    nt_stop_point.address_number    = this->address_number;
    nt_stop_point.address_type_name = this->address_type_name;
    
    nt_stop_point.wheelchair_boarding = this->wheelchair_boarding;
    if(this->stop_area != NULL)
        nt_stop_point.stop_area_idx = this->stop_area->idx;

    if(this->network != NULL)
        nt_stop_point.network_idx = this->network->idx;

    return nt_stop_point;
}


nt::Line Line::get_navitia_type() const {
    navitia::type::Line nt_line;
    nt_line.id = this->id;
    nt_line.idx = this->idx;
    nt_line.uri = this->uri;
    nt_line.name = this->name;
    nt_line.code = this->code;
    nt_line.color = this->color;
    nt_line.sort = this->sort;
    nt_line.backward_name = this->backward_name;
    nt_line.forward_name = this->forward_name;
    nt_line.additional_data = this->additional_data;

    if(this->commercial_mode != NULL)
        nt_line.commercial_mode_idx = this->commercial_mode->idx;

    if(this->network != NULL)
        nt_line.network_idx = this->network->idx;

    return nt_line;
}

nt::City City::get_navitia_type() const {
    navitia::type::City nt_city;
    nt_city.id = this->id;
    nt_city.idx = this->idx;
    nt_city.uri = this->uri;
    nt_city.name = this->name;
    nt_city.comment = this->comment;
    nt_city.coord = this->coord;
    nt_city.main_postal_code = this->main_postal_code;
    nt_city.use_main_stop_area_property = this->use_main_stop_area_property;
    nt_city.main_city = this->main_city;
    if(this->department != NULL)
        nt_city.department_idx = this->department->idx;

    return nt_city;
}

nt::Department Department::get_navitia_type() const {
    navitia::type::Department nt_department;
    nt_department.id = this->id;
    nt_department.idx = this->idx;
    nt_department.uri = this->uri;
    nt_department.name = this->name;

    if(this->district != NULL)
        nt_department.district_idx = this->district->idx;

    return nt_department;
}

nt::Route Route::get_navitia_type() const {
    navitia::type::Route nt_route;
    nt_route.id = this->id;
    nt_route.idx = this->idx;
    nt_route.uri = this->uri;
    nt_route.name = this->name;
    if(this->line != NULL)
        nt_route.line_idx = this->line->idx;

    return nt_route;
}

nt::District District::get_navitia_type() const {
    navitia::type::District nt_district;
    nt_district.id = this->id;
    nt_district.idx = this->idx;
    nt_district.uri = this->uri;
    nt_district.name = this->name;
    if(this->country != NULL)
        nt_district.country_idx = this->country->idx;

    return nt_district;
}

nt::Country Country::get_navitia_type() const {
    navitia::type::Country nt_country;
    nt_country.id = this->id;
    nt_country.idx = this->idx;
    nt_country.uri = this->uri;
    nt_country.name = this->name;
    
    for(auto district : this->district_list)
        nt_country.district_list.push_back(district->idx);

    return nt_country;
}

nt::Network Network::get_navitia_type() const {
    nt::Network nt_network;
    nt_network.id = this->id;
    nt_network.idx = this->idx;
    nt_network.uri = this->uri;
    nt_network.name = this->name;

    nt_network.address_name      = this->address_name;
    nt_network.address_number    = this->address_number;
    nt_network.address_type_name = this->address_type_name;
    nt_network.fax = this->fax;
    nt_network.phone_number = this->phone_number;
    nt_network.mail = this->mail;
    nt_network.website = this->website;
    return nt_network;
}

nt::JourneyPattern JourneyPattern::get_navitia_type() const {
    nt::JourneyPattern nt_journey_pattern;
    nt_journey_pattern.id = this->id;
    nt_journey_pattern.idx = this->idx;
    nt_journey_pattern.uri = this->uri;
    nt_journey_pattern.name = this->name;
    nt_journey_pattern.is_frequence = this->is_frequence;
    
    if(this->route != NULL)
        nt_journey_pattern.route_idx = this->route->idx;

    if(this->physical_mode != NULL)
        nt_journey_pattern.commercial_mode_idx = this->physical_mode->idx;

    return nt_journey_pattern;
}

nt::StopTime StopTime::get_navitia_type() const {
    nt::StopTime nt_stop;
    nt_stop.idx = this->idx;
    nt_stop.arrival_time = this->arrival_time;
    nt_stop.departure_time = this->departure_time;
    nt_stop.start_time = this->start_time;
    nt_stop.end_time = this->end_time;
    nt_stop.headway_secs = this->headway_secs;
    nt_stop.properties[nt::StopTime::ODT] = this->ODT;
    nt_stop.properties[nt::StopTime::DROP_OFF] = this->drop_off_allowed;
    nt_stop.properties[nt::StopTime::PICK_UP] = this->pick_up_allowed;
    nt_stop.properties[nt::StopTime::IS_FREQUENCY] = this->is_frequency;
    nt_stop.properties[nt::StopTime::WHEELCHAIR_BOARDING] = this->wheelchair_boarding;

    nt_stop.local_traffic_zone = this->local_traffic_zone;

    nt_stop.journey_pattern_point_idx = this->journey_pattern_point->idx;
    nt_stop.vehicle_journey_idx = this->vehicle_journey->idx;
    return nt_stop;

}

nt::Connection Connection::get_navitia_type() const {
    nt::Connection nt_connection;
    nt_connection.id = this->id;
    nt_connection.idx = this->idx;
    nt_connection.uri = this->uri;
    nt_connection.departure_stop_point_idx = this->departure_stop_point->idx;
    nt_connection.destination_stop_point_idx = this->destination_stop_point->idx;
    nt_connection.duration = this->duration;
    nt_connection.max_duration = this->max_duration;
    nt_connection.properties = this->properties;
    return nt_connection;
}

nt::JourneyPatternPointConnection 
    JourneyPatternPointConnection::get_navitia_type() const {
    nt::JourneyPatternPointConnection nt_rpc;
    nt_rpc.id = this->id;
    nt_rpc.idx = this->idx;
    nt_rpc.uri = this->uri;
    nt_rpc.departure_journey_pattern_point_idx = this->departure_journey_pattern_point->idx;
    nt_rpc.destination_journey_pattern_point_idx = this->destination_journey_pattern_point->idx;
    nt_rpc.length = this->length;
    switch(this->journey_pattern_point_connection_kind) {
        case Extension: nt_rpc.connection_kind = nt::ConnectionKind::extension; break;
        case Guarantee: nt_rpc.connection_kind = nt::ConnectionKind::guarantee; break;
        case UndefinedJourneyPatternPointConnectionKind: nt_rpc.connection_kind = nt::ConnectionKind::undefined; break;
    };

    return nt_rpc;
}

nt::JourneyPatternPoint JourneyPatternPoint::get_navitia_type() const {
    nt::JourneyPatternPoint nt_journey_pattern_point;
    nt_journey_pattern_point.id = this->id;
    nt_journey_pattern_point.idx = this->idx;
    nt_journey_pattern_point.uri = this->uri;
    nt_journey_pattern_point.order = this->order;
    nt_journey_pattern_point.main_stop_point = this->main_stop_point;
    nt_journey_pattern_point.fare_section = this->fare_section;
    
    nt_journey_pattern_point.stop_point_idx = this->stop_point->idx;
    nt_journey_pattern_point.journey_pattern_idx = this->journey_pattern->idx;
    return nt_journey_pattern_point;
}

nt::VehicleJourney VehicleJourney::get_navitia_type() const {
    nt::VehicleJourney nt_vj;
    nt_vj.id = this->id;
    nt_vj.idx = this->idx;
    nt_vj.name = this->name;
    nt_vj.uri = this->uri;
    nt_vj.comment = this->comment;

    if(this->company != NULL)
        nt_vj.company_idx = this->company->idx;

    if(this->physical_mode != NULL)
        nt_vj.physical_mode_idx = this->physical_mode->idx;

    nt_vj.journey_pattern_idx = this->journey_pattern->idx;

    if(this->validity_pattern != NULL)
        nt_vj.validity_pattern_idx = this->validity_pattern->idx;

    nt_vj.wheelchair_boarding = this->wheelchair_boarding;
    nt_vj.properties = this->properties;

    return nt_vj;
}
nt::ValidityPattern ValidityPattern::get_navitia_type() const {
    nt::ValidityPattern nt_vp;

    nt_vp.id = this->id;
    nt_vp.idx = this->idx;
    nt_vp.uri = this->uri;
    nt_vp.beginning_date = this->beginning_date;

    for(int i=0;i< 366;++i)
        if(this->days[i])
            nt_vp.add(i);
        else
            nt_vp.remove(i);

    return nt_vp;
}
