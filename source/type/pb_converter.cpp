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

#include "pb_converter.h"
#include "georef/georef.h"
#include "georef/street_network.h"
#include "utils/exception.h"
#include "utils/exception.h"
#include <functional>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/date_defs.hpp>
#include <boost/geometry/algorithms/length.hpp>
#include "type/geographical_coord.h"
#include <boost/geometry.hpp>
#include "fare/fare.h"
#include "time_tables/thermometer.h"
#include "vptranslator/block_pattern_to_pb.h"

namespace nt = navitia::type;
namespace pt = boost::posix_time;

namespace navitia{

static pbnavitia::ActiveStatus
compute_disruption_status(const type::new_disruption::Impact& impact,
                          const boost::posix_time::time_period& action_period) {

    bool is_future = false;
    for(const auto& period: impact.application_periods){
        if(period.intersects(action_period)){
            return pbnavitia::active;
        }
        if(!period.is_null() && period.begin() >= action_period.end()){
            is_future = true;
        }
    }

    if(is_future){
        return pbnavitia::future;
    }else{
        return pbnavitia::past;
    }
}

template <typename T>
void fill_address(const T* obj, const nt::Data& data,
                    pbnavitia::Address* address, const std::string& address_name, int house_number,
                    const type::GeographicalCoord& coord, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period) {
    if(obj == nullptr)
        return ;
    int depth = (max_depth <= 3) ? max_depth : 3;
    address->set_name(address_name);
    std::string label;
    if(house_number >= 1){
        address->set_house_number(house_number);
        label += std::to_string(house_number) + " ";
    }
    label += get_label(obj);
    address->set_label(label);

    if(coord.is_initialized()) {
        address->mutable_coord()->set_lon(coord.lon());
        address->mutable_coord()->set_lat(coord.lat());
        std::stringstream ss;
        ss << std::setprecision(16) << coord.lon() << ";" << coord.lat();
        address->set_uri(ss.str());
    }

    if(depth > 0){
        for(georef::Admin* admin : obj->admin_list){
            fill_pb_object(admin, data,  address->add_administrative_regions(),
                           depth-1, now, action_period);
        }
    }
}

static void fill_property(const std::string& name,
                          const std::string& value,
                          pbnavitia::Property* property) {
    property->set_name(name);
    property->set_value(value);
}

template <typename T>
void fill_message(const type::new_disruption::Impact& impact,
        const type::Data&, T pb_object, int,
        const boost::posix_time::ptime&, const boost::posix_time::time_period& action_period) {
    auto pb_disrution = pb_object->add_disruptions();

    pb_disrution->set_disruption_uri(impact.disruption->uri);
    pb_disrution->set_uri(impact.uri);
    for (const auto& app_period: impact.application_periods) {
        auto p = pb_disrution->add_application_periods();
        p->set_begin(navitia::to_posix_timestamp(app_period.begin()));
        p->set_end(navitia::to_posix_timestamp(app_period.last()));
    }

    //TODO: updated at must be computed with the max of all computed values (from disruption, impact, ...)
    pb_disrution->set_updated_at(navitia::to_posix_timestamp(impact.updated_at));

    auto pb_severity = pb_disrution->mutable_severity();
    pb_severity->set_name(impact.severity->wording);
    pb_severity->set_color(impact.severity->color);
    pb_severity->set_effect(to_string(impact.severity->effect));
    pb_severity->set_priority(impact.severity->priority);

    for (const auto& t: impact.disruption->tags) {
        pb_disrution->add_tags(t->name);
    }
    if (impact.disruption->cause) {
        pb_disrution->set_cause(impact.disruption->cause->wording);
    }

    for (const auto& m: impact.messages) {
        auto pb_m = pb_disrution->add_messages();
        pb_m->set_text(m.text);
        auto pb_channel = pb_m->mutable_channel();
        pb_channel->set_content_type(m.channel_content_type);
        pb_channel->set_id(m.channel_id);
        pb_channel->set_name(m.channel_name);
    }

    //we need to compute the active status
    pb_disrution->set_status(compute_disruption_status(impact, action_period));
}

void fill_pb_object(const navitia::type::StopTime* stop_time, const type::Data&,
                    pbnavitia::Properties* properties, int,
                    const boost::posix_time::ptime&, const boost::posix_time::time_period&){
    if (((!stop_time->drop_off_allowed()) && stop_time->pick_up_allowed())
        // No display pick up only information if first stoptime in vehiclejourney
        && ((stop_time->vehicle_journey != nullptr) && (&stop_time->vehicle_journey->stop_time_list.front() != stop_time))){
        properties->add_additional_informations(pbnavitia::Properties::pick_up_only);
    }
    if((stop_time->drop_off_allowed() && (!stop_time->pick_up_allowed()))
        // No display drop off only information if last stoptime in vehiclejourney
        && ((stop_time->vehicle_journey != nullptr) && (&stop_time->vehicle_journey->stop_time_list.back() != stop_time))){
        properties->add_additional_informations(pbnavitia::Properties::drop_off_only);
    }
    if (stop_time->odt()){
        properties->add_additional_informations(pbnavitia::Properties::on_demand_transport);
    }
    if (stop_time->date_time_estimated()){
        properties->add_additional_informations(pbnavitia::Properties::date_time_estimated);
    }
}

void fill_pb_object(const georef::Admin* adm, const nt::Data&,
                    pbnavitia::AdministrativeRegion* admin, int,
                    const pt::ptime&, const pt::time_period&,
                    const bool){
    if(adm == nullptr)
        return ;
    admin->set_name(adm->name);
    admin->set_uri(adm->uri);
    admin->set_label(adm->label);
    admin->set_zip_code(adm->postal_codes_to_string());
    admin->set_level(adm->level);
    if(adm->coord.is_initialized()) {
        admin->mutable_coord()->set_lat(adm->coord.lat());
        admin->mutable_coord()->set_lon(adm->coord.lon());
    }
    if(!adm->insee.empty()){
        admin->set_insee(adm->insee);
    }
}


void fill_pb_object(const nt::Contributor*,
                    const nt::Data& , pbnavitia::Contributor* ,
                    int , const pt::ptime& ,
                    const pt::time_period& , const bool){}

void fill_pb_object(const nt::StopArea* sa,
                    const nt::Data& data, pbnavitia::StopArea* stop_area,
                    int max_depth, const pt::ptime& now,
                    const pt::time_period& action_period, const bool show_codes){
    if(sa == nullptr)
        return ;
    int depth = (max_depth <= 3) ? max_depth : 3;
    stop_area->set_uri(sa->uri);
    stop_area->set_name(sa->name);
    stop_area->set_label(sa->label);
    stop_area->set_timezone(sa->timezone);
    for (const auto& comment: data.pt_data->comments.get(sa)) {
        auto com = stop_area->add_comments();
        com->set_value(comment);
        com->set_type("standard");
    }
    if(sa->coord.is_initialized()) {
        stop_area->mutable_coord()->set_lon(sa->coord.lon());
        stop_area->mutable_coord()->set_lat(sa->coord.lat());
    }
    if(depth > 0){
        for(navitia::georef::Admin* adm : sa->admin_list){
            fill_pb_object(adm, data,  stop_area->add_administrative_regions(),
                           depth-1, now, action_period);
        }
    }
    if (depth > 1) {
        std::vector<nt::idx_t> indexes;
        indexes = sa->get(navitia::type::Type_e::CommercialMode, *(data.pt_data));
        for (const nt::idx_t idx : indexes) {
            nt::CommercialMode* commercial_mode = data.pt_data->commercial_modes[idx];
            fill_pb_object(commercial_mode, data, stop_area->add_commercial_modes(), 0, now, action_period,
                           show_codes);
        }
        indexes = sa->get(navitia::type::Type_e::PhysicalMode, *(data.pt_data));
        for (const nt::idx_t idx : indexes) {
            nt::PhysicalMode* physical_mode = data.pt_data->physical_modes[idx];
            fill_pb_object(physical_mode, data, stop_area->add_physical_modes(), 0, now, action_period,
                           show_codes);
        }
    }

    for(const auto message : sa->get_applicable_messages(now, action_period)){
        fill_message(*message, data, stop_area, max_depth-1, now, action_period);
    }
    if(show_codes) {
        for(auto type_value : sa->codes) {
            fill_codes(type_value.first, type_value.second, stop_area->add_codes());
        }
    }
}


void fill_pb_object(const nt::StopPoint* sp, const nt::Data& data,
                    pbnavitia::StopPoint* stop_point, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period, const bool show_codes){
    if(sp == nullptr)
        return ;
    int depth = (max_depth <= 3) ? max_depth : 3;
    stop_point->set_uri(sp->uri);
    stop_point->set_name(sp->name);
    stop_point->set_label(sp->label);
    if(!sp->platform_code.empty()) {
        stop_point->set_platform_code(sp->platform_code);
    }
    for (const auto& comment: data.pt_data->comments.get(sp)) {
        auto com = stop_point->add_comments();
        com->set_value(comment);
        com->set_type("standard");
    }
    if(sp->coord.is_initialized()) {
        stop_point->mutable_coord()->set_lon(sp->coord.lon());
        stop_point->mutable_coord()->set_lat(sp->coord.lat());
    }

    pbnavitia::hasEquipments* has_equipments =  stop_point->mutable_has_equipments();
    if (sp->wheelchair_boarding()){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_wheelchair_boarding);
    }
    if (sp->sheltered()){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_sheltered);
    }
    if (sp->elevator()){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_elevator);
    }
    if (sp->escalator()){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_escalator);
    }
    if (sp->bike_accepted()){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_bike_accepted);
    }
    if (sp->bike_depot()){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_bike_depot);
    }
    if (sp->visual_announcement()){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_visual_announcement);
    }
    if (sp->audible_announcement()){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_audible_announcement);
    }
    if (sp->appropriate_escort()){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_appropriate_escort);
    }
    if (sp->appropriate_signage()){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_appropriate_signage);
    }
    if(depth > 0){
        for(navitia::georef::Admin* adm : sp->admin_list){
            fill_pb_object(adm, data,  stop_point->add_administrative_regions(),
                           depth-1, now, action_period);
        }
    }
    if(depth > 2){
        fill_pb_object(sp->coord, data, stop_point->mutable_address(), depth - 1, now, action_period);
    }

    if(depth > 0 && sp->stop_area != nullptr)
        fill_pb_object(sp->stop_area, data, stop_point->mutable_stop_area(),
                       depth-1, now, action_period, show_codes);


    for(const auto message : sp->get_applicable_messages(now, action_period)){
        fill_message(*message, data, stop_point, max_depth-1, now, action_period);
    }
    if(show_codes) {
        for(auto type_value : sp->codes) {
            fill_codes(type_value.first, type_value.second, stop_point->add_codes());
        }
    }
}

