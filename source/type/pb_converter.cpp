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
#include "routing/dataraptor.h"
#include "ptreferential/ptreferential.h"

namespace nt = navitia::type;
namespace pt = boost::posix_time;
namespace gd = boost::gregorian;

//*********************************************
namespace ProtoCreator{


struct PbCreator::Filler::PtObjVisitor: public boost::static_visitor<> {
    const nt::disruption::Impact& impact;
    pbnavitia::Impact* pb_impact;

    Filler& filler;
    PtObjVisitor(const nt::disruption::Impact& impact,
                 pbnavitia::Impact* pb_impact, PbCreator::Filler& filler):
        impact(impact),
        pb_impact(pb_impact), filler(filler) {}
    template <typename NavitiaPTObject>
    pbnavitia::ImpactedObject* add_pt_object(const NavitiaPTObject* bo) const {
        auto* pobj = pb_impact->add_impacted_objects();
          // depht = 0 and DumpMessage::No
        filler.copy(0, DumpMessage::No).fill(bo, pobj->mutable_pt_object());
        return pobj;
    }
    template <typename NavitiaPTObject>
    void operator()(const NavitiaPTObject* bo) const {
        add_pt_object(bo);
    }
    void operator()(const nt::disruption::LineSection& line_section) const {
        //TODO: for the moment a line section is only a line, but later we might want to output more stuff
        add_pt_object(line_section.line);
    }
    void operator()(const nt::disruption::UnknownPtObj&) const {}
    void operator()(const nt::MetaVehicleJourney* mvj) const {
        auto* pobj = add_pt_object(mvj);

        //for meta vj we also need to output the impacted stoptimes
        for (const auto& stu: impact.aux_info.stop_times) {
            auto* impacted_stop = pobj->add_impacted_stops();
            impacted_stop->set_cause(stu.cause);
            impacted_stop->set_effect(pbnavitia::DELAYED);
            // depht = 0 and DumpMessage::No
            filler.fill(stu.stop_time.stop_point, impacted_stop->mutable_stop_point(), 0, DumpMessage::No);

            //TODO output only modified stoptime update
            // depht = 0 and DumpMessage::No
            filler.fill(&stu.stop_time, impacted_stop->mutable_amended_stop_time(), 0, DumpMessage::No);

            // we need to get the base stoptime
            // depht = 0 and DumpMessage::No
            const auto* base_st = impact.aux_info.get_base_stop_time(stu);
            if (base_st) {
                filler.fill(base_st, impacted_stop->mutable_base_stop_time(), 0, DumpMessage::No);
            }
        }
    }
};

template<typename NT, typename PB>
static void fill_codes(const NT* nt, const nt::Data& data, PB* pb) {
    for (const auto& code: data.pt_data->codes.get_codes(nt)) {
        for (const auto& value: code.second) {
            auto* pb_code = pb->add_codes();
            pb_code->set_type(code.first);
            pb_code->set_value(value);
        }
    }
}

static void fill_property(const std::string& name,
                          const std::string& value,
                          pbnavitia::Property* property) {
    property->set_name(name);
    property->set_value(value);
}

static bool is_partial_terminus(const navitia::type::StopTime* stop_time){
    return stop_time->vehicle_journey
        && stop_time->vehicle_journey->route
        && stop_time->vehicle_journey->route->destination
        && ! stop_time->vehicle_journey->stop_time_list.empty()
        && stop_time->vehicle_journey->stop_time_list.back().stop_point
        && stop_time->vehicle_journey->route->destination
           != stop_time->vehicle_journey->stop_time_list.back().stop_point->stop_area;
}

PbCreator::Filler PbCreator::Filler::copy(int depth, DumpMessage dump_message){
    if (depth <= 0) {
        return PbCreator::Filler(0, dump_message, pb_creator);
    }
    return PbCreator::Filler(depth, dump_message, pb_creator);
}

void PbCreator::Filler::fill_pb_object(const navitia::type::Contributor* cb, pbnavitia::Contributor* contrib){
    if(cb == nullptr) { return; }
    contrib->set_uri(cb->uri);
    contrib->set_name(cb->name);
    contrib->set_license(cb->license);
    contrib->set_website(cb->website);
}

void PbCreator::Filler::fill_pb_object(const navitia::type::Frame* fr, pbnavitia::Frame* frame){
    if(fr == nullptr) { return; }
    frame->set_uri(fr->uri);
    pt::time_duration td = pt::time_duration(0, 0, 0, 0);
    frame->set_start_validation_date(navitia::to_posix_timestamp(pt::ptime(fr->validation_period.begin(), td)));
    frame->set_end_validation_date(navitia::to_posix_timestamp(pt::ptime(fr->validation_period.end(),td)));
    frame->set_desc(fr->desc);
    frame->set_system(fr->system);

    switch (fr->realtime_level){
        case nt::RTLevel::Base:
        frame->set_realtime_level(pbnavitia::BASE);
        break;
    case nt::RTLevel::Adapted:
        frame->set_realtime_level(pbnavitia::ADAPTED);
        break;
    case nt::RTLevel::RealTime:
        frame->set_realtime_level(pbnavitia::REAL_TIME);
        break;
    default:
        break;
    }
    fill(fr->contributor, frame->mutable_contributor());
}

void PbCreator::Filler::fill_pb_object(const nt::StopArea* sa, pbnavitia::StopArea* stop_area) {
    if(sa == nullptr) { return; }

    stop_area->set_uri(sa->uri);
    stop_area->set_name(sa->name);
    stop_area->set_label(sa->label);
    stop_area->set_timezone(sa->timezone);
    for (const auto& comment: pb_creator.data.pt_data->comments.get(sa)) {
        auto com = stop_area->add_comments();
        com->set_value(comment);
        com->set_type("standard");
    }
    if(sa->coord.is_initialized()) {
        stop_area->mutable_coord()->set_lon(sa->coord.lon());
        stop_area->mutable_coord()->set_lat(sa->coord.lat());
    }
    if(depth > 0){
        fill(sa->admin_list, stop_area->mutable_administrative_regions());
    }

    if (depth > 1) {
        std::vector<navitia::type::CommercialMode*> cm = ptref_indexes<navitia::type::CommercialMode>(sa);
        fill(cm, stop_area->mutable_commercial_modes());

        std::vector<navitia::type::PhysicalMode*> pm = ptref_indexes<navitia::type::PhysicalMode>(sa);
        fill(pm, stop_area->mutable_physical_modes());
    }

    fill_messages(sa, stop_area);
    if (pb_creator.show_codes) { fill_codes(sa, pb_creator.data, stop_area); }
}

void PbCreator::Filler::fill_pb_object(const navitia::georef::Admin* adm, pbnavitia::AdministrativeRegion* admin){

    if(adm == nullptr) { return; }

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

void PbCreator::Filler::fill_pb_object(const nt::StopPoint* sp, pbnavitia::StopPoint* stop_point) {

    if(sp == nullptr) { return; }

    stop_point->set_uri(sp->uri);
    stop_point->set_name(sp->name);
    stop_point->set_label(sp->label);
    if(!sp->platform_code.empty()) {
        stop_point->set_platform_code(sp->platform_code);
    }
    for (const auto& comment: pb_creator.data.pt_data->comments.get(sp)) {
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
        fill(sp->admin_list, stop_point->mutable_administrative_regions());
    }


    if(depth > 2){
        fill(&sp->coord, stop_point->mutable_address());
    }

    if(depth > 0)
        fill(sp->stop_area, stop_point->mutable_stop_area());
    fill_messages(sp, stop_point);
    if (pb_creator.show_codes) { fill_codes(sp, pb_creator.data, stop_point); }
}

void PbCreator::Filler::fill_pb_object(const navitia::type::Company* c, pbnavitia::Company* company){
    if(c == nullptr) { return; }

    company->set_name(c->name);
    company->set_uri(c->uri);

    if (pb_creator.show_codes) { fill_codes(c, pb_creator.data, company); }
}

void PbCreator::Filler::fill_pb_object(const navitia::type::Network* n,
                                         pbnavitia::Network* network){
    if(n == nullptr) { return; }

    network->set_name(n->name);
    network->set_uri(n->uri);

    fill_messages(n, network);
    if (pb_creator.show_codes) { fill_codes(n, pb_creator.data, network); }
}

void PbCreator::Filler::fill_pb_object(const navitia::type::PhysicalMode* m,
                                         pbnavitia::PhysicalMode* physical_mode){
    if(m == nullptr)
        return ;

    physical_mode->set_name(m->name);
    physical_mode->set_uri(m->uri);
}

void PbCreator::Filler::fill_pb_object(const navitia::type::CommercialMode* m,
                      pbnavitia::CommercialMode* commercial_mode){
    if(m == nullptr) { return; }

    commercial_mode->set_name(m->name);
    commercial_mode->set_uri(m->uri);
}

void PbCreator::Filler::fill_pb_object(const navitia::type::Line* l, pbnavitia::Line* line){

    if(l == nullptr) { return; }

    for (const auto& comment: pb_creator.data.pt_data->comments.get(l)) {
        auto com = line->add_comments();
        com->set_value(comment);
        com->set_type("standard");
    }
    if(l->code != "")
        line->set_code(l->code);
    if(l->color != "")
        line->set_color(l->color);

    if(! l->text_color.empty())
        line->set_text_color(l->text_color);

    line->set_name(l->name);
    line->set_uri(l->uri);
    if (l->opening_time) {
        line->set_opening_time((*l->opening_time).total_seconds());
    }
    if (l->closing_time) {
        line->set_closing_time((*l->closing_time).total_seconds());
    }


    if (depth > 0) {
        fill(&l->shape, line->mutable_geojson());

        fill(l->route_list, line->mutable_routes());
        fill(l->physical_mode_list, line->mutable_physical_modes());

        fill(l->commercial_mode, line->mutable_commercial_mode());
        fill(l->network, line->mutable_network());

        fill(l->line_group_list, line->mutable_line_groups());
    }
    fill_messages(l, line);

    if (pb_creator.show_codes) { fill_codes(l, pb_creator.data, line); }

    for(auto property : l->properties) {
        fill_property(property.first, property.second, line->add_properties());
    }

}

void PbCreator::Filler::fill_pb_object(const navitia::type::Route* r, pbnavitia::Route* route){

    if(r == nullptr) { return; }

    route->set_name(r->name);
    route->set_direction_type(r->direction_type);

    fill(r->destination,route->mutable_direction());

    for (const auto& comment: pb_creator.data.pt_data->comments.get(r)) {
        auto com = route->add_comments();
        com->set_value(comment);
        com->set_type("standard");
    }

    route->set_uri(r->uri);
    fill_messages(r, route);

    if (pb_creator.show_codes) { fill_codes(r, pb_creator.data, route); }

    if (depth == 0) { return; }

    fill(r->line, route->mutable_line());

    fill(&r->shape, route->mutable_geojson());

    auto thermometer = navitia::timetables::Thermometer();
    thermometer.generate_thermometer(r);
    if (depth>2) {
        for(auto idx : thermometer.get_thermometer()) {
            auto stop_point = pb_creator.data.pt_data->stop_points[idx];
            fill(stop_point, route->add_stop_points());
        }
        std::set<const nt::PhysicalMode*> physical_modes;
        r->for_each_vehicle_journey([&](const nt::VehicleJourney& vj) {
                physical_modes.insert(vj.physical_mode);
                return true;
            });
        //Attention : here depth should be 0
        for (auto physical_mode : physical_modes) {
            fill(physical_mode, route->add_physical_modes());
        }
    }
}

void PbCreator::Filler::fill_pb_object(const navitia::type::LineGroup* lg,
                                         pbnavitia::LineGroup* line_group){

    if(lg == nullptr) { return; }

    line_group->set_name(lg->name);
    line_group->set_uri(lg->uri);

    if(depth > 0) {
        fill(lg->line_list, line_group->mutable_lines());
        //Attention: Here we pass depth = 0
        fill(lg->main_line, line_group->mutable_main_line(), 0);

        for (const auto& comment: pb_creator.data.pt_data->comments.get(lg)) {
            auto com = line_group->add_comments();
            com->set_value(comment);
            com->set_type("standard");
        }
    }
}

void PbCreator::Filler::fill_pb_object(const navitia::type::Calendar* cal, pbnavitia::Calendar* pb_cal){
     if (cal == nullptr) { return; }
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
        fill(&excep, pb_cal->add_exceptions());
    }
}

void PbCreator::Filler::fill_pb_object(const navitia::type::ExceptionDate* exception_date,
                                         pbnavitia::CalendarException* calendar_exception){
    pbnavitia::ExceptionType type;
    switch (exception_date->type) {
    case nt::ExceptionDate::ExceptionType::add:
        type = pbnavitia::Add;
        break;
    case nt::ExceptionDate::ExceptionType::sub:
        type = pbnavitia::Remove;
        break;
    default:
        throw navitia::exception("exception date case not handled");
    }
    calendar_exception->set_uri("exception:" + std::to_string(type) +
                                boost::gregorian::to_iso_string(exception_date->date));
    calendar_exception->set_type(type);
    calendar_exception->set_date(boost::gregorian::to_iso_string(exception_date->date));
}

void PbCreator::Filler::fill_pb_object(const navitia::type::ValidityPattern* vp,
                                         pbnavitia::ValidityPattern* validity_pattern){
    if(vp == nullptr) { return; }

    auto vp_string = boost::gregorian::to_iso_string(vp->beginning_date);
    validity_pattern->set_beginning_date(vp_string);
    validity_pattern->set_days(vp->days.to_string());
}

void PbCreator::Filler::fill_pb_object(const navitia::type::VehicleJourney* vj,
                                         pbnavitia::VehicleJourney* vehicle_journey){

    if (vj == nullptr) { return; }

    vehicle_journey->set_name(vj->name);
    vehicle_journey->set_uri(vj->uri);
    for (const auto& comment: pb_creator.data.pt_data->comments.get(vj)) {
        auto com = vehicle_journey->add_comments();
        com->set_value(comment);
        com->set_type("standard");
    }
    vehicle_journey->set_odt_message(vj->odt_message);
    vehicle_journey->set_is_adapted(vj->realtime_level == nt::RTLevel::Adapted);

    vehicle_journey->set_wheelchair_accessible(vj->wheelchair_accessible());
    vehicle_journey->set_bike_accepted(vj->bike_accepted());
    vehicle_journey->set_air_conditioned(vj->air_conditioned());
    vehicle_journey->set_visual_announcement(vj->visual_announcement());
    vehicle_journey->set_appropriate_escort(vj->appropriate_escort());
    vehicle_journey->set_appropriate_signage(vj->appropriate_signage());
    vehicle_journey->set_school_vehicle(vj->school_vehicle());

    if(depth > 0) {
        const auto& jp_idx = pb_creator.data.dataRaptor->jp_container.get_jp_from_vj()[navitia::routing::VjIdx(*vj)];
        const auto& pair_jp = pb_creator.data.dataRaptor->jp_container.get_jps()[jp_idx.val];
        fill(&pair_jp, vehicle_journey->mutable_journey_pattern());

        fill(vj->stop_time_list, vehicle_journey->mutable_stop_times());
        fill(vj->meta_vj, vehicle_journey->mutable_trip());

        fill(vj->physical_mode, vehicle_journey->mutable_journey_pattern()->mutable_physical_mode());
        fill(vj->base_validity_pattern(), vehicle_journey->mutable_validity_pattern());
        fill(vj->adapted_validity_pattern(), vehicle_journey->mutable_adapted_validity_pattern());
        fill(vj->base_validity_pattern(),vehicle_journey->mutable_validity_pattern());
        fill(vj->adapted_validity_pattern(), vehicle_journey->mutable_adapted_validity_pattern());

        const auto& vector_bp = navitia::vptranslator::translate(*vj->base_validity_pattern());

        fill(vector_bp,vehicle_journey->mutable_calendars());
    }
    fill_messages(vj->meta_vj, vehicle_journey);

    if (pb_creator.show_codes) { fill_codes(vj, pb_creator.data, vehicle_journey); }
}

void PbCreator::Filler::fill_pb_object(const nt::MetaVehicleJourney* nav_mvj, pbnavitia::Trip* pb_trip){
    pb_trip->set_uri(nav_mvj->uri);
}

void PbCreator::Filler::fill_pb_object(const navitia::type::MultiLineString* shape,
                                         pbnavitia::MultiLineString* geojson){
    for (const std::vector<nt::GeographicalCoord>& line: *shape) {
        auto l = geojson->add_lines();
        for (const auto coord: line) {
            auto c = l->add_coordinates();
            c->set_lon(coord.lon());
            c->set_lat(coord.lat());
        }
    }
}


void PbCreator::Filler::fill_pb_object(const navitia::type::GeographicalCoord* coord,
                                         pbnavitia::Address* address){
    if (!coord->is_initialized()) {
        return;
    }

    try{
        const auto nb_way = pb_creator.data.geo_ref->nearest_addr(*coord);
        house_number_coord hn = {nb_way.first, *coord};
        const way_pair waypair = {nb_way.second, hn};
        fill(&waypair, address);
    }catch(navitia::proximitylist::NotFound){
        LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("logger"),
                       "unable to find a way from coord ["<< coord->lon() << "-" << coord->lat() << "]");
    }
}

