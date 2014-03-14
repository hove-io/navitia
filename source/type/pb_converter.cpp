#include "pb_converter.h"
#include "georef/georef.h"
#include "georef/street_network.h"
#include "utils/exception.h"
#include "utils/exception.h"
#include <functional>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/date_defs.hpp>

namespace nt = navitia::type;
namespace pt = boost::posix_time;

namespace navitia{

void fill_pb_object(navitia::georef::Admin* adm, const nt::Data&,
                    pbnavitia::AdministrativeRegion* admin, int,
                    const pt::ptime&, const pt::time_period& ){
    if(adm == nullptr)
        return ;
    admin->set_name(adm->name);
    admin->set_uri(adm->uri);
    if(adm->post_code != "")
        admin->set_zip_code(adm->post_code);
    admin->set_level(adm->level);
    if(adm->coord.is_initialized()) {
        admin->mutable_coord()->set_lat(adm->coord.lat());
        admin->mutable_coord()->set_lon(adm->coord.lon());
    }
}


void fill_pb_object(const nt::StopArea * sa,
                    const nt::Data& data, pbnavitia::StopArea* stop_area,
                    int max_depth, const pt::ptime& now,
                    const pt::time_period& action_period){
    if(sa == nullptr)
        return ;
    int depth = (max_depth <= 3) ? max_depth : 3;
    stop_area->set_uri(sa->uri);
    stop_area->set_name(sa->name);
    if(!sa->comment.empty()) {
        stop_area->set_comment(sa->comment);
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

    for(const auto message : sa->get_applicable_messages(now, action_period)){
        fill_message(message, data, stop_area->add_messages(), max_depth-1, now, action_period);
    }
}


void fill_pb_object(const nt::StopPoint* sp, const nt::Data& data,
                    pbnavitia::StopPoint* stop_point, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period){
    if(sp == nullptr)
        return ;
    int depth = (max_depth <= 3) ? max_depth : 3;
    stop_point->set_uri(sp->uri);
    stop_point->set_name(sp->name);
    if(!sp->comment.empty()) {
        stop_point->set_comment(sp->comment);
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

    if(depth > 0 && sp->stop_area != nullptr)
        fill_pb_object(sp->stop_area, data, stop_point->mutable_stop_area(),
                       depth-1, now, action_period);


    for(const auto message : sp->get_applicable_messages(now, action_period)){
        fill_message(message, data, stop_point->add_messages(), max_depth-1, now, action_period);
    }
}


void fill_pb_object(navitia::georef::Way* way, const nt::Data& data,
                    pbnavitia::Address * address, int house_number,
                    type::GeographicalCoord& coord, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period){
    if(way == nullptr)
        return ;
    int depth = (max_depth <= 3) ? max_depth : 3;
    address->set_name(way->name);
    if(house_number >= 0){
        address->set_house_number(house_number);
    }
    if(coord.is_initialized()) {
        address->mutable_coord()->set_lon(coord.lon());
        address->mutable_coord()->set_lat(coord.lat());
    }
    address->set_uri(way->uri);

    if(depth > 0){
        for(georef::Admin* admin : way->admin_list){
            fill_pb_object(admin, data,  address->add_administrative_regions(),
                           depth-1, now, action_period);
        }
    }
}


void fill_pb_object(nt::Line const* l, const nt::Data& data,
        pbnavitia::Line * line, int max_depth, const pt::ptime& now,
        const pt::time_period& action_period){
    if(l == nullptr)
        return ;

    int depth = (max_depth <= 3) ? max_depth : 3;
    if(!l->comment.empty()) {
        line->set_comment(l->comment);
    }
    if(l->code != "")
        line->set_code(l->code);
    if(l->color != "")
        line->set_color(l->color);
    line->set_name(l->name);
    line->set_uri(l->uri);

    if(depth>0){
        std::vector<nt::idx_t> physical_mode_idxes;
        for(auto route : l->route_list) {
            fill_pb_object(route, data, line->add_routes(), depth-1);
        }
        for(auto physical_mode : l->physical_mode_list){
            fill_pb_object(physical_mode, data, line->add_physical_mode(),
                    depth-1);
        }

        fill_pb_object(l->commercial_mode, data,
                line->mutable_commercial_mode(), depth-1);
        fill_pb_object(l->network, data, line->mutable_network(), depth-1);
    }
    for(const auto message : l->get_applicable_messages(now, action_period)){
        fill_message(message, data, line->add_messages(), depth-1, now, action_period);
    }
}


void fill_pb_object(const nt::JourneyPattern* jp, const nt::Data& data,
        pbnavitia::JourneyPattern * journey_pattern, int max_depth,
        const pt::ptime& now, const pt::time_period& action_period){
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


void fill_pb_object(const nt::Route* r, const nt::Data& data,
                    pbnavitia::Route* route, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period){
    if(r == nullptr)
        return ;
    int depth = (max_depth <= 3) ? max_depth : 3;

    route->set_name(r->name);
    route->set_uri(r->uri);
    if(depth > 0 && r->line != nullptr) {
        fill_pb_object(r->line, data, route->mutable_line(), depth-1,
                       now, action_period);
    }
    for(const auto& message : r->get_applicable_messages(now, action_period)){
        fill_message(message, data, route->add_messages(), max_depth-1, now, action_period);
    }
}


void fill_pb_object(const nt::Network* n, const nt::Data& data,
                    pbnavitia::Network* network, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period){
    if(n == nullptr)
        return ;

    network->set_name(n->name);
    network->set_uri(n->uri);

    for(const auto& message : n->get_applicable_messages(now, action_period)){
        fill_message(message, data, network->add_messages(), max_depth-1, now, action_period);
    }
}


void fill_pb_object(const nt::CommercialMode* m, const nt::Data&,
                    pbnavitia::CommercialMode* commercial_mode,
                    int, const pt::ptime&, const pt::time_period&){
    if(m == nullptr)
        return ;

    commercial_mode->set_name(m->name);
    commercial_mode->set_uri(m->uri);
}


void fill_pb_object(const nt::PhysicalMode* m, const nt::Data&,
                    pbnavitia::PhysicalMode* physical_mode, int,
                    const pt::ptime&, const pt::time_period&){
    if(m == nullptr)
        return ;

    physical_mode->set_name(m->name);
    physical_mode->set_uri(m->uri);
}


void fill_pb_object(const nt::Company* c, const nt::Data&,
                    pbnavitia::Company* company, int, const pt::ptime&,
                    const pt::time_period&){
    if(c == nullptr)
        return ;

    company->set_name(c->name);
    company->set_uri(c->uri);
}


void fill_pb_object(const nt::StopPointConnection* c, const nt::Data& data,
                    pbnavitia::Connection* connection, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period){
    if(c == nullptr)
        return ;
    int depth = (max_depth <= 3) ? max_depth : 3;

    connection->set_seconds(c->display_duration);
    if(c->departure != nullptr && c->destination != nullptr && depth > 0){
        fill_pb_object(c->departure, data, connection->mutable_origin(),
                       depth-1, now, action_period);
        fill_pb_object(c->destination, data, connection->mutable_destination(),
                       depth-1, now, action_period);
    }
}


void fill_pb_object(const nt::JourneyPatternPointConnection*, const nt::Data&,
                    pbnavitia::Connection* , int,
                    const pt::ptime&, const pt::time_period&){
}


void fill_pb_object(const nt::ValidityPattern* vp, const nt::Data&,
                    pbnavitia::ValidityPattern* validity_pattern, int,
                    const pt::ptime&, const pt::time_period&){
    if(vp == nullptr)
        return;
    auto vp_string = boost::gregorian::to_iso_string(vp->beginning_date);
    validity_pattern->set_beginning_date(vp_string);
    validity_pattern->set_days(vp->days.to_string());
}


pbnavitia::VehicleJourneyType get_pb_odt_type(const navitia::type::VehicleJourneyType vehicle_journey_type){
    pbnavitia::VehicleJourneyType result = pbnavitia::VehicleJourneyType::regular;
    switch(vehicle_journey_type){
        case type::VehicleJourneyType::virtual_with_stop_time:
            result = pbnavitia::VehicleJourneyType::virtual_with_stop_time;
            break;
    case type::VehicleJourneyType::virtual_without_stop_time:
        result = pbnavitia::VehicleJourneyType::virtual_without_stop_time;
            break;
    case type::VehicleJourneyType::stop_point_to_stop_point:
        result = pbnavitia::VehicleJourneyType::stop_point_to_stop_point;
            break;
    case type::VehicleJourneyType::adress_to_stop_point:
        result = pbnavitia::VehicleJourneyType::address_to_stop_point;
            break;
    case type::VehicleJourneyType::odt_point_to_point:
        result = pbnavitia::VehicleJourneyType::odt_point_to_point;
            break;
    default :
        result = pbnavitia::VehicleJourneyType::regular;
    }
    return result;
}


void fill_pb_object(const nt::VehicleJourney* vj, const nt::Data& data,
                    pbnavitia::VehicleJourney * vehicle_journey, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period){
    if(vj == nullptr)
        return ;
    int depth = (max_depth <= 3) ? max_depth : 3;

    vehicle_journey->set_name(vj->name);
    vehicle_journey->set_uri(vj->uri);
    if(!vj->comment.empty()) {
        vehicle_journey->set_comment(vj->comment);
    }
    vehicle_journey->set_odt_message(vj->odt_message);
    vehicle_journey->set_is_adapted(vj->is_adapted);
    vehicle_journey->set_vehicle_journey_type(get_pb_odt_type(vj->vehicle_journey_type));

    vehicle_journey->set_wheelchair_accessible(vj->wheelchair_accessible());
    vehicle_journey->set_bike_accepted(vj->bike_accepted());
    vehicle_journey->set_air_conditioned(vj->air_conditioned());
    vehicle_journey->set_visual_announcement(vj->visual_announcement());
    vehicle_journey->set_appropriate_escort(vj->appropriate_escort());
    vehicle_journey->set_appropriate_signage(vj->appropriate_signage());
    vehicle_journey->set_school_vehicle(vj->school_vehicle());

    if(vj->journey_pattern!= nullptr && depth > 0) {
        fill_pb_object(vj->journey_pattern, data,
                       vehicle_journey->mutable_journey_pattern(), depth-1,
                       now, action_period);
    }
    if(depth > 0) {
        for(auto* stop_time : vj->stop_time_list) {
            fill_pb_object(stop_time, data, vehicle_journey->add_stop_times(),
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
    }
    if(depth > 0) {
        fill_pb_object(vj->validity_pattern, data, vehicle_journey->mutable_validity_pattern(), max_depth-1);
        fill_pb_object(vj->adapted_validity_pattern, data, vehicle_journey->mutable_adapted_validity_pattern(), max_depth-1);
    }

    for(auto message : vj->get_applicable_messages(now, action_period)){
        fill_message(message, data, vehicle_journey->add_messages(), max_depth-1, now, action_period);
    }

    //si on a un vj théorique rataché à notre vj, on récupére les messages qui le concerne
    if(vj->theoric_vehicle_journey != nullptr){
        for(auto message : vj->theoric_vehicle_journey->get_applicable_messages(now, action_period)){
            fill_message(message, data, vehicle_journey->add_messages(), max_depth-1, now, action_period);
        }
    }
}


void fill_pb_object(const nt::StopTime* st, const type::Data &data,
                    pbnavitia::StopTime *stop_time, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period){
    if(st == nullptr)
        return ;
    int depth = (max_depth <= 3) ? max_depth : 3;
    auto p = boost::posix_time::seconds(st->arrival_time);

    stop_time->set_arrival_time(navitia::to_iso_string_no_fractional(p));

    p = boost::posix_time::seconds(st->departure_time);
    stop_time->set_departure_time(navitia::to_iso_string_no_fractional(p));
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


void fill_pb_object(const nt::StopTime* st, const type::Data&,
                    pbnavitia::StopDateTime * stop_date_time, int ,
                    const pt::ptime& , const pt::time_period& ) {
    if(st == nullptr)
        return ;

    pbnavitia::Properties * hp = stop_date_time->mutable_properties();
    if ((!st->drop_off_allowed()) && st->pick_up_allowed()){
        hp->add_additional_informations(pbnavitia::Properties::pick_up_only);
    }
    if(st->drop_off_allowed() && (!st->pick_up_allowed())){
        hp->add_additional_informations(pbnavitia::Properties::drop_off_only);
    }
    if (st->odt()){
        hp->add_additional_informations(pbnavitia::Properties::on_demand_transport);
    }
    if (st->date_time_estimated()){
        hp->add_additional_informations(pbnavitia::Properties::date_time_estimated);
    }
    if(!st->comment.empty()){
        pbnavitia::Note* note = hp->add_notes();
        std::hash<std::string> hash_fn;
        note->set_uri("note:"+std::to_string(hash_fn(st->comment)));
        note->set_note(st->comment);
    }
}


void fill_pb_object(const nt::JourneyPatternPoint* jpp, const nt::Data& data,
                    pbnavitia::JourneyPatternPoint * journey_pattern_point,
                    int max_depth, const pt::ptime& now,
                    const pt::time_period& action_period){
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
                           depth - 1, now, action_period);
        }
    }
}


void fill_pb_placemark(const type::StopPoint* stop_point,
                       const type::Data &data, pbnavitia::Place* place,
                       int max_depth, const pt::ptime& now,
                       const pt::time_period& action_period){
    if(stop_point == nullptr)
        return;
    int depth = (max_depth <= 3) ? max_depth : 3;
    fill_pb_object(stop_point, data, place->mutable_stop_point(), depth,
                   now, action_period);
    place->set_name(stop_point->name);

    for(auto admin : place->stop_point().administrative_regions()) {
        if (admin.level() == 8){
            place->set_name(place->name() + ", " + admin.name());
        }
    }
    place->set_uri(stop_point->uri);
    place->set_embedded_type(pbnavitia::STOP_POINT);
}

void fill_pb_placemark(const type::StopArea* stop_area,
                       const type::Data &data, pbnavitia::Place* place,
                       int max_depth, const pt::ptime& now,
                       const pt::time_period& action_period){
    if(stop_area == nullptr)
        return;
    int depth = (max_depth <= 3) ? max_depth : 3;
    fill_pb_object(stop_area, data, place->mutable_stop_area(), depth,
                   now, action_period);
    place->set_name(stop_area->name);
    for(auto admin : place->stop_area().administrative_regions()) {
        if (admin.level() == 8){
            place->set_name(place->name() + ", " + admin.name());
        }
    }

    place->set_uri(stop_area->uri);
    place->set_embedded_type(pbnavitia::STOP_AREA);
}

void fill_pb_placemark(navitia::georef::Admin* admin,
                       const type::Data &data, pbnavitia::Place* place,
                       int max_depth, const pt::ptime& now,
                       const pt::time_period& action_period){
    if(admin == nullptr)
        return;
    int depth = (max_depth <= 3) ? max_depth : 3;
    fill_pb_object(admin, data, place->mutable_administrative_region(), depth,
                   now, action_period);
    place->set_name(admin->name);
    //If city contains multi postal code(37000;37100;37200), we show only the smallest one in the result.
    //"name": "Tours(37000;37100;37200)" becomes "name": "Tours(37000)"
    if (!admin->post_code.empty()){
        if (admin->post_code.find(";") != std::string::npos){
            std::vector<std::string> str_vec;
            boost::algorithm::split(str_vec, admin->post_code, boost::algorithm::is_any_of(";"));
            assert(!str_vec.empty());
            int min_value = std::numeric_limits<int>::max();
            for (const std::string &str_post_code : str_vec){
                int int_post_code;
                try{
                    int_post_code = boost::lexical_cast<int>(str_post_code);
                }
                catch (boost::bad_lexical_cast){
                    continue;
                }
                if (int_post_code < min_value)
                    min_value = int_post_code;
            }
            if (min_value != std::numeric_limits<int>::max()){
                place->set_name(place->name() + " (" + boost::lexical_cast<std::string>(min_value) + ")");
            }
            else{
                place->set_name(place->name() + " ()");
            }

        }
        else{
            place->set_name(place->name() + " (" + admin->post_code + ")");
        }
    }

    place->set_uri(admin->uri);
    place->set_embedded_type(pbnavitia::ADMINISTRATIVE_REGION);
}

void fill_pb_placemark(navitia::georef::POI* poi,
                       const type::Data &data, pbnavitia::Place* place,
                       int max_depth, const pt::ptime& now,
                       const pt::time_period& action_period){
    if(poi == nullptr)
        return;
    int depth = (max_depth <= 3) ? max_depth : 3;
    fill_pb_object(poi, data, place->mutable_poi(), depth,
                   now, action_period);
    place->set_name(poi->name);
    for(auto admin : place->poi().administrative_regions()) {
        if (admin.level() == 8){
            place->set_name(place->name() + ", " + admin.name());
        }
    }

    place->set_uri(poi->uri);
    place->set_embedded_type(pbnavitia::POI);
}

void fill_pb_placemark(navitia::georef::Way* way,
                       const type::Data &data, pbnavitia::Place* place,
                       int house_number,
                       type::GeographicalCoord& coord,
                       int max_depth, const pt::ptime& now,
                       const pt::time_period& action_period){
    if(way == nullptr)
        return;
    int depth = (max_depth <= 3) ? max_depth : 3;
    fill_pb_object(way, data, place->mutable_address(),
                   house_number, coord , depth,
                   now, action_period);
    if(place->address().has_house_number()) {
        int house_number = place->address().house_number();
        auto str_house_number = std::to_string(house_number) + " ";
        place->set_name(str_house_number);
    }
    auto str_street_name = place->address().name();
    place->set_name(place->name() + str_street_name);
    for(auto admin : place->address().administrative_regions()) {
        if (admin.level() == 8){
            if (admin.zip_code()!=""){
                place->set_name(place->name() + ", " + admin.zip_code() + " " + admin.name());
            }else{
                place->set_name(place->name() + ", " + admin.name());
            }


        }
    }

    place->set_uri(place->address().uri());
    place->set_embedded_type(pbnavitia::ADDRESS);
}

void fill_fare_section(EnhancedResponse& enhanced_response, pbnavitia::Journey* pb_journey, const fare::results& fare) {
    auto pb_fare = pb_journey->mutable_fare();

    size_t cpt_ticket = enhanced_response.response.tickets_size();

    boost::optional<std::string> currency;
    for (const fare::Ticket& ticket : fare.tickets) {
        if (! currency)
            currency = ticket.currency;
        if (ticket.currency != *currency)
            throw navitia::exception("cannot have different currencies for tickets"); //if we really had to handle different currencies it could be done, but I don't see the point

        pbnavitia::Ticket* pb_ticket = nullptr;
        if (ticket.is_default_ticket()) {
            if (! enhanced_response.unkown_ticket) {
                pb_ticket = enhanced_response.response.add_tickets();
                pb_ticket->set_name(ticket.key);
                pb_ticket->set_id("unknown_ticket");
                enhanced_response.unkown_ticket = pb_ticket;
                pb_fare->add_ticket_id(pb_ticket->id());
            }
            else {
                pb_ticket = enhanced_response.unkown_ticket;
            }
        }
        else {
            pb_ticket = enhanced_response.response.add_tickets();

            pb_ticket->set_name(ticket.key);
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

void finalize_section(pbnavitia::Section* section, const navitia::georef::PathItem& last_item,
                      const navitia::type::Data& data, const boost::posix_time::ptime departure,
                      int depth, const pt::ptime& now, const pt::time_period& action_period) {

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

    section->set_begin_date_time(navitia::to_iso_string_no_fractional(departure));
    section->set_end_date_time(navitia::to_iso_string_no_fractional(departure + bt::seconds(total_duration)));

    //add the destination as a placemark
    pbnavitia::Place* dest_place = section->mutable_destination();
    auto way = data.geo_ref.ways[last_item.way_idx];
    type::GeographicalCoord coord = last_item.coordinates.back();
    fill_pb_placemark(way, data, dest_place, way->nearest_number(coord), coord,
                            depth, now, action_period);

    switch (last_item.transportation) {
    case georef::PathItem::TransportCaracteristic::Walk:
        section->mutable_street_network()->set_mode(pbnavitia::Walking);
        break;
    case georef::PathItem::TransportCaracteristic::Bike:
        section->mutable_street_network()->set_mode(pbnavitia::Bike);
        break;
    case georef::PathItem::TransportCaracteristic::Car:
        section->mutable_street_network()->set_mode(pbnavitia::Car);
        break;
    case georef::PathItem::TransportCaracteristic::BssTake:
        section->set_type(pbnavitia::boarding);
        break;
    case georef::PathItem::TransportCaracteristic::BssPutBack:
        section->set_type(pbnavitia::landing);
        break;
    default:
        throw navitia::exception("Unhandled TransportCaracteristic value in pb_converter");
    }
}

pbnavitia::Section* create_section(EnhancedResponse& response, pbnavitia::Journey* pb_journey, const navitia::georef::PathItem& first_item,
                                           const navitia::type::Data& data,
                                           int depth, const pt::ptime& now, const pt::time_period& action_period) {

    auto section = pb_journey->add_sections();
    section->set_id(response.register_section(first_item));
    section->set_type(pbnavitia::STREET_NETWORK);

    pbnavitia::Place* orig_place = section->mutable_origin();
    auto way = data.geo_ref.ways[first_item.way_idx];
    type::GeographicalCoord departure_coord = first_item.coordinates.front();
    fill_pb_placemark(way, data, orig_place, way->nearest_number(departure_coord), departure_coord,
                      depth, now, action_period);

    return section;
}

void fill_street_sections(EnhancedResponse& response, const type::EntryPoint& ori_dest,
                            const georef::Path& path, const type::Data& data,
                            pbnavitia::Journey* pb_journey, const boost::posix_time::ptime departure,
                            int max_depth, const pt::ptime& now,
                            const pt::time_period& action_period) {
    int depth = std::min(max_depth, 3);
    if (path.path_items.empty())
        return;

    auto session_departure = departure;

    boost::optional<georef::PathItem::TransportCaracteristic> last_transportation_carac = {};
    auto section = create_section(response, pb_journey, path.path_items.front(), data, depth, now, action_period);
    georef::PathItem last_item;

    //we create 1 section by mean of transport
    for (auto item : path.path_items) {
        auto transport_carac = item.transportation;

        if (last_transportation_carac && transport_carac != *last_transportation_carac) {
            //we end the last section
            finalize_section(section, last_item, data, session_departure, depth, now, action_period);
            session_departure += bt::seconds(section->duration());

            //and be create a new one
            section = create_section(response, pb_journey, item, data, depth, now, action_period);
        }

        add_path_item(section->mutable_street_network(), item, ori_dest, data);

        last_transportation_carac = transport_carac;
        last_item = item;
    }

    finalize_section(section, path.path_items.back(), data, session_departure, depth, now, action_period);
}


void fill_message(const boost::shared_ptr<type::Message> message,
        const type::Data&, pbnavitia::Message* pb_message, int,
        const boost::posix_time::ptime&, const boost::posix_time::time_period&){
    pb_message->set_uri(message->uri);
    switch(message->message_status){
        case type::MessageStatus::information:
            pb_message->set_message_status(pbnavitia::MessageStatus::information);
            break;
        case type::MessageStatus::warning:
            pb_message->set_message_status(pbnavitia::MessageStatus::warning);
            break;
        case type::MessageStatus::disrupt:
            pb_message->set_message_status(pbnavitia::MessageStatus::disrupt);
            break;
    }
    auto it = message->localized_messages.find("fr");
    if(it !=  message->localized_messages.end()){
        pb_message->set_message(it->second.body);
        pb_message->set_title(it->second.title);
    }

    pb_message->set_start_application_date(navitia::to_iso_string_no_fractional((message->application_period).begin()));
    pb_message->set_end_application_date(navitia::to_iso_string_no_fractional((message->application_period).end()));

    pb_message->set_start_application_daily_hour(navitia::to_iso_string_no_fractional(message->application_daily_start_hour));
    pb_message->set_end_application_daily_hour(navitia::to_iso_string_no_fractional(message->application_daily_end_hour));
}


void add_path_item(pbnavitia::StreetNetwork* sn, const navitia::georef::PathItem& item,
                    const type::EntryPoint &ori_dest, const navitia::type::Data& data) {
    if(item.way_idx >= data.geo_ref.ways.size())
        throw navitia::exception("Wrong way idx : " + boost::lexical_cast<std::string>(item.way_idx));

    pbnavitia::PathItem* path_item = sn->add_path_items();
    path_item->set_name(data.geo_ref.ways[item.way_idx]->name);
    path_item->set_length(item.get_length());
    path_item->set_duration(item.duration.total_seconds() / ori_dest.streetnetwork_params.speed_factor);
    path_item->set_direction(item.angle);

    //we add each path item coordinate to the global coordinate list
    for(auto coord : item.coordinates) {
        if(coord.is_initialized()) {
            pbnavitia::GeographicalCoord * pb_coord = sn->add_coordinates();
            pb_coord->set_lon(coord.lon());
            pb_coord->set_lat(coord.lat());
        }
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
                    const pt::ptime& now, const pt::time_period& action_period){
    if(geopoi == nullptr)
        return;
    int depth = (max_depth <= 3) ? max_depth : 3;

    poi->set_name(geopoi->name);
    poi->set_uri(geopoi->uri);
    if(geopoi->coord.is_initialized()) {
        poi->mutable_coord()->set_lat(geopoi->coord.lat());
        poi->mutable_coord()->set_lon(geopoi->coord.lon());
    }

    if(depth > 0){
        fill_pb_object(data.geo_ref.poitypes[geopoi->poitype_idx], data,
                       poi->mutable_poi_type(), depth-1,
                       now, action_period);
        for(georef::Admin * admin : geopoi->admin_list){
            fill_pb_object(admin, data,  poi->add_administrative_regions(),
                           depth-1, now, action_period);
        }
    }
}

pbnavitia::ExceptionType get_pb_exception_type(const navitia::type::ExceptionDate::ExceptionType exception_type){
    switch (exception_type) {
    case nt::ExceptionDate::ExceptionType::add:
        return pbnavitia::Add;
    case nt::ExceptionDate::ExceptionType::sub:
        return pbnavitia::Remove;
    default:
        throw navitia::exception("exception date case not handled");
    } 
}

void fill_pb_object(const navitia::type::StopTime* stop_time,
                    const nt::Data& data,
                    pbnavitia::ScheduleStopTime* rs_date_time, int max_depth,
                    const boost::posix_time::ptime& now,
                    const boost::posix_time::time_period& action_period,
                    const DateTime& date_time, boost::optional<const std::string> calendar_id){
    if (stop_time == nullptr) {
        rs_date_time->set_date_time("");
        return;
    }
    std::string str_datetime;
    if (!calendar_id) {
        str_datetime = iso_string(date_time, data);
    } else {
        str_datetime = iso_hour_string(date_time, data);
    }
    rs_date_time->set_date_time(str_datetime);
    pbnavitia::Properties * hn = rs_date_time->mutable_properties();
    if ((!stop_time->drop_off_allowed()) && stop_time->pick_up_allowed()){
        hn->add_additional_informations(pbnavitia::Properties::pick_up_only);
    }
    if (stop_time->drop_off_allowed() && (!stop_time->pick_up_allowed())){
        hn->add_additional_informations(pbnavitia::Properties::drop_off_only);
    }
    if (stop_time->odt()){
        hn->add_additional_informations(pbnavitia::Properties::on_demand_transport);
    }
    if (stop_time->date_time_estimated()){
        hn->add_additional_informations(pbnavitia::Properties::date_time_estimated);
    }
    if (!stop_time->comment.empty()){
        pbnavitia::Note* note = hn->add_notes();
        std::hash<std::string> hash_fn;
        note->set_uri("note:"+std::to_string(hash_fn(stop_time->comment)));
        note->set_note(stop_time->comment);
    }
    if (stop_time->vehicle_journey != nullptr) {
        if(!stop_time->vehicle_journey->odt_message.empty()){
            pbnavitia::Note* note = hn->add_notes();
            std::hash<std::string> hash_fn;
            note->set_uri("note:"+std::to_string(hash_fn(stop_time->vehicle_journey->odt_message)));
            note->set_note(stop_time->vehicle_journey->odt_message);
        }
    }

    if((calendar_id) && (stop_time->vehicle_journey != nullptr)) {
        auto asso_cal_it = stop_time->vehicle_journey->associated_calendars.find(*calendar_id);
        if(asso_cal_it != stop_time->vehicle_journey->associated_calendars.end()){
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
    for(auto message : r->get_applicable_messages(now, action_period)){
        fill_message(message, data, pt_display_info->add_messages(), max_depth-1, now, action_period);
    }
    pt_display_info->set_direction(r->name);
    if (r->line != nullptr){
        pt_display_info->set_color(r->line->color);
        pt_display_info->set_code(r->line->code);
        pt_display_info->set_name(r->line->name);
        for(auto message : r->line->get_applicable_messages(now, action_period)){
            fill_message(message, data, pt_display_info->add_messages(), max_depth-1, now, action_period);
        }
        uris->set_line(r->line->uri);
        if (r->line->network != nullptr){
            pt_display_info->set_network(r->line->network->name);
            uris->set_network(r->line->network->uri);
            for(auto message : r->line->network->get_applicable_messages(now, action_period)){
                fill_message(message, data, pt_display_info->add_messages(), max_depth-1, now, action_period);
            }
        }
        if (r->line->commercial_mode != nullptr){
            pt_display_info->set_commercial_mode(r->line->commercial_mode->name);
            uris->set_commercial_mode(r->line->commercial_mode->uri);
        }
    }

}

void fill_pb_object(const nt::VehicleJourney* vj, const nt::Data& data,
                    pbnavitia::PtDisplayInfo* pt_display_info, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period)
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
        fill_message(message, data, pt_display_info->add_messages(), max_depth-1, now, action_period);
    }
    pt_display_info->set_headsign(vj->name);
    pt_display_info->set_direction(vj->get_direction());
    if ((vj->journey_pattern != nullptr) && (vj->journey_pattern->physical_mode != nullptr)){
        pt_display_info->set_physical_mode(vj->journey_pattern->physical_mode->name);
        uris->set_physical_mode(vj->journey_pattern->physical_mode->uri);
    }
    pt_display_info->set_description(vj->odt_message);
    pt_display_info->set_vehicle_journey_type(get_pb_odt_type(vj->vehicle_journey_type));

    pbnavitia::hasEquipments* has_equipments = pt_display_info->mutable_has_equipments();
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
                    const pt::ptime& now, const pt::time_period& action_period)
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

void fill_pb_object(const nt::VehicleJourney* vj, const nt::Data& ,
                    pbnavitia::addInfoVehicleJourney * add_info_vehicle_journey, int ,
                    const pt::ptime& , const pt::time_period& )
{
    if(vj == nullptr)
        return ;
    add_info_vehicle_journey->set_vehicle_journey_type(get_pb_odt_type(vj->vehicle_journey_type));
    add_info_vehicle_journey->set_has_date_time_estimated(vj->has_date_time_estimated());
}


void fill_pb_error(const pbnavitia::Error::error_id id, const std::string& message,
                    pbnavitia::Error* error, int ,
                    const boost::posix_time::ptime& , const boost::posix_time::time_period& ){
    error->set_id(id);
    error->set_message(message);
}
}//namespace navitia