void fill_pb_object(const navitia::georef::Way* way, const nt::Data& data,
                    pbnavitia::Address * address, int house_number,
                    const type::GeographicalCoord& coord, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period){
   fill_address(way, data, address, way->name, house_number, coord, max_depth, now, action_period);
}

void fill_pb_object(const navitia::type::GeographicalCoord& coord, const type::Data &data, pbnavitia::Address* address,
        int max_depth, const boost::posix_time::ptime& now,
        const boost::posix_time::time_period& action_period){
    if (!coord.is_initialized()) {
        return;
    }

    try{
        const auto nb_way = data.geo_ref->nearest_addr(coord);
        fill_pb_object(nb_way.second, data, address, nb_way.first,
                       coord, max_depth, now, action_period);
    }catch(proximitylist::NotFound){
        LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("logger"),
                       "unable to find a way from coord ["<< coord.lon() << "-" << coord.lat() << "]");
    }
}

void fill_pb_object(nt::Line const* l, const nt::Data& data,
        pbnavitia::Line * line, int max_depth, const pt::ptime& now,
        const pt::time_period& action_period, const bool show_codes){
    if(l == nullptr)
        return ;

    int depth = (max_depth <= 3) ? max_depth : 3;
    for (const auto& comment: data.pt_data->comments.get(l)) {
        auto com = line->add_comments();
        com->set_value(comment);
        com->set_type("standard");
    }
    if(l->code != "")
        line->set_code(l->code);
    if(l->color != "")
        line->set_color(l->color);
    line->set_name(l->name);
    line->set_uri(l->uri);
    if (l->opening_time) {
        line->set_opening_time((*l->opening_time).total_seconds());
    }
    if (l->closing_time) {
        line->set_closing_time((*l->closing_time).total_seconds());
    }


    if (depth > 0) {
        fill_pb_object(l->shape, line->mutable_geojson());

        for(auto route : l->route_list) {
            fill_pb_object(route, data, line->add_routes(), depth-1, now, action_period, show_codes);
        }
        for(auto physical_mode : l->physical_mode_list){
            fill_pb_object(physical_mode, data, line->add_physical_modes(),
                    depth-1, now, action_period, show_codes);
        }

        fill_pb_object(l->commercial_mode, data,
                line->mutable_commercial_mode(), depth-1, now, action_period, show_codes);
        fill_pb_object(l->network, data, line->mutable_network(), depth-1, now, action_period, show_codes);

        for(const auto& line_group : l->line_group_list) {
            fill_pb_object(line_group, data, line->add_line_groups(), depth-1, now, action_period, show_codes);
        }
    }

    for(const auto message : l->get_applicable_messages(now, action_period)){
        fill_message(*message, data, line, depth-1, now, action_period);
    }

    if(show_codes) {
        for(auto type_value : l->codes) {
            fill_codes(type_value.first, type_value.second, line->add_codes());
        }
    }

    for(auto property : l->properties) {
        fill_property(property.first, property.second, line->add_properties());
    }
}

void fill_pb_object(const nt::LineGroup* lg, const nt::Data& data,
        pbnavitia::LineGroup* line_group, int max_depth,
        const pt::ptime& now, const pt::time_period& action_period, const bool show_codes){
    if(lg == nullptr)
        return ;
    int depth = (max_depth <= 3) ? max_depth : 3;

    line_group->set_name(lg->name);
    line_group->set_uri(lg->uri);

    if(depth > 0) {
        for(const auto& line : lg->line_list) {
            fill_pb_object(line, data, line_group->add_lines(), depth-1, now, action_period, show_codes);
        }
        fill_pb_object(lg->main_line, data, line_group->mutable_main_line(), 0, now, action_period, show_codes);

        for (const auto& comment: data.pt_data->comments.get(lg)) {
            auto com = line_group->add_comments();
            com->set_value(comment);
            com->set_type("standard");
        }
    }
}


void fill_pb_object(const nt::JourneyPattern* jp, const nt::Data& data,
        pbnavitia::JourneyPattern * journey_pattern, int max_depth,
        const pt::ptime& now, const pt::time_period& action_period, const bool){
    if(jp == nullptr)
        return ;
    int depth = (max_depth <= 3) ? max_depth : 3;

    journey_pattern->set_name(jp->name);
    journey_pattern->set_uri(jp->uri);
    if(depth > 0 && jp->route != nullptr) {
        fill_pb_object(jp->route, data, journey_pattern->mutable_route(),
                depth-1, now, action_period);
    }
}

void fill_pb_object(const nt::MultiLineString& shape, pbnavitia::MultiLineString* geojson) {
    for (const std::vector<nt::GeographicalCoord>& line: shape) {
        auto l = geojson->add_lines();
        for (const auto coord: line) {
            auto c = l->add_coordinates();
            c->set_lon(coord.lon());
            c->set_lat(coord.lat());
        }
    }
}