void PbCreator::Filler::fill_pb_object(const way_pair* waypair, pbnavitia::Address* address){
    const navitia::georef::Way* way = waypair->first;
    const way_pair_name waypair_name = {{way, way->name}, waypair->second};
    fill(&waypair_name, address);
}

void PbCreator::Filler::fill_pb_object(const way_pair_name* waypair_name, pbnavitia::Address* address){
    const way_name wayname = waypair_name->first;
    const house_number_coord hn_coord = waypair_name->second;

    if(wayname.first == nullptr) { return; }

    address->set_name(wayname.second);
    std::string label;
    if(hn_coord.first >= 1){
        address->set_house_number(hn_coord.first);
        label += std::to_string(hn_coord.first) + " ";
    }
//    label += get_label(wayname.first);
//    address->set_label(label);

    if(hn_coord.second.is_initialized()) {
        address->mutable_coord()->set_lon(hn_coord.second.lon());
        address->mutable_coord()->set_lat(hn_coord.second.lat());
        std::stringstream ss;
        ss << std::setprecision(16) << hn_coord.second.lon() << ";" << hn_coord.second.lat();
        address->set_uri(ss.str());
    }

    if(depth > 0){
        fill(wayname.first->admin_list, address->mutable_administrative_regions());
    }
}

void PbCreator::Filler::fill_pb_object(const nt::StopPointConnection* c, pbnavitia::Connection* connection){
    if(c == nullptr) { return; }

    connection->set_duration(c->duration);
    connection->set_display_duration(c->display_duration);
    connection->set_max_duration(c->max_duration);
    if(c->departure != nullptr && c->destination != nullptr && depth > 0){
        fill(c->departure, connection->mutable_origin());
        fill(c->destination, connection->mutable_destination());
    }
}
void PbCreator::Filler::fill_pb_object(const navitia::type::StopTime* st, pbnavitia::StopTime* stop_time){

    // arrival/departure in protobuff are as seconds from midnight in local time
    const auto offset = [&](const uint32_t time) {
        static const auto flag = std::numeric_limits<uint32_t>::max();
        return time == flag ? 0 : st->vehicle_journey->utc_to_local_offset();
    };
    stop_time->set_arrival_time(st->arrival_time + offset(st->arrival_time));
    stop_time->set_departure_time(st->departure_time + offset(st->departure_time));
    stop_time->set_headsign(pb_creator.data.pt_data->headsign_handler.get_headsign(*st));

    stop_time->set_pickup_allowed(st->pick_up_allowed());
    stop_time->set_drop_off_allowed(st->drop_off_allowed());

    // TODO V2: the dump of the JPP is deprecated, but we keep it for retrocompatibility
    if (depth > 0) {
        const auto& jpp_idx = pb_creator.data.dataRaptor->jp_container.get_jpp(*st);
        const auto& pair_jpp = pb_creator.data.dataRaptor->jp_container.get_jpps()[jpp_idx.val];
        fill(&pair_jpp, stop_time->mutable_journey_pattern_point());
    }

    // we always dump the stop point (with the same depth)
    fill(st->stop_point, stop_time->mutable_stop_point(), depth);

    if ( depth > 0) {
        fill(st->vehicle_journey, stop_time->mutable_vehicle_journey());
    }
}

