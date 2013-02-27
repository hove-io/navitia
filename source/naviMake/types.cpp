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
    if(is_valid(duration))
        add(duration);
    else
        std::cerr << day << " " << beginning_date << " " <<  std::flush;
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


bool CommercialMode::operator<(const CommercialMode& other) const {
    return this->name < other.name;
}

bool PhysicalMode::operator<(const PhysicalMode& other) const {
    if(this->commercial_mode == other.commercial_mode || this->commercial_mode == NULL){
        return this->name < other.name;
    }else{
        return *(this->commercial_mode) < *(other.commercial_mode);
    }
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

navitia::type::StopArea StopArea::Transformer::operator()(const StopArea& stop_area){
    navitia::type::StopArea sa;
    sa.id = stop_area.id;
    sa.idx = stop_area.idx;
    sa.uri = stop_area.uri;
    sa.coord = stop_area.coord;
    sa.comment = stop_area.comment;
    sa.name = stop_area.name;

    sa.additional_data = stop_area.additional_data;
    sa.properties = stop_area.properties;
    return sa;
}


nt::PhysicalMode PhysicalMode::Transformer::operator()(const PhysicalMode& mode){
    nt::PhysicalMode nt_mode;
    nt_mode.id = mode.id;
    nt_mode.idx = mode.idx;
    nt_mode.uri = mode.uri;
    nt_mode.name = mode.name;
    nt_mode.commercial_mode_idx = mode.commercial_mode->idx;
    return nt_mode;

}


nt::CommercialMode CommercialMode::Transformer::operator()(const CommercialMode& commercial_mode){
    nt::CommercialMode nt_commercial_mode;
    nt_commercial_mode.id = commercial_mode.id;
    nt_commercial_mode.idx = commercial_mode.idx;
    nt_commercial_mode.uri = commercial_mode.uri;
    nt_commercial_mode.name = commercial_mode.name;
    return nt_commercial_mode;
}

nt::StopPoint StopPoint::Transformer::operator()(const StopPoint& stop_point){
    nt::StopPoint nt_stop_point;
    nt_stop_point.id = stop_point.id;
    nt_stop_point.idx = stop_point.idx;
    nt_stop_point.uri = stop_point.uri;
    nt_stop_point.name = stop_point.name;
    nt_stop_point.coord = stop_point.coord;
    nt_stop_point.fare_zone = stop_point.fare_zone;

    nt_stop_point.address_name      = stop_point.address_name;
    nt_stop_point.address_number    = stop_point.address_number;       
    nt_stop_point.address_type_name = stop_point.address_type_name;
    
    if(stop_point.stop_area != NULL)
        nt_stop_point.stop_area_idx = stop_point.stop_area->idx;


    if(stop_point.physical_mode != NULL)
        nt_stop_point.physical_mode_idx = stop_point.physical_mode->idx;

    if(stop_point.city != NULL)
        nt_stop_point.city_idx = stop_point.city->idx;

    return nt_stop_point;

}


nt::Line Line::Transformer::operator()(const Line& line){
    navitia::type::Line nt_line;
    nt_line.id = line.id;
    nt_line.idx = line.idx;
    nt_line.uri = line.uri;
    nt_line.name = line.name;
    nt_line.code = line.code;
    nt_line.color = line.color;
    nt_line.sort = line.sort;
    nt_line.backward_name = line.backward_name;
    nt_line.forward_name = line.forward_name;
    nt_line.additional_data = line.additional_data;

    if(line.commercial_mode != NULL)
        nt_line.commercial_mode_idx = line.commercial_mode->idx;

    if(line.network != NULL)
        nt_line.network_idx = line.network->idx;

    return nt_line;
}

nt::City City::Transformer::operator()(const City& city){
    navitia::type::City nt_city;
    nt_city.id = city.id;
    nt_city.idx = city.idx;
    nt_city.uri = city.uri;
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
    nt_department.uri = department.uri;
    nt_department.name = department.name;

    if(department.district != NULL)
        nt_department.district_idx = department.district->idx;
    else
        nt_department.district_idx = nt::invalid_idx;

    return nt_department;
}

nt::Route Route::Transformer::operator()(const Route& route){
    navitia::type::Route nt_route;
    nt_route.id = route.id;
    nt_route.idx = route.idx;
    nt_route.uri = route.uri;
    nt_route.name = route.name;
    if(route.line != NULL)
        nt_route.line_idx = route.line->idx;

    return nt_route;
}

nt::District District::Transformer::operator()(const District& district){
    navitia::type::District nt_district;
    nt_district.id = district.id;
    nt_district.idx = district.idx;
    nt_district.uri = district.uri;
    nt_district.name = district.name;

    return nt_district;
}

nt::Network Network::Transformer::operator()(const Network& network){
    nt::Network nt_network;
    nt_network.id = network.id;
    nt_network.idx = network.idx;
    nt_network.uri = network.uri;
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

nt::JourneyPattern JourneyPattern::Transformer::operator()(const JourneyPattern& journey_pattern){
    nt::JourneyPattern nt_journey_pattern;
    nt_journey_pattern.id = journey_pattern.id;
    nt_journey_pattern.idx = journey_pattern.idx;
    nt_journey_pattern.uri = journey_pattern.uri;
    nt_journey_pattern.name = journey_pattern.name;
    nt_journey_pattern.is_frequence = journey_pattern.is_frequence;
    
    if(journey_pattern.route != NULL)
        nt_journey_pattern.route_idx = journey_pattern.route->idx;

    if(journey_pattern.physical_mode != NULL && journey_pattern.physical_mode->commercial_mode != NULL)
        nt_journey_pattern.commercial_mode_idx = journey_pattern.physical_mode->commercial_mode->idx;

    return nt_journey_pattern;
}

nt::StopTime StopTime::Transformer::operator()(const StopTime& stop){
    nt::StopTime nt_stop;
    nt_stop.idx = stop.idx;
    nt_stop.arrival_time = stop.arrival_time;
    nt_stop.departure_time = stop.departure_time;
    nt_stop.start_time = stop.start_time;
    nt_stop.end_time = stop.end_time;
    nt_stop.headway_secs = stop.headway_secs;
    nt_stop.properties[nt::StopTime::ODT] = stop.ODT;
    nt_stop.properties[nt::StopTime::DROP_OFF] = stop.drop_off_allowed;
    nt_stop.properties[nt::StopTime::PICK_UP] = stop.pick_up_allowed;
    nt_stop.properties[nt::StopTime::IS_FREQUENCY] = stop.is_frequency;

    nt_stop.local_traffic_zone = stop.local_traffic_zone;

    nt_stop.journey_pattern_point_idx = stop.journey_pattern_point->idx;
    nt_stop.vehicle_journey_idx = stop.vehicle_journey->idx;
    return nt_stop;

}

nt::Connection Connection::Transformer::operator()(const Connection& connection){
    nt::Connection nt_connection;
    nt_connection.id = connection.id;
    nt_connection.idx = connection.idx;
    nt_connection.uri = connection.uri;
    nt_connection.departure_stop_point_idx = connection.departure_stop_point->idx;
    nt_connection.destination_stop_point_idx = connection.destination_stop_point->idx;
    nt_connection.duration = connection.duration;
    nt_connection.max_duration = connection.max_duration;
    return nt_connection;
}

nt::JourneyPatternPointConnection 
    JourneyPatternPointConnection::Transformer::operator()(const JourneyPatternPointConnection& journey_pattern_point_connection) {
    nt::JourneyPatternPointConnection nt_rpc;
    nt_rpc.id = journey_pattern_point_connection.id;
    nt_rpc.idx = journey_pattern_point_connection.idx;
    nt_rpc.uri = journey_pattern_point_connection.uri;
    nt_rpc.departure_journey_pattern_point_idx = journey_pattern_point_connection.departure_journey_pattern_point->idx;
    nt_rpc.destination_journey_pattern_point_idx = journey_pattern_point_connection.destination_journey_pattern_point->idx;    
    nt_rpc.length = journey_pattern_point_connection.length;
    switch(journey_pattern_point_connection.journey_pattern_point_connection_kind) {
        case Extension: nt_rpc.connection_kind = nt::ConnectionKind::extension; break;
        case Guarantee: nt_rpc.connection_kind = nt::ConnectionKind::guarantee; break;
        case UndefinedJourneyPatternPointConnectionKind: nt_rpc.connection_kind = nt::ConnectionKind::undefined; break;
    };

    return nt_rpc;
}

nt::JourneyPatternPoint JourneyPatternPoint::Transformer::operator()(const JourneyPatternPoint& journey_pattern_point){
    nt::JourneyPatternPoint nt_journey_pattern_point;
    nt_journey_pattern_point.id = journey_pattern_point.id;
    nt_journey_pattern_point.idx = journey_pattern_point.idx;
    nt_journey_pattern_point.uri = journey_pattern_point.uri;
    nt_journey_pattern_point.order = journey_pattern_point.order;
    nt_journey_pattern_point.main_stop_point = journey_pattern_point.main_stop_point;
    nt_journey_pattern_point.fare_section = journey_pattern_point.fare_section;
    
    nt_journey_pattern_point.stop_point_idx = journey_pattern_point.stop_point->idx;
    nt_journey_pattern_point.journey_pattern_idx = journey_pattern_point.journey_pattern->idx;
    return nt_journey_pattern_point;
}

nt::VehicleJourney VehicleJourney::Transformer::operator()(const VehicleJourney& vj){
    nt::VehicleJourney nt_vj;
    nt_vj.id = vj.id;
    nt_vj.idx = vj.idx;
    nt_vj.name = vj.name;
    nt_vj.uri = vj.uri;
    nt_vj.comment = vj.comment;

    if(vj.company != NULL)
        nt_vj.company_idx = vj.company->idx;

    if(vj.physical_mode != NULL)
        nt_vj.physical_mode_idx = vj.physical_mode->idx;

    nt_vj.journey_pattern_idx = vj.journey_pattern->idx;

    if(vj.validity_pattern != NULL)
        nt_vj.validity_pattern_idx = vj.validity_pattern->idx;

    return nt_vj;
}
nt::ValidityPattern ValidityPattern::Transformer::operator()(const ValidityPattern& vp){
    nt::ValidityPattern nt_vp;

    nt_vp.id = vp.id;
    nt_vp.idx = vp.idx;
    nt_vp.uri = vp.uri;
    nt_vp.beginning_date = vp.beginning_date;

    for(int i=0;i< 366;++i)
        if(vp.days[i])
            nt_vp.add(i);
        else
            nt_vp.remove(i);

    return nt_vp;
}