void fill_pb_object(const nt::Route* r, const nt::Data& data,
                    pbnavitia::Route* route, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period, const bool show_codes){
    if(r == nullptr)
        return ;
    int depth = (max_depth <= 3) ? max_depth : 3;

    route->set_name(r->name);
    if (r->destination != nullptr){
        fill_pb_placemark(r->destination, data, route->mutable_direction(), max_depth - 1, now, action_period, show_codes);
    }

    for (const auto& comment: data.pt_data->comments.get(r)) {
        auto com = route->add_comments();
        com->set_value(comment);
        com->set_type("standard");
    }

    route->set_uri(r->uri);
    for(const auto& message : r->get_applicable_messages(now, action_period)){
        fill_message(*message, data, route, max_depth-1, now, action_period);
    }

    if(show_codes) {
        for(auto type_value : r->codes) {
            fill_codes(type_value.first, type_value.second, route->add_codes());
        }
    }
    if (depth == 0) {
        return;
    }
    if(r->line != nullptr) {
        fill_pb_object(r->line, data, route->mutable_line(), depth-1,
                       now, action_period, show_codes);
    }

    fill_pb_object(r->shape, route->mutable_geojson());

    auto thermometer = timetables::Thermometer();
    thermometer.generate_thermometer(r);
    if (depth>2) {
        for(auto idx : thermometer.get_thermometer()) {
            auto stop_point = data.pt_data->stop_points[idx];
                fill_pb_object(stop_point, data, route->add_stop_points(), depth-1,
                        now, action_period, show_codes);
        }
        std::set<nt::PhysicalMode*> physical_modes;
        for (auto journey_pattern : r->journey_pattern_list) {
            physical_modes.insert(journey_pattern->physical_mode);
        }
        for (auto physical_mode : physical_modes) {
            fill_pb_object(physical_mode, data, route->add_physical_modes(), 0, now, action_period,
                           show_codes);
        }
    }
}


void fill_pb_object(const nt::Network* n, const nt::Data& data,
                    pbnavitia::Network* network, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period, const bool show_codes){
    if(n == nullptr)
        return ;

    network->set_name(n->name);
    network->set_uri(n->uri);

    for(const auto& message : n->get_applicable_messages(now, action_period)){
        fill_message(*message, data, network, max_depth-1, now, action_period);
    }
    if(show_codes) {
        for(auto type_value : n->codes) {
            fill_codes(type_value.first, type_value.second, network->add_codes());
        }
    }
}


void fill_pb_object(const nt::CommercialMode* m, const nt::Data&,
                    pbnavitia::CommercialMode* commercial_mode,
                    int, const pt::ptime&, const pt::time_period&, const bool){
    if(m == nullptr)
        return ;

    commercial_mode->set_name(m->name);
    commercial_mode->set_uri(m->uri);
}


void fill_pb_object(const nt::PhysicalMode* m, const nt::Data&,
                    pbnavitia::PhysicalMode* physical_mode, int,
                    const pt::ptime&, const pt::time_period&, const bool){
    if(m == nullptr)
        return ;

    physical_mode->set_name(m->name);
    physical_mode->set_uri(m->uri);
}


void fill_pb_object(const nt::Company* c, const nt::Data&,
                    pbnavitia::Company* company, int, const pt::ptime&,
                    const pt::time_period&, const bool show_codes){
    if(c == nullptr)
        return ;

    company->set_name(c->name);
    company->set_uri(c->uri);

    if(show_codes) {
        for(auto type_value : c->codes) {
            fill_codes(type_value.first, type_value.second, company->add_codes());
        }
    }
}


void fill_pb_object(const nt::StopPointConnection* c, const nt::Data& data,
                    pbnavitia::Connection* connection, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period){
    if(c == nullptr)
        return ;
    int depth = (max_depth <= 3) ? max_depth : 3;

    connection->set_duration(c->duration);
    connection->set_display_duration(c->display_duration);
    connection->set_max_duration(c->max_duration);
    if(c->departure != nullptr && c->destination != nullptr && depth > 0){
        fill_pb_object(c->departure, data, connection->mutable_origin(),
                       depth-1, now, action_period);
        fill_pb_object(c->destination, data, connection->mutable_destination(),
                       depth-1, now, action_period);
    }
}



void fill_pb_object(const nt::ValidityPattern* vp, const nt::Data&,
                    pbnavitia::ValidityPattern* validity_pattern, int,
                    const pt::ptime&, const pt::time_period&, const bool){
    if(vp == nullptr)
        return;
    auto vp_string = boost::gregorian::to_iso_string(vp->beginning_date);
    validity_pattern->set_beginning_date(vp_string);
    validity_pattern->set_days(vp->days.to_string());
}

void fill_pb_object(const nt::VehicleJourney* vj,
                    const nt::Data& data,
                    pbnavitia::VehicleJourney * vehicle_journey,
                    int max_depth,
                    const pt::ptime& now,
                    const pt::time_period& action_period,
                    const bool show_codes){
    if(vj == nullptr)
        return ;
    int depth = (max_depth <= 3) ? max_depth : 3;

    vehicle_journey->set_name(vj->name);
    vehicle_journey->set_uri(vj->uri);
    for (const auto& comment: data.pt_data->comments.get(vj)) {
        auto com = vehicle_journey->add_comments();
        com->set_value(comment);
        com->set_type("standard");
    }
    vehicle_journey->set_odt_message(vj->odt_message);
    vehicle_journey->set_is_adapted(vj->is_adapted);

    vehicle_journey->set_wheelchair_accessible(vj->wheelchair_accessible());
    vehicle_journey->set_bike_accepted(vj->bike_accepted());
    vehicle_journey->set_air_conditioned(vj->air_conditioned());
    vehicle_journey->set_visual_announcement(vj->visual_announcement());
    vehicle_journey->set_appropriate_escort(vj->appropriate_escort());
    vehicle_journey->set_appropriate_signage(vj->appropriate_signage());
    vehicle_journey->set_school_vehicle(vj->school_vehicle());

    if(depth > 0) {
        if(vj->journey_pattern != nullptr) {
            fill_pb_object(vj->journey_pattern, data,
                           vehicle_journey->mutable_journey_pattern(), depth-1,
                           now, action_period, show_codes);
        }
        for(const auto& stop_time : vj->stop_time_list) {
            fill_pb_object(&stop_time, data, vehicle_journey->add_stop_times(),
                           depth-1, now, action_period);
        }
        fill_pb_object(vj->journey_pattern->physical_mode, data,
                       vehicle_journey->mutable_journey_pattern()->mutable_physical_mode(), depth-1,
                       now, action_period);
        fill_pb_object(vj->validity_pattern, data,
                       vehicle_journey->mutable_validity_pattern(),
                       depth-1);
        fill_pb_object(vj->adapted_validity_pattern, data,
                       vehicle_journey->mutable_adapted_validity_pattern(),
                       depth-1);
        fill_pb_object(vj->validity_pattern, data, vehicle_journey->mutable_validity_pattern(), max_depth-1);
        fill_pb_object(vj->adapted_validity_pattern, data, vehicle_journey->mutable_adapted_validity_pattern(), max_depth-1);
        vptranslator::fill_pb_object(vptranslator::translate(*vj->validity_pattern),
                                     data,
                                     vehicle_journey,
                                     max_depth - 1,
                                     now,
                                     action_period);
    }

    for(auto message : vj->get_applicable_messages(now, action_period)){
        fill_message(*message, data, vehicle_journey, max_depth-1, now, action_period);
    }

    //si on a un vj théorique rataché à notre vj, on récupére les messages qui le concerne
    if(vj->theoric_vehicle_journey != nullptr){
        for(auto message : vj->theoric_vehicle_journey->get_applicable_messages(now, action_period)){
            fill_message(*message, data, vehicle_journey, max_depth-1, now, action_period);
        }
    }


    if(show_codes) {
        for(auto type_value : vj->codes) {
            fill_codes(type_value.first, type_value.second, vehicle_journey->add_codes());
        }
    }
}