void PbCreator::Filler::fill_pb_object(const nt::StopTime* st, pbnavitia::StopDateTime* stop_date_time){

    if(st == nullptr) { return; }

    pbnavitia::Properties* properties = stop_date_time->mutable_properties();
    fill(st, properties);

    for (const std::string& comment: pb_creator.data.pt_data->comments.get(*st)) {
        fill(&comment, properties->add_notes());
    }
}

void PbCreator::Filler::fill_pb_object(const std::string* comment, pbnavitia::Note* note){
    std::hash<std::string> hash_fn;
    note->set_uri("note:"+std::to_string(hash_fn(*comment)));
    note->set_note(*comment);
}

void PbCreator::Filler::fill_pb_object(const jp_pair* jp, pbnavitia::JourneyPattern* journey_pattern){

    const std::string id = pb_creator.data.dataRaptor->jp_container.get_id(jp->first);
    journey_pattern->set_name(id);
    journey_pattern->set_uri(id);
    if (depth > 0) {
        const auto* route = pb_creator.data.pt_data->routes[jp->second.route_idx.val];
        fill(route,journey_pattern->mutable_route());
    }
}

void PbCreator::Filler::fill_pb_object(const jpp_pair* jpp,
                                       pbnavitia::JourneyPatternPoint* journey_pattern_point){

    journey_pattern_point->set_uri(pb_creator.data.dataRaptor->jp_container.get_id(jpp->first));
    journey_pattern_point->set_order(jpp->second.order);

    if(depth > 0){
        fill(pb_creator.data.pt_data->stop_points[jpp->second.sp_idx.val],
                journey_pattern_point->mutable_stop_point());
        const auto& jp = pb_creator.data.dataRaptor->jp_container.get_jps()[jpp->second.jp_idx.val];
        fill(&jp, journey_pattern_point->mutable_journey_pattern());
    }
}
void PbCreator::Filler::fill_pb_object(const navitia::vptranslator::BlockPattern* bp,
                                       pbnavitia::Calendar* pb_cal){

    auto week = pb_cal->mutable_week_pattern();
    week->set_monday(bp->week[navitia::Monday]);
    week->set_tuesday(bp->week[navitia::Tuesday]);
    week->set_wednesday(bp->week[navitia::Wednesday]);
    week->set_thursday(bp->week[navitia::Thursday]);
    week->set_friday(bp->week[navitia::Friday]);
    week->set_saturday(bp->week[navitia::Saturday]);
    week->set_sunday(bp->week[navitia::Sunday]);

    for (const auto& p: bp->validity_periods) {
        auto pb_period = pb_cal->add_active_periods();
        pb_period->set_begin(boost::gregorian::to_iso_string(p.begin()));
        pb_period->set_end(boost::gregorian::to_iso_string(p.end()));
    }

    for (const auto& excep: bp->exceptions) {
        fill(&excep, pb_cal->add_exceptions());
    }

}


