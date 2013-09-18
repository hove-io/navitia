#include "pb_converter.h"
#include "georef/georef.h"
#include "georef/street_network.h"
#include "boost/lexical_cast.hpp"

namespace nt = navitia::type;
namespace pt = boost::posix_time;

namespace navitia{

void fill_pb_object(navitia::georef::Admin* adm, const nt::Data&,
                    pbnavitia::AdministrativeRegion* admin, int,
                    const pt::ptime&, const pt::time_period& ){
//    navitia::georef::Admin* adm = data.geo_ref.admins.at(idx);
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
    if(sp->coord.is_initialized()) {
        stop_point->mutable_coord()->set_lon(sp->coord.lon());
        stop_point->mutable_coord()->set_lat(sp->coord.lat());
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
        for(const auto message : l->get_applicable_messages(now, action_period)){
            fill_message(message, data, line->add_messages(), depth-1, now, action_period);
        }
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
    if(depth > 0 && jp->route != nullptr)
        fill_pb_object(jp->route, data, journey_pattern->mutable_route(),
                depth-1, now, action_period);

    if(depth > 0) {
        /*auto messages = data.pt_data.message_holder
            .find_messages(jp->uri, now, action_period);
        for(const auto& message : messages){
            fill_message(message, data, journey_pattern->add_messages(),
                    depth-1, now, action_period);
        }*/
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


void fill_pb_object(const nt::Network* n, const nt::Data&,
                    pbnavitia::Network* network, int,
                    const pt::ptime&, const pt::time_period&){
    if(n == nullptr)
        return ;

    network->set_name(n->name);
    network->set_uri(n->uri);
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

    connection->set_seconds(c->duration);
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


pbnavitia::OdtType get_pb_odt_type(const navitia::type::OdtType odt_type){
    pbnavitia::OdtType result = pbnavitia::OdtType::regular_line;
    switch(odt_type){
        case type::OdtType::virtual_with_stop_time:
            result = pbnavitia::OdtType::virtual_with_stop_time;
            break;
    case type::OdtType::virtual_without_stop_time:
        result = pbnavitia::OdtType::virtual_without_stop_time;
            break;
    case type::OdtType::stop_point_to_stop_point:
        result = pbnavitia::OdtType::stop_point_to_stop_point;
            break;
    case type::OdtType::adress_to_stop_point:
        result = pbnavitia::OdtType::adress_to_stop_point;
            break;
    case type::OdtType::odt_point_to_point:
        result = pbnavitia::OdtType::odt_point_to_point;
            break;
    default :
        result = pbnavitia::OdtType::regular_line;
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
    vehicle_journey->set_odt_message(vj->odt_message);
    vehicle_journey->set_is_adapted(vj->is_adapted);
    vehicle_journey->set_odt_type(get_pb_odt_type(vj->odt_type));

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
        fill_pb_object(vj->physical_mode, data,
                       vehicle_journey->mutable_physical_mode(), depth-1,
                       now, action_period);
        fill_pb_object(vj->validity_pattern, data,
                       vehicle_journey->mutable_validity_pattern(),
                       depth-1);
        fill_pb_object(vj->adapted_validity_pattern, data,
                       vehicle_journey->mutable_adapted_validity_pattern(),
                       depth-1);
    }
    if(depth > 0) {
        fill_pb_object(vj->physical_mode, data, vehicle_journey->mutable_physical_mode(), max_depth-1, now, action_period);

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

    stop_time->set_arrival_time(boost::posix_time::to_iso_string(p));

    p = boost::posix_time::seconds(st->departure_time);
    stop_time->set_departure_time(boost::posix_time::to_iso_string(p));
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

    pbnavitia::hasPropertie * hp = stop_date_time->mutable_has_properties();
    if ((!st->drop_off_allowed()) && st->pick_up_allowed()){
        hp->add_additional_informations(pbnavitia::hasPropertie::PICK_UP_ONLY);
    }
    if(st->drop_off_allowed() && (!st->pick_up_allowed())){
        hp->add_additional_informations(pbnavitia::hasPropertie::DROP_OFF_ONLY);
    }
    if (st->odt()){
        hp->add_additional_informations(pbnavitia::hasPropertie::ON_DEMAND_TRANSPORT);
    }
    if (st->date_time_estimated()){
        hp->add_additional_informations(pbnavitia::hasPropertie::DATE_TIME_ESTIMATED);
    }
    if(!st->comment.empty()){
        pbnavitia::Note* note = hp->add_notes();
        auto note_str = std::to_string(st->journey_pattern_point->idx)
                      + std::to_string(st->vehicle_journey->idx);
        note->set_uri("note:"+note_str);
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
    /*for(auto message : data.pt_data.message_holder.find_messages(jpp->uri, now, action_period)){
        fill_message(message, data, journey_pattern_point->add_messages(), max_depth-1, now, action_period);
    }*/
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
    place->set_uri(stop_point->uri);
    place->set_embedded_type(pbnavitia::STOP_POINT);
}


void fill_street_section(const type::EntryPoint &ori_dest,
                         const georef::Path &path, const type::Data &data,
                         pbnavitia::Section* section, int max_depth,
                         const pt::ptime& now,
                         const pt::time_period& action_period){
    int depth = (max_depth <= 3) ? max_depth : 3;
    if(path.path_items.size() > 0) {
        section->set_type(pbnavitia::STREET_NETWORK);
        pbnavitia::StreetNetwork * sn = section->mutable_street_network();
        create_pb(ori_dest, path, data, sn);
        section->set_duration(sn->length()/ori_dest.streetnetwork_params.speed);
        navitia::georef::Way* way;
        type::GeographicalCoord coord;
        if(path.path_items.size() > 0){
            pbnavitia::Place* orig_place = section->mutable_origin();
            way = data.geo_ref.ways[path.path_items.front().way_idx];
            coord = path.coordinates.front();
            orig_place = section->mutable_origin();
            fill_pb_object(way, data, orig_place->mutable_address(),
                           way->nearest_number(coord),coord , depth,
                           now, action_period);
            if(orig_place->address().has_house_number()) {
                int house_number = orig_place->address().house_number();
                auto str_house_number = std::to_string(house_number) + ", ";
                orig_place->set_name(str_house_number);
            }
            auto str_street_name = orig_place->address().name();
            orig_place->set_name(orig_place->name() + str_street_name);
            for(auto admin : orig_place->address().administrative_regions()) {
                orig_place->set_name(orig_place->name() + ", " + admin.name());
            }
            orig_place->set_uri(orig_place->address().uri());
            orig_place->set_embedded_type(pbnavitia::ADDRESS);
            pbnavitia::Place* dest_place = section->mutable_destination();
            way = data.geo_ref.ways[path.path_items.back().way_idx];
            coord = path.coordinates.back();
            dest_place = section->mutable_destination();
            fill_pb_object(way, data, dest_place->mutable_address(),
                           way->nearest_number(coord),coord , depth,
                           now, action_period);
            if(dest_place->address().has_house_number()) {
                int house_number = dest_place->address().house_number();
                auto str_house_number = std::to_string(house_number) + ", ";
                dest_place->set_name(str_house_number);
            }
            auto street_name = dest_place->address().name();
            dest_place->set_name(dest_place->name() + street_name);
            for(auto admin : dest_place->address().administrative_regions()) {
                dest_place->set_name(dest_place->name() + ", " + admin.name());
            }
            dest_place->set_uri(dest_place->address().uri());
            dest_place->set_embedded_type(pbnavitia::ADDRESS);
        }
    }
}

void fill_message(const boost::shared_ptr<type::Message> message,
        const type::Data&, pbnavitia::Message* pb_message, int,
        const boost::posix_time::ptime&, const boost::posix_time::time_period&){
    pb_message->set_uri(message->uri);
    auto it = message->localized_messages.find("fr");
    if(it !=  message->localized_messages.end()){
        pb_message->set_message(it->second.body);
        pb_message->set_title(it->second.title);
    }
}


void create_pb(const type::EntryPoint &ori_dest,
               const navitia::georef::Path& path,
               const navitia::type::Data& data, pbnavitia::StreetNetwork* sn,
               const pt::ptime&, const pt::time_period&){
    switch(ori_dest.streetnetwork_params.mode){
        case type::Mode_e::Bike:
            sn->set_mode(pbnavitia::Bike);
            break;
        case type::Mode_e::Car:
            sn->set_mode(pbnavitia::Car);
            break;
    case type::Mode_e::Vls:
        sn->set_mode(pbnavitia::Vls);
        break;
        default :
            sn->set_mode(pbnavitia::Walking);
    }

    uint32_t length = 0;
    for(auto item : path.path_items){
        if(item.way_idx < data.geo_ref.ways.size()){
            pbnavitia::PathItem * path_item = sn->add_path_items();
            path_item->set_name(data.geo_ref.ways[item.way_idx]->name);
            path_item->set_length(item.length);
            length += item.length;
        }else{
            std::cout << "Way étrange : " << item.way_idx << std::endl;
        }
    }
    sn->set_length(length);
    for(auto coord : path.coordinates){
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


void fill_pb_object(const navitia::type::StopTime* stop_time,
                    const nt::Data& data,
                    pbnavitia::ScheduleStopTime* rs_stop_time, int,
                    const boost::posix_time::ptime&,
                    const boost::posix_time::time_period&,
                    const type::DateTime& date_time){
    if(stop_time != nullptr) {
        rs_stop_time->set_stop_time(iso_string(date_time.date(),
                                    date_time.hour(), data));
        pbnavitia::hasPropertie * hn = rs_stop_time->mutable_has_properties();
        if ((!stop_time->drop_off_allowed()) && stop_time->pick_up_allowed()){
            hn->add_additional_informations(pbnavitia::hasPropertie::PICK_UP_ONLY);
        }
        if (stop_time->drop_off_allowed() && (!stop_time->pick_up_allowed())){
            hn->add_additional_informations(pbnavitia::hasPropertie::DROP_OFF_ONLY);
        }
        if (stop_time->odt()){
            hn->add_additional_informations(pbnavitia::hasPropertie::ON_DEMAND_TRANSPORT);
        }
        if (stop_time->date_time_estimated()){
            hn->add_additional_informations(pbnavitia::hasPropertie::DATE_TIME_ESTIMATED);
        }
        if(!stop_time->comment.empty()){
            pbnavitia::Note* note = hn->add_notes();
            auto note_id = std::to_string(stop_time->journey_pattern_point->idx)
                           + std::to_string(stop_time->vehicle_journey->idx);
            note->set_uri("note:"+note_id);
            note->set_note(stop_time->comment);
        }
    }else {
        rs_stop_time->set_stop_time("");
    }
}
}//namespace navitia