void fill_pb_object(const nt::StopTime* st, const type::Data &data,
                    pbnavitia::StopTime *stop_time, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period){
    if(st == nullptr)
        return ;
    int depth = (max_depth <= 3) ? max_depth : 3;

    //arrival/departure in protobuff are also as seconds from midnight (in UTC of course)
    stop_time->set_arrival_time(st->arrival_time);
    stop_time->set_departure_time(st->departure_time);

    stop_time->set_pickup_allowed(st->pick_up_allowed());
    stop_time->set_drop_off_allowed(st->drop_off_allowed());
    if(st->journey_pattern_point != nullptr && depth > 0) {
        fill_pb_object(st->journey_pattern_point, data,
                       stop_time->mutable_journey_pattern_point(), depth-1,
                       now, action_period);
    }

    if(st->vehicle_journey != nullptr && depth > 0) {
        fill_pb_object(st->vehicle_journey, data,
                       stop_time->mutable_vehicle_journey(), depth-1, now,
                       action_period);
    }
}


void fill_pb_object(const nt::StopTime* st, const type::Data& data,
                    pbnavitia::StopDateTime * stop_date_time, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period) {
    if(st == nullptr)
        return ;
    pbnavitia::Properties* properties = stop_date_time->mutable_properties();
    fill_pb_object(st, data, properties, max_depth, now, action_period);

    for (const std::string& comment: data.pt_data->comments.get(*st)) {
        fill_pb_object(comment, data,  properties->add_notes(), max_depth, now, action_period);
    }
}


void fill_pb_object(const nt::JourneyPatternPoint* jpp, const nt::Data& data,
                    pbnavitia::JourneyPatternPoint * journey_pattern_point,
                    int max_depth, const pt::ptime& now,
                    const pt::time_period& action_period, const bool show_codes){
    if(jpp == nullptr)
        return ;
    int depth = (max_depth <= 3) ? max_depth : 3;

    journey_pattern_point->set_uri(jpp->uri);
    journey_pattern_point->set_order(jpp->order);

    if(depth > 0){
        if(jpp->stop_point != nullptr) {
            fill_pb_object(jpp->stop_point, data,
                           journey_pattern_point->mutable_stop_point(),
                           depth-1, now, action_period);
        }
        if(jpp->journey_pattern != nullptr) {
            fill_pb_object(jpp->journey_pattern, data,
                           journey_pattern_point->mutable_journey_pattern(),
                           depth - 1, now, action_period, show_codes);
        }
    }
}

void fill_codes(const std::string& type, const std::string& value, pbnavitia::Code* code) {
    code->set_type(type);
    code->set_value(value);
}


void fill_pb_placemark(const navitia::georef::Admin* admin, const type::Data &data, pbnavitia::PtObject* pt_object,
                       int max_depth, const boost::posix_time::ptime& now,
                       const boost::posix_time::time_period& action_period,
                       const bool show_codes){
    if(admin == nullptr)
        return;
    int depth = (max_depth <= 3) ? max_depth : 3;
    fill_pb_object(admin, data, pt_object->mutable_administrative_region(), depth,
                   now, action_period, show_codes);

    // we get the label instead of the name to get some additional informations
    pt_object->set_name(admin->label);

    pt_object->set_uri(admin->uri);
    pt_object->set_embedded_type(pbnavitia::ADMINISTRATIVE_REGION);
}

void fill_pb_placemark(const navitia::georef::Way* way,
                       const type::Data &data, pbnavitia::PtObject* place,
                       int house_number,
                       const type::GeographicalCoord& coord,
                       int max_depth, const pt::ptime& now,
                       const pt::time_period& action_period,
                       const bool){
    if(way == nullptr)
        return;
    int depth = (max_depth <= 3) ? max_depth : 3;
    fill_pb_object(way, data, place->mutable_address(),
                   house_number, coord , depth,
                   now, action_period);

    place->set_name(place->address().label());

    place->set_uri(place->address().uri());
    place->set_embedded_type(pbnavitia::ADDRESS);
}

void fill_fare_section(EnhancedResponse& enhanced_response, pbnavitia::Journey* pb_journey, const fare::results& fare) {
    auto pb_fare = pb_journey->mutable_fare();

    size_t cpt_ticket = enhanced_response.response.tickets_size();

    boost::optional<std::string> currency;
    for (const fare::Ticket& ticket : fare.tickets) {
        if (! ticket.is_default_ticket()) {
            if (! currency)
                currency = ticket.currency;
            if (ticket.currency != *currency) {
                throw navitia::exception("cannot have different currencies for tickets");
            } //if we really had to handle different currencies it could be done, but I don't see the point
            // It may happen when not all tickets were found
        }


        pbnavitia::Ticket* pb_ticket = nullptr;
        if (ticket.is_default_ticket()) {
            if (! enhanced_response.unknown_ticket) {
                pb_ticket = enhanced_response.response.add_tickets();
                pb_ticket->set_name(ticket.key);
                pb_ticket->set_found(false);
                pb_ticket->set_id("unknown_ticket");
                pb_ticket->set_comment("unknown ticket");
                enhanced_response.unknown_ticket = pb_ticket;
                pb_fare->add_ticket_id(pb_ticket->id());
            }
            else {
                pb_ticket = enhanced_response.unknown_ticket;
            }
        }
        else {
            pb_ticket = enhanced_response.response.add_tickets();

            pb_ticket->set_name(ticket.key);
            pb_ticket->set_found(true);
            pb_ticket->set_comment(ticket.comment);
            pb_ticket->set_id("ticket_" + boost::lexical_cast<std::string>(++cpt_ticket));
            pb_ticket->mutable_cost()->set_currency(*currency);
            pb_ticket->mutable_cost()->set_value(ticket.value.value);
            pb_fare->add_ticket_id(pb_ticket->id());
        }

        for (auto section: ticket.sections) {
            auto section_id = enhanced_response.get_section_id(pb_journey, section.path_item_idx);
            pb_ticket->add_section_id(section_id);
        }

    }
    pb_fare->mutable_total()->set_value(fare.total.value);
    if (currency)
        pb_fare->mutable_total()->set_currency(*currency);
    pb_fare->set_found(! fare.not_found);
}

static const navitia::georef::POI*
get_nearest_poi(const navitia::type::Data& data,
                const nt::GeographicalCoord& coord,
                const navitia::georef::POIType& poi_type) {
    //we loop through all poi near the coord to find a poi of the required type
    for (const auto pair: data.geo_ref->poi_proximity_list.find_within(coord, 500)) {
        const auto poi_idx = pair.first;
        const auto poi = data.geo_ref->pois[poi_idx];
        if (poi->poitype_idx == poi_type.idx) {
            return poi;
        }
    }
    return nullptr;
}

static const navitia::georef::POI*
get_nearest_parking(const navitia::type::Data& data, const nt::GeographicalCoord& coord) {
    navitia::type::idx_t poi_type_idx = data.geo_ref->poitype_map["poi_type:amenity:parking"];
    const navitia::georef::POIType poi_type = *data.geo_ref->poitypes[poi_type_idx];
    return get_nearest_poi(data, coord, poi_type);
}

static const navitia::georef::POI*
get_nearest_bss_station(const navitia::type::Data& data, const nt::GeographicalCoord& coord) {
    navitia::type::idx_t poi_type_idx = data.geo_ref->poitype_map["poi_type:amenity:bicycle_rental"];
    const navitia::georef::POIType poi_type = *data.geo_ref->poitypes[poi_type_idx];
    return get_nearest_poi(data, coord, poi_type);
}