void PbCreator::Filler::fill_pb_object(const navitia::type::StopTime* stop_time, pbnavitia::Properties* properties){
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

void PbCreator::Filler::fill_informed_entity(const nt::disruption::PtObj& ptobj,
                                             const nt::disruption::Impact& impact,
                                             pbnavitia::Impact* pb_impact){
    auto filler = copy(depth - 1, dump_message);
        boost::apply_visitor(PtObjVisitor(impact, pb_impact, filler), ptobj);
    }


static pbnavitia::ActiveStatus
compute_disruption_status(const navitia::type::disruption::Impact& impact,
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

template <typename P>
void PbCreator::Filler::fill_message(const navitia::type::disruption::Impact& impact, P pb_object){
    auto pb_impact = pb_object->add_impacts();

    pb_impact->set_disruption_uri(impact.disruption->uri);

    if (!impact.disruption->contributor.empty()) {
        pb_impact->set_contributor(impact.disruption->contributor);
    }

    pb_impact->set_uri(impact.uri);
    for (const auto& app_period: impact.application_periods) {
        auto p = pb_impact->add_application_periods();
        p->set_begin(navitia::to_posix_timestamp(app_period.begin()));
        p->set_end(navitia::to_posix_timestamp(app_period.last()));
    }

    //TODO: updated at must be computed with the max of all computed values (from disruption, impact, ...)
    pb_impact->set_updated_at(navitia::to_posix_timestamp(impact.updated_at));

    auto pb_severity = pb_impact->mutable_severity();
    pb_severity->set_name(impact.severity->wording);
    pb_severity->set_color(impact.severity->color);
    pb_severity->set_effect(to_string(impact.severity->effect));
    pb_severity->set_priority(impact.severity->priority);

    for (const auto& t: impact.disruption->tags) {
        pb_impact->add_tags(t->name);
    }
    if (impact.disruption->cause) {
        pb_impact->set_cause(impact.disruption->cause->wording);
    }

    for (const auto& m: impact.messages) {
        auto pb_m = pb_impact->add_messages();
        pb_m->set_text(m.text);
        auto pb_channel = pb_m->mutable_channel();
        pb_channel->set_content_type(m.channel_content_type);
        pb_channel->set_id(m.channel_id);
        pb_channel->set_name(m.channel_name);
        for (const auto& type: m.channel_types){
            switch (type) {
            case nt::disruption::ChannelType::web:
                pb_channel->add_channel_types(pbnavitia::Channel::web);
                break;
            case nt::disruption::ChannelType::sms:
                pb_channel->add_channel_types(pbnavitia::Channel::sms);
                break;
            case nt::disruption::ChannelType::email:
                pb_channel->add_channel_types(pbnavitia::Channel::email);
                break;
            case nt::disruption::ChannelType::mobile:
                pb_channel->add_channel_types(pbnavitia::Channel::mobile);
                break;
            case nt::disruption::ChannelType::notification:
                pb_channel->add_channel_types(pbnavitia::Channel::notification);
                break;
            case nt::disruption::ChannelType::twitter:
                pb_channel->add_channel_types(pbnavitia::Channel::twitter);
                break;
            case nt::disruption::ChannelType::facebook:
                pb_channel->add_channel_types(pbnavitia::Channel::facebook);
                break;
            case nt::disruption::ChannelType::unknown_type:
                pb_channel->add_channel_types(pbnavitia::Channel::unknown_type);
                break;
            }
        }
    }

    //we need to compute the active status
    pb_impact->set_status(compute_disruption_status(impact, pb_creator.action_period));

    for (const auto& informed_entity: impact.informed_entities) {
        fill_informed_entity(informed_entity, impact, pb_impact);
    }
}

void PbCreator::Filler::fill_pb_object(const nt::Route* r, pbnavitia::PtDisplayInfo* pt_display_info){
    if(r == nullptr) { return; }

    pbnavitia::Uris* uris = pt_display_info->mutable_uris();
    uris->set_route(r->uri);
    for (const auto& comment: pb_creator.data.pt_data->comments.get(r)) {
        fill(&comment, pt_display_info->add_notes());
    }
    fill_messages(r, pt_display_info);

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
        for (const auto& comment: pb_creator.data.pt_data->comments.get(r->line)) {
            fill(&comment, pt_display_info->add_notes());
        }

        fill_messages(r->line, pt_display_info);
        uris->set_line(r->line->uri);
        if (r->line->network != nullptr){
            pt_display_info->set_network(r->line->network->name);
            uris->set_network(r->line->network->uri);

            fill_messages(r->line->network, pt_display_info);
        }
        if (r->line->commercial_mode != nullptr){
            pt_display_info->set_commercial_mode(r->line->commercial_mode->name);
            uris->set_commercial_mode(r->line->commercial_mode->uri);
        }
    }
}


void PbCreator::Filler::fill_pb_object(const navitia::georef::POI* geopoi, pbnavitia::Poi* poi){
    if(geopoi == nullptr) { return; }

    poi->set_name(geopoi->name);
    poi->set_uri(geopoi->uri);
    poi->set_label(geopoi->label);

    if(geopoi->coord.is_initialized()) {
        poi->mutable_coord()->set_lat(geopoi->coord.lat());
        poi->mutable_coord()->set_lon(geopoi->coord.lon());
    }

    if(depth > 0){
        fill(pb_creator.data.geo_ref->poitypes[geopoi->poitype_idx], poi->mutable_poi_type());

        fill(geopoi->admin_list, poi->mutable_administrative_regions());

        pbnavitia::Address* pb_address = poi->mutable_address();
        if(geopoi->address_name.empty()){
            fill(&geopoi->coord, pb_address);
        }else{
            fill(geopoi, pb_address);
        }
    }
    for(const auto& propertie : geopoi->properties){
        pbnavitia::Code * pb_code = poi->add_properties();
        pb_code->set_type(propertie.first);
        pb_code->set_value(propertie.second);
    }
}

void PbCreator::Filler::fill_pb_object(const navitia::georef::POI* poi, pbnavitia::Address* address){
    if(poi == nullptr) { return; }

    address->set_name(poi->address_name);
    std::string label;
    if(poi->address_number >= 1){
        address->set_house_number(poi->address_number);
        label += std::to_string(poi->address_number) + " ";
    }
    label += get_label(poi);
    address->set_label(label);

    if(poi->coord.is_initialized()) {
        address->mutable_coord()->set_lon(poi->coord.lon());
        address->mutable_coord()->set_lat(poi->coord.lat());
        std::stringstream ss;
        ss << std::setprecision(16) << poi->coord.lon() << ";" << poi->coord.lat();
        address->set_uri(ss.str());
    }

    if(depth > 0){
        fill(poi->admin_list, address->mutable_administrative_regions());
    }
}

void PbCreator::Filler::fill_pb_object(const navitia::georef::POIType* geo_poi_type,
                                       pbnavitia::PoiType* poi_type){
    poi_type->set_name(geo_poi_type->name);
    poi_type->set_uri(geo_poi_type->uri);
}

void PbCreator::Filler::fill_pb_object(const navitia::type::disruption::Impact* impact,
                                       pbnavitia::Response* response){
    fill_message(*impact, response);
}

void PbCreator::Filler::fill_pb_object(const VjOrigDest* vj_orig_dest, pbnavitia::hasEquipments* has_equipments){
    bool wheelchair_accessible = vj_orig_dest->vj->wheelchair_accessible() && vj_orig_dest->orig->wheelchair_boarding()
        && vj_orig_dest->dest->wheelchair_boarding();
    if (wheelchair_accessible){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_wheelchair_accessibility);
    }
    if (vj_orig_dest->vj->bike_accepted()){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_bike_accepted);
    }
    if (vj_orig_dest->vj->air_conditioned()){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_air_conditioned);
    }
    bool visual_announcement = vj_orig_dest->vj->visual_announcement() && vj_orig_dest->orig->visual_announcement()
        && vj_orig_dest->dest->visual_announcement();
    if (visual_announcement){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_visual_announcement);
    }
    bool audible_announcement = vj_orig_dest->vj->audible_announcement() && vj_orig_dest->orig->audible_announcement()
        && vj_orig_dest->dest->audible_announcement();
    if (audible_announcement){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_audible_announcement);
    }
    bool appropriate_escort = vj_orig_dest->vj->appropriate_escort() && vj_orig_dest->orig->appropriate_escort()
        && vj_orig_dest->dest->appropriate_escort();
    if (appropriate_escort){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_appropriate_escort);
    }
    bool appropriate_signage = vj_orig_dest->vj->appropriate_signage() && vj_orig_dest->orig->appropriate_signage()
        && vj_orig_dest->dest->appropriate_signage();
    if (appropriate_signage){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_appropriate_signage);
    }
    if (vj_orig_dest->vj->school_vehicle()){
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_school_vehicle);
    }
}