static void finalize_section(pbnavitia::Section* section,
                             const navitia::georef::PathItem& last_item,
                             const navitia::georef::PathItem& item,
                             const navitia::type::Data& data,
                             const boost::posix_time::ptime departure,
                             int depth,
                             const pt::ptime& now,
                             const pt::time_period& action_period) {

    double total_duration = 0;
    double total_length = 0;
    for (int pb_item_idx = 0 ; pb_item_idx < section->mutable_street_network()->path_items_size() ; ++pb_item_idx) {
        const pbnavitia::PathItem& pb_item = section->mutable_street_network()->path_items(pb_item_idx);
        total_length += pb_item.length();
        total_duration += pb_item.duration();
    }
    section->mutable_street_network()->set_duration(total_duration);
    section->mutable_street_network()->set_length(total_length);
    section->set_duration(total_duration);
    section->set_length(total_length);

    section->set_begin_date_time(navitia::to_posix_timestamp(departure));
    section->set_end_date_time(navitia::to_posix_timestamp(departure + bt::seconds(total_duration)));

    //add the destination as a placemark
    pbnavitia::PtObject* dest_place = section->mutable_destination();

    // we want to have a specific place mark for vls or for the departure if we started from a poi
    switch(item.transportation){
        case georef::PathItem::TransportCaracteristic::BssPutBack:
        {
            const auto vls_station = get_nearest_bss_station(data, item.coordinates.front());
            if (vls_station) {
                fill_pb_placemark(vls_station, data, dest_place, depth, now, action_period);
            } else {
                LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("logger"), "impossible to find the associated BSS putback station poi for coord " << last_item.coordinates.front());
            }
            break;
        }
        case georef::PathItem::TransportCaracteristic::CarPark:
        case georef::PathItem::TransportCaracteristic::CarLeaveParking:
        {
            const auto parking = get_nearest_parking(data, item.coordinates.front());
            if (parking) {
                fill_pb_placemark(parking, data, dest_place, depth, now, action_period);
            } else {
                LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("logger"), "impossible to find the associated parking poi for coord " << last_item.coordinates.front());
            }
            break;
        }
        default: break;
    }
    if (! dest_place->IsInitialized()) {
        auto way = data.geo_ref->ways[last_item.way_idx];
        type::GeographicalCoord coord = last_item.coordinates.back();
        fill_pb_placemark(way, data, dest_place, way->nearest_number(coord).first,
                          coord, depth, now, action_period);
    }

    switch (last_item.transportation) {
    case georef::PathItem::TransportCaracteristic::Walk:
        section->mutable_street_network()->set_mode(pbnavitia::Walking);
        break;
    case georef::PathItem::TransportCaracteristic::Bike:
        section->mutable_street_network()->set_mode(pbnavitia::Bike);
        fill_co2_emission_by_mode(section, data, "physical_mode:Bike");
        break;
    case georef::PathItem::TransportCaracteristic::Car:
        section->mutable_street_network()->set_mode(pbnavitia::Car);
        fill_co2_emission_by_mode(section, data, "physical_mode:Car");
        break;
    case georef::PathItem::TransportCaracteristic::BssTake:
        section->set_type(pbnavitia::BSS_RENT);
        break;
    case georef::PathItem::TransportCaracteristic::BssPutBack:
        section->set_type(pbnavitia::BSS_PUT_BACK);
        break;
    case georef::PathItem::TransportCaracteristic::CarPark:
        section->set_type(pbnavitia::PARK);
        break;
    case georef::PathItem::TransportCaracteristic::CarLeaveParking:
        section->set_type(pbnavitia::LEAVE_PARKING);
        break;
    default:
        throw navitia::exception("Unhandled TransportCaracteristic value in pb_converter");
    }
}


static pbnavitia::GeographicalCoord get_coord(const pbnavitia::PtObject& pt_object) {
    switch(pt_object.embedded_type()) {
    case pbnavitia::NavitiaType::STOP_AREA: return pt_object.stop_area().coord();
    case pbnavitia::NavitiaType::STOP_POINT: return pt_object.stop_point().coord();
    case pbnavitia::NavitiaType::POI: return pt_object.poi().coord();
    case pbnavitia::NavitiaType::ADDRESS: return pt_object.address().coord();
    default: return pbnavitia::GeographicalCoord();
    }
}

static pbnavitia::Section* create_section(EnhancedResponse& response,
                                          pbnavitia::Journey* pb_journey,
                                          const navitia::georef::PathItem& first_item,
                                          const navitia::type::Data& data,
                                          int depth,
                                          const pt::ptime& now,
                                          const pt::time_period& action_period) {
    pbnavitia::Section* prev_section = (pb_journey->sections_size() > 0) ?
            pb_journey->mutable_sections(pb_journey->sections_size()-1) : nullptr;
    auto section = pb_journey->add_sections();
    section->set_id(response.register_section());
    section->set_type(pbnavitia::STREET_NETWORK);

    pbnavitia::PtObject* orig_place = section->mutable_origin();
    // we want to have a specific place mark for vls or for the departure if we started from a poi
    if (first_item.transportation == georef::PathItem::TransportCaracteristic::BssTake) {
        const auto vls_station = get_nearest_bss_station(data, first_item.coordinates.front());
        if (vls_station) {
            fill_pb_placemark(vls_station, data, section->mutable_destination(), depth, now, action_period);
        } else {
            LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("logger"), "impossible to find the associated BSS rent station poi for coord " << first_item.coordinates.front());
        }
    }
    if (prev_section) {
        orig_place->CopyFrom(prev_section->destination());
    } else if (first_item.way_idx != nt::invalid_idx) {
        auto way = data.geo_ref->ways[first_item.way_idx];
        type::GeographicalCoord departure_coord = first_item.coordinates.front();
        fill_pb_placemark(way, data, orig_place, way->nearest_number(departure_coord).first,
                          departure_coord, depth, now, action_period);
    }

    //NOTE: do we want to add a placemark for crow fly sections (they won't have a proper way) ?

    auto origin_coord = get_coord(section->origin());
    if (origin_coord.IsInitialized() && !first_item.coordinates.empty() &&
        first_item.coordinates.front().is_initialized() &&
        first_item.coordinates.front() != type::GeographicalCoord(origin_coord.lon(), origin_coord.lat())) {
        pbnavitia::GeographicalCoord * pb_coord = section->mutable_street_network()->add_coordinates();
        pb_coord->set_lon(origin_coord.lon());
        pb_coord->set_lat(origin_coord.lat());
    }

    return section;
}

void fill_pb_placemark(const type::EntryPoint& point, const type::Data &data,
                       pbnavitia::PtObject* place, int max_depth, const pt::ptime& now,
                       const pt::time_period& action_period, const bool show_codes) {
    if (point.type == type::Type_e::StopPoint) {
        const auto it = data.pt_data->stop_points_map.find(point.uri);
        if (it != data.pt_data->stop_points_map.end()) {
            fill_pb_placemark(it->second, data, place, max_depth, now, action_period, show_codes);
        }
    } else if (point.type == type::Type_e::StopArea) {
        const auto it = data.pt_data->stop_areas_map.find(point.uri);
        if (it != data.pt_data->stop_areas_map.end()) {
            fill_pb_placemark(it->second, data, place, max_depth, now, action_period, show_codes);
        }
    } else if (point.type == type::Type_e::POI) {
        const auto it = data.geo_ref->poi_map.find(point.uri);
        if (it != data.geo_ref->poi_map.end()) {
            fill_pb_placemark(data.geo_ref->pois[it->second], data, place, max_depth, now, action_period);
        }
    } else if (point.type == type::Type_e::Admin) {
        const auto it = data.geo_ref->admin_map.find(point.uri);
        if (it != data.geo_ref->admin_map.end()) {
            fill_pb_placemark(data.geo_ref->admins[it->second], data, place, max_depth, now, action_period);
        }
    } else if (point.type == type::Type_e::Coord){
        try{
            auto address = data.geo_ref->nearest_addr(point.coordinates);
            fill_pb_placemark(address.second, data, place, address.first, point.coordinates, max_depth, now,
                    action_period);
        }catch(proximitylist::NotFound){
            //we didn't find a address at this coordinate, we fill the address manually with the coord, so we have a valid output
            place->set_name("");
            place->set_uri(point.coordinates.uri());
            place->set_embedded_type(pbnavitia::ADDRESS);
            auto addr = place->mutable_address();
            auto c = addr->mutable_coord();
            c->set_lon(point.coordinates.lon());
            c->set_lat(point.coordinates.lat());
        }

    }
}

void fill_crowfly_section(const type::EntryPoint& origin, const type::EntryPoint& destination,
                          const time_duration& crow_fly_duration, type::Mode_e mode,
                          boost::posix_time::ptime origin_time, const type::Data& data,
                          EnhancedResponse& response, pbnavitia::Journey* pb_journey,
                          const pt::ptime& now, const pt::time_period& action_period) {
    pbnavitia::Section* section = pb_journey->add_sections();
    section->set_id(response.register_section());
    fill_pb_placemark(origin, data, section->mutable_origin(), 2, now, action_period);
    fill_pb_placemark(destination, data, section->mutable_destination(), 2, now, action_period);
    section->set_begin_date_time(navitia::to_posix_timestamp(origin_time));
    section->set_duration(crow_fly_duration.total_seconds());
    if (crow_fly_duration.total_seconds() > 0) {
        section->set_length(origin.coordinates.distance_to(destination.coordinates));
        auto* new_coord = section->add_shape();
        new_coord->set_lon(origin.coordinates.lon());
        new_coord->set_lat(origin.coordinates.lat());
        new_coord = section->add_shape();
        new_coord->set_lon(destination.coordinates.lon());
        new_coord->set_lat(destination.coordinates.lat());
    } else {
        section->set_length(0);
    }
    section->set_end_date_time(navitia::to_posix_timestamp(origin_time + crow_fly_duration.to_posix()));
    section->set_type(pbnavitia::SectionType::CROW_FLY);

    //we want to store the transportation mode used
    switch (mode) {
    case type::Mode_e::Walking:
        section->mutable_street_network()->set_mode(pbnavitia::Walking);
        break;
    case type::Mode_e::Bike:
        section->mutable_street_network()->set_mode(pbnavitia::Bike);
        break;
    case type::Mode_e::Car:
        section->mutable_street_network()->set_mode(pbnavitia::Car);
        break;
    case type::Mode_e::Bss:
        section->mutable_street_network()->set_mode(pbnavitia::Bss);
        break;
    default:
        throw navitia::exception("Unhandled TransportCaracteristic value in pb_converter");
    }
}

void fill_street_sections(EnhancedResponse& response, const type::EntryPoint& ori_dest,
                            const georef::Path& path, const type::Data& data,
                            pbnavitia::Journey* pb_journey, const boost::posix_time::ptime departure,
                            int max_depth, const pt::ptime& now, const pt::time_period& action_period) {
    int depth = std::min(max_depth, 3);
    if (path.path_items.empty())
        return;

    auto session_departure = departure;

    boost::optional<georef::PathItem::TransportCaracteristic> last_transportation_carac = {};
    auto section = create_section(response, pb_journey, path.path_items.front(), data,
            depth, now, action_period);
    georef::PathItem last_item;

    //we create 1 section by mean of transport
    for (auto item : path.path_items) {
        auto transport_carac = item.transportation;

        if (last_transportation_carac && transport_carac != *last_transportation_carac) {
            //we end the last section
            finalize_section(section, last_item, item, data, session_departure, depth, now, action_period);
            session_departure += bt::seconds(section->duration());

            //and be create a new one
            section = create_section(response, pb_journey, item, data, depth, now, action_period);
        }

        add_path_item(section->mutable_street_network(), item, ori_dest, data);

        last_transportation_carac = transport_carac;
        last_item = item;
    }

    finalize_section(section, path.path_items.back(), {}, data, session_departure, depth, now, action_period);
    //We add consistency between origin/destination places and geojson

    auto sections = pb_journey->mutable_sections();
    for (auto section = sections->begin(); section != sections->end(); ++section) {
        auto destination_coord = get_coord(section->destination());
        auto sn = section->mutable_street_network();
        if (destination_coord.IsInitialized() ||
                sn->coordinates().size() == 0) {
            continue;
        }
        auto last_coord = sn->coordinates(sn->coordinates_size()-1);
        if (last_coord.IsInitialized() &&
            type::GeographicalCoord(last_coord.lon(), last_coord.lat())
                != type::GeographicalCoord(destination_coord.lon(), destination_coord.lat())) {
            pbnavitia::GeographicalCoord * pb_coord = sn->add_coordinates();
            pb_coord->set_lon(destination_coord.lon());
            pb_coord->set_lat(destination_coord.lat());
        }
    }
}

void add_path_item(pbnavitia::StreetNetwork* sn, const navitia::georef::PathItem& item,
                    const type::EntryPoint &ori_dest, const navitia::type::Data& data) {
    if(item.way_idx >= data.geo_ref->ways.size())
        throw navitia::exception("Wrong way idx : " + boost::lexical_cast<std::string>(item.way_idx));

    pbnavitia::PathItem* path_item = sn->add_path_items();
    path_item->set_name(data.geo_ref->ways[item.way_idx]->name);
    path_item->set_length(item.get_length());
    path_item->set_duration(item.duration.total_seconds() / ori_dest.streetnetwork_params.speed_factor);
    path_item->set_direction(item.angle);

    //we add each path item coordinate to the global coordinate list
    for(auto coord : item.coordinates) {
        if(!coord.is_initialized()) {
            continue;
        }
        pbnavitia::GeographicalCoord * pb_coord = sn->add_coordinates();
        pb_coord->set_lon(coord.lon());
        pb_coord->set_lat(coord.lat());
    }
}

void fill_pb_object(const georef::POIType* geo_poi_type, const type::Data &,
                    pbnavitia::PoiType* poi_type, int,
                    const pt::ptime&, const pt::time_period&) {
    poi_type->set_name(geo_poi_type->name);
    poi_type->set_uri(geo_poi_type->uri);
}


void fill_pb_object(const georef::POI* geopoi, const type::Data &data,
                    pbnavitia::Poi* poi, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period,
                    const bool){
    if(geopoi == nullptr)
        return;
    int depth = (max_depth <= 3) ? max_depth : 3;

    poi->set_name(geopoi->name);
    poi->set_uri(geopoi->uri);
    poi->set_label(geopoi->label);

    if(geopoi->coord.is_initialized()) {
        poi->mutable_coord()->set_lat(geopoi->coord.lat());
        poi->mutable_coord()->set_lon(geopoi->coord.lon());
    }

    if(depth > 0){
        fill_pb_object(data.geo_ref->poitypes[geopoi->poitype_idx], data,
                       poi->mutable_poi_type(), depth-1,
                       now, action_period);
        for(georef::Admin * admin : geopoi->admin_list){
            fill_pb_object(admin, data,  poi->add_administrative_regions(),
                           depth-1, now, action_period);
        }
        pbnavitia::Address* pb_address = poi->mutable_address();
        if(geopoi->address_name.empty()){
            fill_pb_object(geopoi->coord, data, pb_address, depth - 1, now, action_period);
        }else{
            fill_address(geopoi, data, pb_address,
                         geopoi->address_name, geopoi->address_number, geopoi->coord, max_depth, now, action_period);
        }
    }
    for(const auto& propertie : geopoi->properties){
        pbnavitia::Code * pb_code = poi->add_properties();
        pb_code->set_type(propertie.first);
        pb_code->set_value(propertie.second);
    }
}