void PbCreator::Filler::fill_pb_object(const VjStopTimes* vj_stoptimes,
                                       pbnavitia::PtDisplayInfo* pt_display_info){
    if(vj_stoptimes->vj == nullptr)
        return ;
    pbnavitia::Uris* uris = pt_display_info->mutable_uris();
    uris->set_vehicle_journey(vj_stoptimes->vj->uri);
    if (depth > 0 && vj_stoptimes->vj->route) {
        fill(vj_stoptimes->vj->route, pt_display_info);
        uris->set_route(vj_stoptimes->vj->route->uri);
        const auto& jp_idx = pb_creator.data.dataRaptor->jp_container.get_jp_from_vj()[navitia::routing::VjIdx(*vj_stoptimes->vj)];
        uris->set_journey_pattern(pb_creator.data.dataRaptor->jp_container.get_id(jp_idx));
    }

    fill_messages(vj_stoptimes->vj->meta_vj, pt_display_info);

    if (vj_stoptimes->orig != nullptr) {
        pt_display_info->set_headsign(pb_creator.data.pt_data->headsign_handler.get_headsign(*vj_stoptimes->orig));
    }
    pt_display_info->set_direction(vj_stoptimes->vj->get_direction());
    if (vj_stoptimes->vj->physical_mode != nullptr) {
        pt_display_info->set_physical_mode(vj_stoptimes->vj->physical_mode->name);
        uris->set_physical_mode(vj_stoptimes->vj->physical_mode->uri);
    }
    pt_display_info->set_description(vj_stoptimes->vj->odt_message);
    pbnavitia::hasEquipments* has_equipments = pt_display_info->mutable_has_equipments();
    if (vj_stoptimes->orig && vj_stoptimes->dest) {
        const auto& vj = VjOrigDest(vj_stoptimes->vj, vj_stoptimes->orig->stop_point, vj_stoptimes->dest->stop_point);
        fill(&vj, has_equipments);
    } else {
        fill(vj_stoptimes->vj, has_equipments);
    }
    for (const auto& comment: pb_creator.data.pt_data->comments.get(vj_stoptimes->vj)) {
        fill(&comment, pt_display_info->add_notes());
    }
}

void PbCreator::Filler::fill_pb_object(const navitia::type::VehicleJourney* vj,
                                       pbnavitia::hasEquipments* has_equipments){
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

void PbCreator::Filler::fill_pb_object(const StopTimeCalandar* stop_time_calendar,
                                       pbnavitia::ScheduleStopTime* rs_date_time){
    if (stop_time_calendar->stop_time == nullptr) {
        //we need to represent a 'null' value (for not found datetime)
        // before it was done with a empty string, but now it is the max value (since 0 is a valid value)
        rs_date_time->set_time(std::numeric_limits<u_int64_t>::max());
        return;
    }

    rs_date_time->set_time(navitia::DateTimeUtils::hour(stop_time_calendar->date_time));

    if (! stop_time_calendar->calendar_id) {
        //for calendar we don't want to have a date
        rs_date_time->set_date(navitia::to_int_date(navitia::to_posix_time(stop_time_calendar->date_time,pb_creator.data)));
    }

    pbnavitia::Properties* hn = rs_date_time->mutable_properties();
    fill(stop_time_calendar->stop_time, hn);

    // partial terminus
    if (is_partial_terminus(stop_time_calendar->stop_time)) {
        auto sa = stop_time_calendar->stop_time->vehicle_journey->stop_time_list.back().stop_point->stop_area;
        pbnavitia::Destination* destination = hn->mutable_destination();
        std::hash<std::string> hash_fn;
        destination->set_uri("destination:"+std::to_string(hash_fn(sa->name)));
        destination->set_destination(sa->name);
        rs_date_time->set_dt_status(pbnavitia::ResponseStatus::partial_terminus);
    }
    for (const auto& comment: pb_creator.data.pt_data->comments.get(*stop_time_calendar->stop_time)) {
        fill(&comment, hn->add_notes());
    }
    if (stop_time_calendar->stop_time->vehicle_journey != nullptr) {
        if(!stop_time_calendar->stop_time->vehicle_journey->odt_message.empty()){
            fill(&stop_time_calendar->stop_time->vehicle_journey->odt_message, hn->add_notes());
        }
        rs_date_time->mutable_properties()->set_vehicle_journey_id(stop_time_calendar->stop_time->vehicle_journey->uri);
    }

    if((stop_time_calendar->calendar_id) && (stop_time_calendar->stop_time->vehicle_journey != nullptr)) {
        auto asso_cal_it = stop_time_calendar->stop_time->vehicle_journey->meta_vj->associated_calendars.find(*stop_time_calendar->calendar_id);
        if(asso_cal_it != stop_time_calendar->stop_time->vehicle_journey->meta_vj->associated_calendars.end()){
            fill(asso_cal_it->second->exceptions, hn->mutable_exceptions());
        }
    }
}

void PbCreator::Filler::fill_pb_object(const navitia::type::EntryPoint* point, pbnavitia::PtObject* place){
    if (point->type == navitia::type::Type_e::StopPoint) {
        const auto it = pb_creator.data.pt_data->stop_points_map.find(point->uri);
        if (it != pb_creator.data.pt_data->stop_points_map.end()) {
            fill(it->second, place);
        }
    } else if (point->type == navitia::type::Type_e::StopArea) {
        const auto it = pb_creator.data.pt_data->stop_areas_map.find(point->uri);
        if (it != pb_creator.data.pt_data->stop_areas_map.end()) {
            fill(it->second, place);
        }
    } else if (point->type == navitia::type::Type_e::POI) {
        const auto it = pb_creator.data.geo_ref->poi_map.find(point->uri);
        if (it != pb_creator.data.geo_ref->poi_map.end()) {
            fill(pb_creator.data.geo_ref->pois[it->second], place);
        }
    } else if (point->type == navitia::type::Type_e::Admin) {
        const auto it = pb_creator.data.geo_ref->admin_map.find(point->uri);
        if (it != pb_creator.data.geo_ref->admin_map.end()) {
            fill(pb_creator.data.geo_ref->admins[it->second], place);
        }
    } else if (point->type == navitia::type::Type_e::Coord){
        try{
            auto address = pb_creator.data.geo_ref->nearest_addr(point->coordinates);
            navitia::fill_pb_placemark(address.second, pb_creator.data, place, address.first,
                                       point->coordinates, depth, pb_creator.now,
                                       pb_creator.action_period);
        }catch(navitia::proximitylist::NotFound){
            //we didn't find a address at this coordinate, we fill the address manually with the coord, so we have a valid output
            place->set_name("");
            place->set_uri(point->coordinates.uri());
            place->set_embedded_type(pbnavitia::ADDRESS);
            auto addr = place->mutable_address();
            auto c = addr->mutable_coord();
            c->set_lon(point->coordinates.lon());
            c->set_lat(point->coordinates.lat());
        }

    }
}

void PbCreator::Filler::fill_pb_object(const WayCoord* way_coord, pbnavitia::PtObject*  place){
    if(way_coord->way == nullptr)
        return;

    fill(way_coord, place->mutable_address(), depth);

    place->set_name(place->address().label());

    place->set_uri(place->address().uri());
    place->set_embedded_type(pbnavitia::ADDRESS);
}

void PbCreator::Filler::fill_pb_object(const WayCoord* way_coord, pbnavitia::Address* address){
    const navitia::georef::Way* way = way_coord->way;
    const way_pair_name waypair_name = {{way, way->name}, {way_coord->number, way_coord->coord}};
    fill(&waypair_name, address, depth);
}

}

//*********************************************

namespace navitia {

template<typename NT, typename PB>
static void fill_codes(const NT* nt, const nt::Data& data, PB* pb) {
    for (const auto& code: data.pt_data->codes.get_codes(nt)) {
        for (const auto& value: code.second) {
            auto* pb_code = pb->add_codes();
            pb_code->set_type(code.first);
            pb_code->set_value(value);
        }
    }
}


static void fill_modes_for_stop_area(pbnavitia::StopArea* pb_sa,
                                     const navitia::type::Type_e type_e,
                                     const nt::Data& data,
                                     const navitia::type::StopArea* stop_area,
                                     int depth,
                                     const pt::ptime& now,
                                     const pt::time_period& action_period,
                                     const bool show_codes) {
    std::vector<navitia::type::idx_t> indexes;
    try{
        std::string request = "stop_area.uri=" + stop_area->uri;
        indexes = ptref::make_query(type_e, request, data);
    } catch(const ptref::parsing_error &parse_error) {
        LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("logger"),
                       "fill_modes_from_stop_area, Unable to parse :" + parse_error.more);
    } catch(const ptref::ptref_error &pt_error) {
        LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("logger"),
                       "fill_modes_from_stop_area, " + pt_error.more);
    }
    switch (type_e) {
        case navitia::type::Type_e::CommercialMode:
            for(const navitia::type::idx_t idx: indexes){
                navitia::type::CommercialMode* cm = data.pt_data->commercial_modes[idx];
                fill_pb_object(cm, data, pb_sa->add_commercial_modes(), depth, now, action_period,
                                           show_codes);
            }
            break;
        case navitia::type::Type_e::PhysicalMode:
            for(const navitia::type::idx_t idx: indexes){
                navitia::type::PhysicalMode* pm = data.pt_data->physical_modes[idx];
                fill_pb_object(pm, data, pb_sa->add_physical_modes(), depth, now, action_period,
                                           show_codes);
            }
            break;
         default:
            LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("logger"), "fill_modes_from_stop_area, unkown type "
                            << navitia::type::static_data::captionByType(type_e));
            break;
    }

}