static pbnavitia::ExceptionType
get_pb_exception_type(const navitia::type::ExceptionDate::ExceptionType exception_type){
    switch (exception_type) {
    case nt::ExceptionDate::ExceptionType::add:
        return pbnavitia::Add;
    case nt::ExceptionDate::ExceptionType::sub:
        return pbnavitia::Remove;
    default:
        throw navitia::exception("exception date case not handled");
    }
}

static bool is_partial_terminus(const navitia::type::StopTime* stop_time){
    return stop_time->vehicle_journey
            && stop_time->vehicle_journey->journey_pattern
            && stop_time->vehicle_journey->journey_pattern->route
            && stop_time->vehicle_journey->journey_pattern->route->destination
            && (!stop_time->journey_pattern_point->journey_pattern->journey_pattern_point_list.empty())
            && stop_time->journey_pattern_point->journey_pattern->journey_pattern_point_list.back()->stop_point
            && stop_time->journey_pattern_point->journey_pattern->journey_pattern_point_list.back()->stop_point->stop_area
            && (stop_time->vehicle_journey->journey_pattern->route->destination
                != stop_time->journey_pattern_point->journey_pattern->journey_pattern_point_list.back()->stop_point->stop_area);
}

void fill_pb_object(const navitia::type::StopTime* stop_time,
                    const nt::Data& data,
                    pbnavitia::ScheduleStopTime* rs_date_time, int max_depth,
                    const boost::posix_time::ptime& now,
                    const boost::posix_time::time_period& action_period,
                    const DateTime& date_time,
                    boost::optional<const std::string> calendar_id){
    if (stop_time == nullptr) {
        //we need to represent a 'null' value (for not found datetime)
        // before it was done with a empty string, but now it is the max value (since 0 is a valid value)
        rs_date_time->set_time(std::numeric_limits<u_int64_t>::max());
        return;
    }

    rs_date_time->set_time(DateTimeUtils::hour(date_time));

    if (! calendar_id) {
        //for calendar we don't want to have a date
        rs_date_time->set_date(to_int_date(to_posix_time(date_time, data)));
    }

    pbnavitia::Properties* hn = rs_date_time->mutable_properties();
    fill_pb_object(stop_time, data, hn, max_depth, now, action_period);

    // partial terminus
    if (is_partial_terminus(stop_time)) {
        auto sa = stop_time->journey_pattern_point->journey_pattern->journey_pattern_point_list.back()->stop_point->stop_area;
        pbnavitia::Destination* destination = hn->mutable_destination();
        std::hash<std::string> hash_fn;
        destination->set_uri("destination:"+std::to_string(hash_fn(sa->name)));
        destination->set_destination(sa->name);
        rs_date_time->set_dt_status(pbnavitia::ResponseStatus::partial_terminus);
    }
    for (const auto& comment: data.pt_data->comments.get(*stop_time)) {
        fill_pb_object(comment, data, hn->add_notes(), max_depth, now, action_period);
    }
    if (stop_time->vehicle_journey != nullptr) {
        if(!stop_time->vehicle_journey->odt_message.empty()){
            fill_pb_object(stop_time->vehicle_journey->odt_message, data, hn->add_notes(),max_depth,now,action_period);
        }
        rs_date_time->mutable_properties()->set_vehicle_journey_id(stop_time->vehicle_journey->uri);
    }

    if((calendar_id) && (stop_time->vehicle_journey != nullptr)) {
        auto asso_cal_it = stop_time->vehicle_journey->meta_vj->associated_calendars.find(*calendar_id);
        if(asso_cal_it != stop_time->vehicle_journey->meta_vj->associated_calendars.end()){
            for(const type::ExceptionDate& excep : asso_cal_it->second->exceptions){
                fill_pb_object(excep, data, hn->add_exceptions(), max_depth, now, action_period);
            }
        }
    }
}

void fill_pb_object(const navitia::type::ExceptionDate& exception_date, const nt::Data&,
                    pbnavitia::CalendarException* calendar_exception, int,
                    const pt::ptime&, const pt::time_period& ){
    pbnavitia::ExceptionType type = get_pb_exception_type(exception_date.type);
    calendar_exception->set_uri("exception:" + std::to_string(type)+boost::gregorian::to_iso_string(exception_date.date));
    calendar_exception->set_type(type);
    calendar_exception->set_date(boost::gregorian::to_iso_string(exception_date.date));
}

void fill_pb_object(const nt::Route* r, const nt::Data& data,
                    pbnavitia::PtDisplayInfo* pt_display_info, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period){
    if(r == nullptr)
        return ;
    pbnavitia::Uris* uris = pt_display_info->mutable_uris();
    uris->set_route(r->uri);
    for (const auto& comment: data.pt_data->comments.get(r)) {
        fill_pb_object(comment, data, pt_display_info->add_notes(), max_depth, now, action_period);
    }
    for(auto message : r->get_applicable_messages(now, action_period)){
        fill_message(*message, data, pt_display_info, max_depth-1, now, action_period);
    }

    if(r->destination != nullptr){
        //Here we format display_informations.direction for stop_schedules.
        pt_display_info->set_direction(r->destination->name);
        for(auto admin : r->destination->admin_list) {
            if (admin->level == 8){
                pt_display_info->set_direction(r->destination->name + " (" + admin->name + ")");
            }
        }

    }

    if (r->line != nullptr){
        pt_display_info->set_color(r->line->color);
        pt_display_info->set_code(r->line->code);
        pt_display_info->set_name(r->line->name);
        for (const auto& comment: data.pt_data->comments.get(r->line)) {
            fill_pb_object(comment, data, pt_display_info->add_notes(), max_depth, now, action_period);
        }
        for(auto message : r->line->get_applicable_messages(now, action_period)){
            fill_message(*message, data, pt_display_info, max_depth-1, now, action_period);
        }
        uris->set_line(r->line->uri);
        if (r->line->network != nullptr){
            pt_display_info->set_network(r->line->network->name);
            uris->set_network(r->line->network->uri);
            for(auto message : r->line->network->get_applicable_messages(now, action_period)){
                fill_message(*message, data, pt_display_info, max_depth-1, now, action_period);
            }
        }
        if (r->line->commercial_mode != nullptr){
            pt_display_info->set_commercial_mode(r->line->commercial_mode->name);
            uris->set_commercial_mode(r->line->commercial_mode->uri);
        }
    }
}

void fill_pb_object(const nt::VehicleJourney* vj,
                    const nt::Data& data,
                    pbnavitia::PtDisplayInfo* pt_display_info,
                    const nt::StopPoint* origin,
                    const nt::StopPoint* destination,
                    int max_depth,
                    const pt::ptime& now,
                    const pt::time_period& action_period)
{
    if(vj == nullptr)
        return ;
    pbnavitia::Uris* uris = pt_display_info->mutable_uris();
    uris->set_vehicle_journey(vj->uri);
    if ((vj->journey_pattern != nullptr) && (vj->journey_pattern->route)){
        fill_pb_object(vj->journey_pattern->route, data, pt_display_info,max_depth,now,action_period);
        uris->set_route(vj->journey_pattern->route->uri);
        uris->set_journey_pattern(vj->journey_pattern->uri);
    }
    for(auto message : vj->get_applicable_messages(now, action_period)){
        fill_message(*message, data, pt_display_info, max_depth-1, now, action_period);
    }
    pt_display_info->set_headsign(vj->name);
    pt_display_info->set_direction(vj->get_direction());
    if ((vj->journey_pattern != nullptr) && (vj->journey_pattern->physical_mode != nullptr)){
        pt_display_info->set_physical_mode(vj->journey_pattern->physical_mode->name);
        uris->set_physical_mode(vj->journey_pattern->physical_mode->uri);
    }
    pt_display_info->set_description(vj->odt_message);
    pbnavitia::hasEquipments* has_equipments = pt_display_info->mutable_has_equipments();
    if(origin && destination){
        fill_pb_object(vj, data, has_equipments, origin, destination,  max_depth-1, now, action_period);
    }else{
        fill_pb_object(vj, data, has_equipments, max_depth-1, now, action_period);
    }
    for (const auto& comment: data.pt_data->comments.get(vj)) {
        fill_pb_object(comment, data, pt_display_info->add_notes(), max_depth, now, action_period);
    }
}

void fill_pb_object(const nt::VehicleJourney* vj,
                    const nt::Data&,
                    pbnavitia::hasEquipments* has_equipments,
                    const nt::StopPoint* origin,
                    const nt::StopPoint* destination,
                    int,
                    const pt::ptime&,
                    const pt::time_period&){
    bool wheelchair_accessible = vj->wheelchair_accessible() && origin->wheelchair_boarding()
        && destination->wheelchair_boarding();
    if (wheelchair_accessible){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_wheelchair_accessibility);
    }
    if (vj->bike_accepted()){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_bike_accepted);
    }
    if (vj->air_conditioned()){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_air_conditioned);
    }
    bool visual_announcement = vj->visual_announcement() && origin->visual_announcement()
        && destination->visual_announcement();
    if (visual_announcement){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_visual_announcement);
    }
    bool audible_announcement = vj->audible_announcement() && origin->audible_announcement()
        && destination->audible_announcement();
    if (audible_announcement){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_audible_announcement);
    }
    bool appropriate_escort = vj->appropriate_escort() && origin->appropriate_escort()
        && destination->appropriate_escort();
    if (appropriate_escort){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_appropriate_escort);
    }
    bool appropriate_signage = vj->appropriate_signage() && origin->appropriate_signage()
        && destination->appropriate_signage();
    if (appropriate_signage){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_appropriate_signage);
    }
    if (vj->school_vehicle()){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_school_vehicle);
    }

}

void fill_pb_object(const nt::VehicleJourney* vj,
                    const nt::Data&,
                    pbnavitia::hasEquipments* has_equipments,
                    int,
                    const pt::ptime&,
                    const pt::time_period&){
    if (vj->wheelchair_accessible()){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_wheelchair_accessibility);
    }
    if (vj->bike_accepted()){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_bike_accepted);
    }
    if (vj->air_conditioned()){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_air_conditioned);
    }
    if (vj->visual_announcement()){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_visual_announcement);
    }
    if (vj->audible_announcement()){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_audible_announcement);
    }
    if (vj->appropriate_escort()){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_appropriate_escort);
    }
    if (vj->appropriate_signage()){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_appropriate_signage);
    }
    if (vj->school_vehicle()){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_school_vehicle);
    }

}

void fill_pb_object(const nt::VehicleJourney* vj,
                    const nt::Data& data,
                    pbnavitia::PtDisplayInfo* pt_display_info,
                    int max_depth,
                    const pt::ptime& now,
                    const pt::time_period& action_period)
{
    fill_pb_object(vj, data, pt_display_info, nullptr, nullptr, max_depth, now, action_period);
}

void fill_pb_object(const nt::Calendar* cal, const nt::Data& data,
                    pbnavitia::Calendar* pb_cal, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period, const bool)
{
    pb_cal->set_uri(cal->uri);
    pb_cal->set_name(cal->name);
    auto vp = pb_cal->mutable_validity_pattern();
    vp->set_beginning_date(boost::gregorian::to_iso_string(cal->validity_pattern.beginning_date));
    std::string vp_str = cal->validity_pattern.str();
    std::reverse(vp_str.begin(), vp_str.end());
    vp->set_days(vp_str);
    auto week = pb_cal->mutable_week_pattern();
    week->set_monday(cal->week_pattern[navitia::Monday]);
    week->set_tuesday(cal->week_pattern[navitia::Tuesday]);
    week->set_wednesday(cal->week_pattern[navitia::Wednesday]);
    week->set_thursday(cal->week_pattern[navitia::Thursday]);
    week->set_friday(cal->week_pattern[navitia::Friday]);
    week->set_saturday(cal->week_pattern[navitia::Saturday]);
    week->set_sunday(cal->week_pattern[navitia::Sunday]);

    for (const auto& p: cal->active_periods) {
        auto pb_period = pb_cal->add_active_periods();
        pb_period->set_begin(boost::gregorian::to_iso_string(p.begin()));
        pb_period->set_end(boost::gregorian::to_iso_string(p.end()));
    }

    for (const auto& excep: cal->exceptions) {
        fill_pb_object(excep, data, pb_cal->add_exceptions(), max_depth, now, action_period);
    }
}

void fill_additional_informations(google::protobuf::RepeatedField<int>* infos,
                                  const bool has_datetime_estimated,
                                  const bool has_odt,
                                  const bool is_zonal) {
    if (has_datetime_estimated) {
        infos->Add(pbnavitia::HAS_DATETIME_ESTIMATED);
    }
    if (is_zonal) {
        infos->Add(pbnavitia::ODT_WITH_ZONE);
    } else if (has_odt && has_datetime_estimated) {
        infos->Add(pbnavitia::ODT_WITH_STOP_POINT);
    } else if (has_odt) {
        infos->Add(pbnavitia::ODT_WITH_STOP_TIME);
    }
    if (infos->size() < 1) {
        infos->Add(pbnavitia::REGULAR);
    }
}

void fill_pb_error(const pbnavitia::Error::error_id id, const std::string& message,
                    pbnavitia::Error* error, int ,
                    const boost::posix_time::ptime& , const boost::posix_time::time_period& ){
    error->set_id(id);
    error->set_message(message);
}

void fill_pb_object(const std::string comment, const nt::Data&,
                    pbnavitia::Note* note, int,
                    const boost::posix_time::ptime&, const boost::posix_time::time_period&){
    std::hash<std::string> hash_fn;
    note->set_uri("note:"+std::to_string(hash_fn(comment)));
    note->set_note(comment);
}

pbnavitia::StreetNetworkMode convert(const navitia::type::Mode_e& mode) {
    switch (mode) {
        case navitia::type::Mode_e::Walking : return pbnavitia::Walking;
        case navitia::type::Mode_e::Bike : return pbnavitia::Bike;
        case navitia::type::Mode_e::Car : return pbnavitia::Car;
        case navitia::type::Mode_e::Bss : return pbnavitia::Bss;
    }
    throw navitia::exception("Techinical Error, unable to convert mode " +
            std::to_string(static_cast<int>(mode)));

}

void fill_co2_emission_by_mode(pbnavitia::Section *pb_section, const nt::Data& data, const std::string& mode_uri){
    if (!mode_uri.empty()){
      const auto it_physical_mode = data.pt_data->physical_modes_map.find(mode_uri);
      if ((it_physical_mode != data.pt_data->physical_modes_map.end())
              && (it_physical_mode->second->co2_emission)){
          pbnavitia::Co2Emission* pb_co2_emission = pb_section->mutable_co2_emission();
          pb_co2_emission->set_unit("gEC");
          pb_co2_emission->set_value((pb_section->length()/1000.0) * (*it_physical_mode->second->co2_emission));
      }
    }
}

void fill_co2_emission(pbnavitia::Section *pb_section, const nt::Data& data, const type::VehicleJourney* vehicle_journey){
    if (vehicle_journey
            && vehicle_journey->journey_pattern
            && vehicle_journey->journey_pattern->physical_mode){
        fill_co2_emission_by_mode(pb_section, data, vehicle_journey->journey_pattern->physical_mode->uri);
    }
}

}//namespace navitia