static pbnavitia::ActiveStatus
compute_disruption_status(const type::disruption::Impact& impact,
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

struct PtObjVisitor: public boost::static_visitor<> {
    const nt::Data& data;
    const nt::disruption::Impact& impact;
    pbnavitia::Impact* pb_impact;
    const pt::ptime& now;
    const pt::time_period& action_period;
    const bool show_codes;
    PtObjVisitor(const nt::Data& data,
                 const nt::disruption::Impact& impact,
                 pbnavitia::Impact* pb_impact,
                 int /*depth*/,
                 const pt::ptime& now,
                 const pt::time_period& action_period,
                 const bool show_codes):
                 data(data), impact(impact), pb_impact(pb_impact),
                 now(now), action_period(action_period), show_codes(show_codes) {}
    template <typename NavitiaPTObject>
    pbnavitia::ImpactedObject* add_pt_object(const NavitiaPTObject* bo) const {
        auto* pobj = pb_impact->add_impacted_objects();
        fill_pb_placemark(bo, data, pobj->mutable_pt_object(),
                          0, now, action_period, show_codes, DumpMessage::No);
        return pobj;
    }
    template <typename NavitiaPTObject>
    void operator()(const NavitiaPTObject* bo) const {
        add_pt_object(bo);
    }
    void operator()(const nt::disruption::LineSection& line_section) const {
        //TODO: for the moment a line section is only a line, but later we might want to output more stuff
        add_pt_object(line_section.line);
    }
    void operator()(const nt::disruption::UnknownPtObj&) const {}
    void operator()(const nt::MetaVehicleJourney* mvj) const {
        auto* pobj = add_pt_object(mvj);

        //for meta vj we also need to output the impacted stoptimes
        for (const auto& stu: impact.aux_info.stop_times) {
            auto* impacted_stop = pobj->add_impacted_stops();
            impacted_stop->set_cause(stu.cause);
            impacted_stop->set_effect(pbnavitia::DELAYED);
            fill_pb_object(stu.stop_time.stop_point, data, impacted_stop->mutable_stop_point(),
                           0, now, action_period, show_codes, DumpMessage::No);

            //TODO output only modified stoptime update
            fill_pb_object(stu.stop_time, data, impacted_stop->mutable_amended_stop_time(),
                           0, now, action_period, show_codes, DumpMessage::No);

            // we need to get the base stoptime
            const auto* base_st = impact.aux_info.get_base_stop_time(stu);
            if (base_st) {
                fill_pb_object(*base_st, data, impacted_stop->mutable_base_stop_time(),
                               0, now, action_period, show_codes, DumpMessage::No);
            }
        }
    }
};


static void fill_pb_object(const nt::disruption::PtObj& ptobj,
                           const nt::disruption::Impact& impact,
                           const nt::Data& data,
                           pbnavitia::Impact* pb_impact, int max_depth,
                           const pt::ptime& now, const pt::time_period& action_period,
                           const bool show_codes) {
    boost::apply_visitor(PtObjVisitor(data, impact, pb_impact, max_depth, now, action_period, show_codes), ptobj);
}

template <typename T>
void fill_message(const type::disruption::Impact& impact,
        const type::Data& data, T pb_object, int depth,
        const boost::posix_time::ptime& now, const boost::posix_time::time_period& action_period,
        const bool show_codes) {
    auto pb_impact = pb_object->add_impacts();

    pb_impact->set_disruption_uri(impact.disruption->uri);

    if (!impact.disruption->contributor.empty()) {
        pb_impact->set_contributor(impact.disruption->contributor);
    }

    pb_impact->set_uri(impact.uri);
    for (const auto& app_period: impact.application_periods) {
        auto p = pb_impact->add_application_periods();
        p->set_begin(navitia::to_posix_timestamp(app_period.begin()));
        p->set_end(navitia::to_posix_timestamp(app_period.last()));
    }

    //TODO: updated at must be computed with the max of all computed values (from disruption, impact, ...)
    pb_impact->set_updated_at(navitia::to_posix_timestamp(impact.updated_at));

    auto pb_severity = pb_impact->mutable_severity();
    pb_severity->set_name(impact.severity->wording);
    pb_severity->set_color(impact.severity->color);
    pb_severity->set_effect(to_string(impact.severity->effect));
    pb_severity->set_priority(impact.severity->priority);

    for (const auto& t: impact.disruption->tags) {
        pb_impact->add_tags(t->name);
    }
    if (impact.disruption->cause) {
        pb_impact->set_cause(impact.disruption->cause->wording);
    }

    for (const auto& m: impact.messages) {
        auto pb_m = pb_impact->add_messages();
        pb_m->set_text(m.text);
        auto pb_channel = pb_m->mutable_channel();
        pb_channel->set_content_type(m.channel_content_type);
        pb_channel->set_id(m.channel_id);
        pb_channel->set_name(m.channel_name);
        for (const auto& type: m.channel_types){
            switch (type) {
            case nt::disruption::ChannelType::web:
                pb_channel->add_channel_types(pbnavitia::Channel::web);
                break;
            case nt::disruption::ChannelType::sms:
                pb_channel->add_channel_types(pbnavitia::Channel::sms);
                break;
            case nt::disruption::ChannelType::email:
                pb_channel->add_channel_types(pbnavitia::Channel::email);
                break;
            case nt::disruption::ChannelType::mobile:
                pb_channel->add_channel_types(pbnavitia::Channel::mobile);
                break;
            case nt::disruption::ChannelType::notification:
                pb_channel->add_channel_types(pbnavitia::Channel::notification);
                break;
            case nt::disruption::ChannelType::twitter:
                pb_channel->add_channel_types(pbnavitia::Channel::twitter);
                break;
            case nt::disruption::ChannelType::facebook:
                pb_channel->add_channel_types(pbnavitia::Channel::facebook);
                break;
            case nt::disruption::ChannelType::unknown_type:
                pb_channel->add_channel_types(pbnavitia::Channel::unknown_type);
                break;
            }
        }
    }

    //we need to compute the active status
    pb_impact->set_status(compute_disruption_status(impact, action_period));

    for (const auto& informed_entity: impact.informed_entities) {
        fill_pb_object(informed_entity, impact, data, pb_impact,
                       depth, now, action_period, show_codes);
    }
}


template <typename HasMessage, typename PbObj>
void fill_messages(const HasMessage* obj, const type::Data& data, PbObj pb_obj, int max_depth,
                   const boost::posix_time::ptime& now,
                   const boost::posix_time::time_period& action_period,
                   const bool show_codes, const DumpMessage dump_message) {
    if (dump_message == DumpMessage::No) { return; }
    for (const auto& message : obj->get_applicable_messages(now, action_period)){
        fill_message(*message, data, pb_obj, max_depth, now, action_period, show_codes);
    }
}

void fill_pb_object(const type::disruption::Impact* impact,
                    const type::Data& data,
                    pbnavitia::Response* response,
                    int depth,
                    const boost::posix_time::ptime& current_time,
                    const boost::posix_time::time_period& action_period,
                    bool /*show_codes*/) {
    fill_message(*impact, data, response, depth, current_time, action_period);
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
                    const bool, const DumpMessage) {
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

void fill_pb_object(const nt::Contributor* cb,
                    const nt::Data& , pbnavitia::Contributor* contributor,
                    int , const pt::ptime& ,
                    const pt::time_period& , const bool, const DumpMessage) {
    if(cb == nullptr)
        return;
    contributor->set_uri(cb->uri);
    contributor->set_name(cb->name);
    contributor->set_license(cb->license);
    contributor->set_website(cb->website);
}

void fill_pb_object(const nt::Frame* fr,
                    const nt::Data& data, pbnavitia::Frame* frame,
                    int depth, const pt::ptime&  now,
                    const pt::time_period& action_period,
                    const bool show_codes, const DumpMessage dump_message){
    if(fr == nullptr)
        return;
    frame->set_uri(fr->uri);
    pt::time_duration td = pt::time_duration(0, 0, 0, 0);
    frame->set_start_validation_date(navitia::to_posix_timestamp(pt::ptime(fr->validation_period.begin(), td)));
    frame->set_end_validation_date(navitia::to_posix_timestamp(pt::ptime(fr->validation_period.end(),td)));
    frame->set_desc(fr->desc);
    frame->set_system(fr->system);

    switch (fr->realtime_level){
        case nt::RTLevel::Base:
        frame->set_realtime_level(pbnavitia::BASE);
        break;
    case nt::RTLevel::Adapted:
        frame->set_realtime_level(pbnavitia::ADAPTED);
        break;
    case nt::RTLevel::RealTime:
        frame->set_realtime_level(pbnavitia::REAL_TIME);
        break;
    default:
        break;
    }

    if(fr->contributor != nullptr) {
        fill_pb_object(fr->contributor, data, frame->mutable_contributor(), depth-1,
                      now, action_period, show_codes, dump_message);
    }
}

void fill_pb_object(const nt::StopArea* sa,
                    const nt::Data& data, pbnavitia::StopArea* stop_area,
                    int max_depth, const pt::ptime& now,
                    const pt::time_period& action_period, const bool show_codes,
                    const DumpMessage dump_message) {
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
        fill_modes_for_stop_area(stop_area, navitia::type::Type_e::CommercialMode,
                                 data, sa, 0, now, action_period, show_codes);

        fill_modes_for_stop_area(stop_area, navitia::type::Type_e::PhysicalMode,
                                 data, sa, 0, now, action_period, show_codes);
    }

    fill_messages(sa, data, stop_area, max_depth-1, now, action_period, show_codes, dump_message);
    if (show_codes) { fill_codes(sa, data, stop_area); }
}


void fill_pb_object(const nt::StopPoint* sp, const nt::Data& data,
                    pbnavitia::StopPoint* stop_point, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period,
                    const bool show_codes, const DumpMessage dump_message) {
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

    fill_messages(sp, data, stop_point, max_depth-1, now, action_period, show_codes, dump_message);
    if (show_codes) { fill_codes(sp, data, stop_point); }
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
                    pbnavitia::Line* line, int max_depth, const pt::ptime& now,
                    const pt::time_period& action_period, const bool show_codes,
                    const DumpMessage dump_message) {
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

    if(! l->text_color.empty())
        line->set_text_color(l->text_color);

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

    fill_messages(l, data, line, max_depth-1, now, action_period, show_codes, dump_message);

    if (show_codes) { fill_codes(l, data, line); }

    for(auto property : l->properties) {
        fill_property(property.first, property.second, line->add_properties());
    }
}

void fill_pb_object(const nt::LineGroup* lg, const nt::Data& data,
                    pbnavitia::LineGroup* line_group, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period,
                    const bool show_codes, const DumpMessage dump_message) {
    if(lg == nullptr)
        return ;
    int depth = (max_depth <= 3) ? max_depth : 3;

    line_group->set_name(lg->name);
    line_group->set_uri(lg->uri);

    if(depth > 0) {
        for(const auto& line : lg->line_list) {
            fill_pb_object(line, data, line_group->add_lines(), depth-1,
                           now, action_period, show_codes, dump_message);
        }
        fill_pb_object(lg->main_line, data, line_group->mutable_main_line(), 0,
                       now, action_period, show_codes, dump_message);

        for (const auto& comment: data.pt_data->comments.get(lg)) {
            auto com = line_group->add_comments();
            com->set_value(comment);
            com->set_type("standard");
        }
    }
}


void fill_pb_object(const std::pair<const routing::JpIdx, const routing::JourneyPattern&>& jp,
                    const nt::Data& data,
                    pbnavitia::JourneyPattern * journey_pattern,
                    int max_depth,
                    const pt::ptime& now,
                    const pt::time_period& action_period,
                    const bool show_codes){
    int depth = (max_depth <= 3) ? max_depth : 3;

    const std::string id = data.dataRaptor->jp_container.get_id(jp.first);
    journey_pattern->set_name(id);
    journey_pattern->set_uri(id);
    if (depth > 0) {
        const auto* route = data.pt_data->routes[jp.second.route_idx.val];
        fill_pb_object(route, data, journey_pattern->mutable_route(),
                depth-1, now, action_period, show_codes);
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
                    const pt::ptime& now, const pt::time_period& action_period,
                    const bool show_codes, const DumpMessage dump_message) {
    if(r == nullptr)
        return ;
    int depth = (max_depth <= 3) ? max_depth : 3;

    route->set_name(r->name);
    route->set_direction_type(r->direction_type);

    if (r->destination != nullptr){
        fill_pb_placemark(r->destination, data, route->mutable_direction(), max_depth - 1, now, action_period, show_codes);
    }

    for (const auto& comment: data.pt_data->comments.get(r)) {
        auto com = route->add_comments();
        com->set_value(comment);
        com->set_type("standard");
    }

    route->set_uri(r->uri);

    fill_messages(r, data, route, max_depth-1, now, action_period, show_codes, dump_message);

    if (show_codes) { fill_codes(r, data, route); }

    if (depth == 0) { return; }

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
        std::set<const nt::PhysicalMode*> physical_modes;
        r->for_each_vehicle_journey([&](const nt::VehicleJourney& vj) {
                physical_modes.insert(vj.physical_mode);
                return true;
            });
        for (auto physical_mode : physical_modes) {
            fill_pb_object(physical_mode, data, route->add_physical_modes(), 0, now, action_period,
                           show_codes);
        }
    }
}


void fill_pb_object(const nt::Network* n, const nt::Data& data,
                    pbnavitia::Network* network, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period,
                    const bool show_codes, const DumpMessage dump_message) {
    if(n == nullptr)
        return ;

    network->set_name(n->name);
    network->set_uri(n->uri);

    fill_messages(n, data, network, max_depth-1, now, action_period, show_codes, dump_message);
    if (show_codes) { fill_codes(n, data, network); }
}


void fill_pb_object(const nt::CommercialMode* m, const nt::Data&,
                    pbnavitia::CommercialMode* commercial_mode,
                    int, const pt::ptime&, const pt::time_period&, const bool, const DumpMessage){
    if(m == nullptr)
        return ;

    commercial_mode->set_name(m->name);
    commercial_mode->set_uri(m->uri);
}


void fill_pb_object(const nt::PhysicalMode* m, const nt::Data&,
                    pbnavitia::PhysicalMode* physical_mode, int,
                    const pt::ptime&, const pt::time_period&, const bool, const DumpMessage){
    if(m == nullptr)
        return ;

    physical_mode->set_name(m->name);
    physical_mode->set_uri(m->uri);
}


void fill_pb_object(const nt::Company* c, const nt::Data& data,
                    pbnavitia::Company* company, int, const pt::ptime&,
                    const pt::time_period&, const bool show_codes, const DumpMessage){
    if(c == nullptr)
        return ;

    company->set_name(c->name);
    company->set_uri(c->uri);

    if (show_codes) { fill_codes(c, data, company); }
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
                    const pt::ptime&, const pt::time_period&, const bool, const DumpMessage) {
    if(vp == nullptr)
        return;
    auto vp_string = boost::gregorian::to_iso_string(vp->beginning_date);
    validity_pattern->set_beginning_date(vp_string);
    validity_pattern->set_days(vp->days.to_string());
}

void fill_pb_object(const nt::MetaVehicleJourney* nav_mvj,
                    const nt::Data&,
                    pbnavitia::Trip* pb_trip,
                    int /*max_depth*/,
                    const pt::ptime& /*now*/,
                    const pt::time_period& /*action_period*/,
                    const bool /*show_codes*/,
                    const DumpMessage) {
    pb_trip->set_uri(nav_mvj->uri);
}

void fill_pb_object(const nt::VehicleJourney* vj,
                    const nt::Data& data,
                    pbnavitia::VehicleJourney* vehicle_journey,
                    int max_depth,
                    const pt::ptime& now,
                    const pt::time_period& action_period,
                    const bool show_codes,
                    const DumpMessage dump_message){
    if (vj == nullptr) { return; }
    int depth = (max_depth <= 3) ? max_depth : 3;

    vehicle_journey->set_name(vj->name);
    vehicle_journey->set_uri(vj->uri);
    for (const auto& comment: data.pt_data->comments.get(vj)) {
        auto com = vehicle_journey->add_comments();
        com->set_value(comment);
        com->set_type("standard");
    }
    vehicle_journey->set_odt_message(vj->odt_message);
    vehicle_journey->set_is_adapted(vj->realtime_level == nt::RTLevel::Adapted);

    vehicle_journey->set_wheelchair_accessible(vj->wheelchair_accessible());
    vehicle_journey->set_bike_accepted(vj->bike_accepted());
    vehicle_journey->set_air_conditioned(vj->air_conditioned());
    vehicle_journey->set_visual_announcement(vj->visual_announcement());
    vehicle_journey->set_appropriate_escort(vj->appropriate_escort());
    vehicle_journey->set_appropriate_signage(vj->appropriate_signage());
    vehicle_journey->set_school_vehicle(vj->school_vehicle());

    if(depth > 0) {
        const auto& jp_idx = data.dataRaptor->jp_container.get_jp_from_vj()[routing::VjIdx(*vj)];
        fill_pb_object(data.dataRaptor->jp_container.get_jps()[jp_idx.val], data,
                       vehicle_journey->mutable_journey_pattern(), depth-1,
                       now, action_period, show_codes);
        for(const auto& stop_time : vj->stop_time_list) {
            fill_pb_object(stop_time, data, vehicle_journey->add_stop_times(),
                           depth-1, now, action_period, show_codes, dump_message);
        }
        fill_pb_object(vj->meta_vj, data, vehicle_journey->mutable_trip(), depth-1, now,
                       action_period, show_codes);
        fill_pb_object(vj->physical_mode, data,
                       vehicle_journey->mutable_journey_pattern()->mutable_physical_mode(), depth-1,
                       now, action_period);
        fill_pb_object(vj->base_validity_pattern(), data,
                       vehicle_journey->mutable_validity_pattern(),
                       depth-1);
        fill_pb_object(vj->adapted_validity_pattern(), data,
                       vehicle_journey->mutable_adapted_validity_pattern(),
                       depth-1);
        fill_pb_object(vj->base_validity_pattern(), data,
                       vehicle_journey->mutable_validity_pattern(), max_depth-1);
        fill_pb_object(vj->adapted_validity_pattern(), data,
                       vehicle_journey->mutable_adapted_validity_pattern(), max_depth-1);
//        vptranslator::fill_pb_object(vptranslator::translate(*vj->base_validity_pattern()),
//                                     data,
//                                     vehicle_journey,
//                                     max_depth - 1,
//                                     now,
//                                     action_period);
    }

    fill_messages(vj->meta_vj, data, vehicle_journey, max_depth-1, now, action_period, show_codes, dump_message);

    if (show_codes) { fill_codes(vj, data, vehicle_journey); }
}


void fill_pb_object(const nt::StopTime& st, const type::Data &data,
                    pbnavitia::StopTime *stop_time, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period,
                    const bool show_codes, const DumpMessage dump_message) {
    int depth = (max_depth <= 3) ? max_depth : 3;

    // arrival/departure in protobuff are as seconds from midnight in local time
    const auto offset = [&](const uint32_t time) {
        static const auto flag = std::numeric_limits<uint32_t>::max();
        return time == flag ? 0 : st.vehicle_journey->utc_to_local_offset();
    };
    stop_time->set_arrival_time(st.arrival_time + offset(st.arrival_time));
    stop_time->set_departure_time(st.departure_time + offset(st.departure_time));
    stop_time->set_headsign(data.pt_data->headsign_handler.get_headsign(st));

    stop_time->set_pickup_allowed(st.pick_up_allowed());
    stop_time->set_drop_off_allowed(st.drop_off_allowed());

    // TODO V2: the dump of the JPP is deprecated, but we keep it for retrocompatibility
    if (depth > 0) {
        const auto& jpp_idx = data.dataRaptor->jp_container.get_jpp(st);
        fill_pb_object(data.dataRaptor->jp_container.get_jpps()[jpp_idx.val], data,
                       stop_time->mutable_journey_pattern_point(), depth-1,
                       now, action_period, show_codes);
    }

    // we always dump the stop point (with the same depth)
    fill_pb_object(st.stop_point, data,
                   stop_time->mutable_stop_point(), depth,
                   now, action_period, show_codes, dump_message);

    if (st.vehicle_journey != nullptr && depth > 0) {
        fill_pb_object(st.vehicle_journey, data,
                       stop_time->mutable_vehicle_journey(), depth-1, now,
                       action_period, show_codes, dump_message);
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


void fill_pb_object(const std::pair<const routing::JppIdx, const routing::JourneyPatternPoint&>& jpp,
                    const nt::Data& data,
                    pbnavitia::JourneyPatternPoint * journey_pattern_point,
                    int max_depth,
                    const pt::ptime& now,
                    const pt::time_period& action_period,
                    const bool show_codes){
    int depth = (max_depth <= 3) ? max_depth : 3;

    journey_pattern_point->set_uri(data.dataRaptor->jp_container.get_id(jpp.first));
    journey_pattern_point->set_order(jpp.second.order);

    if(depth > 0){
        fill_pb_object(data.pt_data->stop_points[jpp.second.sp_idx.val], data,
                       journey_pattern_point->mutable_stop_point(),
                       depth-1, now, action_period, show_codes);
        fill_pb_object(data.dataRaptor->jp_container.get_jps()[jpp.second.jp_idx.val], data,
                       journey_pattern_point->mutable_journey_pattern(),
                       depth - 1, now, action_period, show_codes);
    }
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

            //and we create a new one
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
    path_item->set_length(item.get_length(ori_dest.streetnetwork_params.speed_factor));
    path_item->set_duration(item.duration.total_seconds());
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
                    const bool, const DumpMessage){
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
        && stop_time->vehicle_journey->route
        && stop_time->vehicle_journey->route->destination
        && ! stop_time->vehicle_journey->stop_time_list.empty()
        && stop_time->vehicle_journey->stop_time_list.back().stop_point
        && stop_time->vehicle_journey->route->destination
           != stop_time->vehicle_journey->stop_time_list.back().stop_point->stop_area;
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
        auto sa = stop_time->vehicle_journey->stop_time_list.back().stop_point->stop_area;
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
    fill_messages(r, data, pt_display_info, max_depth-1, now, action_period);

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

        fill_messages(r->line, data, pt_display_info, max_depth-1, now, action_period);
        uris->set_line(r->line->uri);
        if (r->line->network != nullptr){
            pt_display_info->set_network(r->line->network->name);
            uris->set_network(r->line->network->uri);

            fill_messages(r->line->network, data, pt_display_info, max_depth-1, now, action_period);
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
                    const nt::StopTime* origin,
                    const nt::StopTime* destination,
                    int max_depth,
                    const pt::ptime& now,
                    const pt::time_period& action_period)
{
    if(vj == nullptr)
        return ;
    pbnavitia::Uris* uris = pt_display_info->mutable_uris();
    uris->set_vehicle_journey(vj->uri);
    if (max_depth > 0 && vj->route) {
        fill_pb_object(vj->route, data, pt_display_info,max_depth,now,action_period);
        uris->set_route(vj->route->uri);
        const auto& jp_idx = data.dataRaptor->jp_container.get_jp_from_vj()[routing::VjIdx(*vj)];
        uris->set_journey_pattern(data.dataRaptor->jp_container.get_id(jp_idx));
    }

    fill_messages(vj->meta_vj, data, pt_display_info, max_depth-1, now, action_period);
    if (origin != nullptr) {
        pt_display_info->set_headsign(data.pt_data->headsign_handler.get_headsign(*origin));
    }
    pt_display_info->set_direction(vj->get_direction());
    if (vj->physical_mode != nullptr) {
        pt_display_info->set_physical_mode(vj->physical_mode->name);
        uris->set_physical_mode(vj->physical_mode->uri);
    }
    pt_display_info->set_description(vj->odt_message);
    pbnavitia::hasEquipments* has_equipments = pt_display_info->mutable_has_equipments();
    if (origin && destination) {
        fill_pb_object(vj, data, has_equipments, origin->stop_point,
                       destination->stop_point,  max_depth-1, now,
                       action_period);
    } else {
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

void fill_pb_object(const nt::Calendar* cal, const nt::Data& data,
                    pbnavitia::Calendar* pb_cal, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period, const bool, const DumpMessage)
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
    if (vehicle_journey && vehicle_journey->physical_mode) {
        fill_co2_emission_by_mode(pb_section, data, vehicle_journey->physical_mode->uri);
    }
}

}//namespace navitia
