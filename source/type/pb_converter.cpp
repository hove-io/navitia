#include "pb_converter.h"
#include "georef/georef.h"
#include "georef/street_network.h"
#include "boost/lexical_cast.hpp"

namespace nt = navitia::type;
namespace pt = boost::posix_time;

namespace navitia{

void fill_pb_object(navitia::georef::Admin* adm, const nt::Data& , pbnavitia::AdministrativeRegion* admin, int, const pt::ptime&, const pt::time_period& ){
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

void fill_pb_object(const nt::StopArea * sa, const nt::Data& data, pbnavitia::StopArea* stop_area, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period){
    if(sa == nullptr)
        return ;
    stop_area->set_uri(sa->uri);
    stop_area->set_name(sa->name);
    if(sa->coord.is_initialized()) {
        stop_area->mutable_coord()->set_lon(sa->coord.lon());
        stop_area->mutable_coord()->set_lat(sa->coord.lat());
    }
    if(max_depth > 0){
        for(navitia::georef::Admin* adm : sa->admin_list){
            fill_pb_object(adm, data,  stop_area->add_administrative_regions(), max_depth-1, now, action_period);
        }
    }

    for(const auto& message : data.pt_data.message_holder.find_messages(sa->uri, now, action_period)){
        fill_message(message, data, stop_area->add_messages(), max_depth-1, now, action_period);
    }
}

void fill_pb_object(const nt::StopPoint* sp, const nt::Data& data, pbnavitia::StopPoint* stop_point, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period){
    if(sp == nullptr)
        return ;

    stop_point->set_uri(sp->uri);
    stop_point->set_name(sp->name);
    if(sp->coord.is_initialized()) {
        stop_point->mutable_coord()->set_lon(sp->coord.lon());
        stop_point->mutable_coord()->set_lat(sp->coord.lat());
    }

    if(max_depth > 0){
        for(navitia::georef::Admin* adm : sp->admin_list){
            fill_pb_object(adm, data,  stop_point->add_administrative_regions(), max_depth-1, now, action_period);
        }
    }

    if(max_depth > 0 && sp->stop_area != nullptr)
        fill_pb_object(sp->stop_area, data, stop_point->mutable_stop_area(), max_depth-1, now, action_period);


    for(const auto& message : data.pt_data.message_holder.find_messages(sp->uri, now, action_period)){
        fill_message(message, data, stop_point->add_messages(), max_depth-1, now, action_period);
    }
}

void fill_pb_object(navitia::georef::Way* way, const nt::Data& data, pbnavitia::Address * address, int house_number,
        type::GeographicalCoord& coord, int max_depth, const pt::ptime& now, const pt::time_period& action_period){
    if(way == nullptr)
        return ;
    address->set_name(way->name);
    if(house_number >= 0){
        address->set_house_number(house_number);
    }
    if(coord.is_initialized()) {
        address->mutable_coord()->set_lon(coord.lon());
        address->mutable_coord()->set_lat(coord.lat());
    }
    address->set_uri(way->uri);

    if(max_depth > 0){
        for(georef::Admin* admin : way->admin_list){
            fill_pb_object(admin, data,  address->add_administrative_regions(), max_depth-1, now, action_period);
        }
    }
}

void fill_pb_object(nt::Line const* l, const nt::Data& data, pbnavitia::Line * line, int depth,
                    const pt::ptime& now, const pt::time_period& action_period){
    if(l == nullptr)
        return ;

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
            fill_pb_object(physical_mode, data, line->add_physical_mode(), depth-1);
        }
        
        fill_pb_object(l->commercial_mode, data, line->mutable_commercial_mode(), depth-1);
        fill_pb_object(l->network, data, line->mutable_network(), depth-1);
    }

    for(const auto& message : data.pt_data.message_holder.find_messages(l->uri, now, action_period)){
        fill_message(message, data, line->add_messages(), depth-1, now, action_period);
    }
}

void fill_pb_object(const nt::JourneyPattern* jp, const nt::Data& data, pbnavitia::JourneyPattern * journey_pattern, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period){
    if(jp == nullptr)
        return ;

    journey_pattern->set_name(jp->name);
    journey_pattern->set_uri(jp->uri);
    if(max_depth > 0 && jp->route != nullptr)
        fill_pb_object(jp->route, data, journey_pattern->mutable_route(), max_depth - 1, now, action_period);

    for(const auto& message : data.pt_data.message_holder.find_messages(jp->uri, now, action_period)){
        fill_message(message, data, journey_pattern->add_messages(), max_depth-1, now, action_period);
    }
}

void fill_pb_object(const nt::Route* r, const nt::Data& data, pbnavitia::Route * route, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period){
    if(r == nullptr)
        return ;

    route->set_name(r->name);
    route->set_uri(r->uri);
    if(max_depth > 0 && r->line != nullptr)
        fill_pb_object(r->line, data, route->mutable_line(), max_depth - 1, now, action_period);

    for(const auto& message : data.pt_data.message_holder.find_messages(r->uri, now, action_period)){
        fill_message(message, data, route->add_messages(), max_depth-1, now, action_period);
    }
}

void fill_pb_object(const nt::Network* n, const nt::Data&, pbnavitia::Network * network, int,
                    const pt::ptime&, const pt::time_period&){
    if(n == nullptr)
        return ;

    network->set_name(n->name);
    network->set_uri(n->uri);
}

void fill_pb_object(const nt::CommercialMode* m, const nt::Data&, pbnavitia::CommercialMode * commercial_mode,
                    int, const pt::ptime&, const pt::time_period&){
    if(m == nullptr)
        return ;

    commercial_mode->set_name(m->name);
    commercial_mode->set_uri(m->uri);
}

void fill_pb_object(const nt::PhysicalMode* m, const nt::Data&, pbnavitia::PhysicalMode * physical_mode, int,
                    const pt::ptime&, const pt::time_period&){
    if(m == nullptr)
        return ;

    physical_mode->set_name(m->name);
    physical_mode->set_uri(m->uri);
}

void fill_pb_object(const nt::Company* c, const nt::Data&, pbnavitia::Company * company, int, const pt::ptime&, const pt::time_period&){
    if(c == nullptr)
        return ;

    company->set_name(c->name);
    company->set_uri(c->uri);
}

void fill_pb_object(const nt::StopPointConnection* c, const nt::Data& data, pbnavitia::Connection * connection, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period){
    if(c == nullptr)
        return ;

    connection->set_seconds(c->duration);
    if(c->departure != nullptr && c->destination != nullptr && max_depth > 0){
        fill_pb_object(c->departure, data, connection->mutable_origin(), max_depth - 1, now, action_period);
        fill_pb_object(c->destination, data, connection->mutable_destination(), max_depth - 1, now, action_period);
    }
}

void fill_pb_object(const nt::JourneyPatternPointConnection* , const nt::Data& , pbnavitia::Connection* , int,
                    const pt::ptime&, const pt::time_period&){
}


void fill_pb_object(const nt::ValidityPattern* vp, const nt::Data&, pbnavitia::ValidityPattern* validity_pattern, int, const pt::ptime&, const pt::time_period&){
    if(vp == nullptr)
        return;

    validity_pattern->set_beginning_date(boost::gregorian::to_iso_string(vp->beginning_date));
    validity_pattern->set_days(vp->days.to_string());
}

void fill_pb_object(const nt::VehicleJourney* vj, const nt::Data& data, pbnavitia::VehicleJourney * vehicle_journey, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period){
    if(vj == nullptr)
        return ;

    vehicle_journey->set_name(vj->name);
    vehicle_journey->set_uri(vj->uri);
    vehicle_journey->set_is_adapted(vj->is_adapted);
    if(vj->journey_pattern!= nullptr && max_depth > 0)
        fill_pb_object(vj->journey_pattern, data, vehicle_journey->mutable_journey_pattern(), max_depth-1, now, action_period);

    if(max_depth > 0) {
        for(auto* stop_time : vj->stop_time_list) {
            fill_pb_object(stop_time, data, vehicle_journey->add_stop_times(), max_depth -1, now, action_period);
        }
        fill_pb_object(vj->physical_mode, data, vehicle_journey->mutable_physical_mode(), max_depth-1, now, action_period);

        fill_pb_object(vj->validity_pattern, data, vehicle_journey->mutable_validity_pattern(), max_depth-1);
        fill_pb_object(vj->adapted_validity_pattern, data, vehicle_journey->mutable_adapted_validity_pattern(), max_depth-1);

    }

    for(auto message : data.pt_data.message_holder.find_messages(vj->uri, now, action_period)){
        fill_message(message, data, vehicle_journey->add_messages(), max_depth-1, now, action_period);
    }
    //si on a un vj théorique rataché à notre vj, on récupére les messages qui le concerne
    if(vj->theoric_vehicle_journey != nullptr){
        for(auto message : data.pt_data.message_holder.find_messages(vj->theoric_vehicle_journey->uri, now, action_period)){
            fill_message(message, data, vehicle_journey->add_messages(), max_depth-1, now, action_period);
        }
    }
}

void fill_pb_object(const nt::StopTime* st, const type::Data &data, pbnavitia::StopTime *stop_time, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period) {
    if(st == nullptr)
        return ;
    boost::posix_time::time_duration p = boost::posix_time::seconds(st->arrival_time);

    stop_time->set_arrival_time(boost::posix_time::to_iso_string(p));

    p = boost::posix_time::seconds(st->departure_time);
    stop_time->set_departure_time(boost::posix_time::to_iso_string(p));
    stop_time->set_pickup_allowed(st->pick_up_allowed());
    stop_time->set_drop_off_allowed(st->drop_off_allowed());
    if(st->journey_pattern_point != nullptr && max_depth > 0)
        fill_pb_object(st->journey_pattern_point, data, stop_time->mutable_journey_pattern_point(), max_depth-1, now, action_period);

    if(st->vehicle_journey != nullptr && max_depth > 0)
        fill_pb_object(st->vehicle_journey, data, stop_time->mutable_vehicle_journey(), max_depth-1, now, action_period);
}


void fill_pb_object(const nt::JourneyPatternPoint* jpp, const nt::Data& data, pbnavitia::JourneyPatternPoint * journey_pattern_point, int max_depth,
                    const pt::ptime& now, const pt::time_period& action_period){
    if(jpp == nullptr)
        return ;

    journey_pattern_point->set_uri(jpp->uri);
    journey_pattern_point->set_order(jpp->order);

    if(max_depth > 0){
        if(jpp->stop_point != nullptr)
            fill_pb_object(jpp->stop_point, data, journey_pattern_point->mutable_stop_point(), max_depth - 1, now, action_period);
        if(jpp->journey_pattern != nullptr)
            fill_pb_object(jpp->journey_pattern, data, journey_pattern_point->mutable_journey_pattern(), max_depth - 1, now, action_period);
    }
    for(auto message : data.pt_data.message_holder.find_messages(jpp->uri, now, action_period)){
        fill_message(message, data, journey_pattern_point->add_messages(), max_depth-1, now, action_period);
    }
}


void fill_pb_placemark(const type::StopPoint* stop_point, const type::Data &data, pbnavitia::Place* place, int max_depth,
                       const pt::ptime& now, const pt::time_period& action_period){
    fill_pb_object(stop_point, data, place->mutable_stop_point(), max_depth, now, action_period);
    place->set_name(stop_point->name);
    place->set_uri(stop_point->uri);
}

void fill_street_section(const type::EntryPoint &ori_dest, const georef::Path &path, const type::Data &data, pbnavitia::Section* section,
        int max_depth, const pt::ptime& now, const pt::time_period& action_period){
    if(path.path_items.size() > 0) {
        section->set_type(pbnavitia::STREET_NETWORK);
        pbnavitia::StreetNetwork * sn = section->mutable_street_network();
        create_pb(ori_dest, path, data, sn);

        pbnavitia::Place* place;
        navitia::georef::Way* way;
        type::GeographicalCoord coord;

        if(path.path_items.size() > 1){

            way = data.geo_ref.ways[path.path_items.front().way_idx];
            coord = path.coordinates.front();
            place = section->mutable_origin();
            fill_pb_object(way, data, place->mutable_address(), way->nearest_number(coord),coord , max_depth, now, action_period);
            if(place->address().has_house_number())
                place->set_name(boost::lexical_cast<std::string>(place->address().house_number()) + ", ");
            place->set_name(place->name() + place->address().name());
            for(auto admin : place->address().administrative_regions())
                place->set_name(place->name() + ", " + admin.name());
            place->set_uri(place->address().uri());

            way = data.geo_ref.ways[path.path_items.back().way_idx];
            coord = path.coordinates.back();
            place = section->mutable_destination();
            fill_pb_object(way, data, place->mutable_address(), way->nearest_number(coord),coord , max_depth, now, action_period);
            if(place->address().has_house_number())
                place->set_name(boost::lexical_cast<std::string>(place->address().house_number()) + ", ");
            place->set_name(place->name() + place->address().name());
            for(auto admin : place->address().administrative_regions())
                place->set_name(place->name() + ", " + admin.name());
            place->set_uri(place->address().uri());
        }
    }
}

void fill_message(const type::Message & message, const type::Data&, pbnavitia::Message* pb_message, int,
        const boost::posix_time::ptime&, const boost::posix_time::time_period&){
    pb_message->set_uri(message.uri);
    pb_message->set_message(message.message);
    pb_message->set_title(message.title);
}

void create_pb(const type::EntryPoint &ori_dest, const navitia::georef::Path& path, const navitia::type::Data& data, pbnavitia::StreetNetwork* sn,
        const pt::ptime&, const pt::time_period&){
    sn->set_length(path.length);

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

    for(auto item : path.path_items){
        if(item.way_idx < data.geo_ref.ways.size()){
            pbnavitia::PathItem * path_item = sn->add_path_items();
            path_item->set_name(data.geo_ref.ways[item.way_idx]->name);
            path_item->set_length(item.length);
        }else{
            std::cout << "Way étrange : " << item.way_idx << std::endl;
        }
    }
    for(auto coord : path.coordinates){
        if(coord.is_initialized()) {
            pbnavitia::GeographicalCoord * pb_coord = sn->add_coordinates();
            pb_coord->set_lon(coord.lon());
            pb_coord->set_lat(coord.lat());
        }
    }
}

void fill_pb_object(const georef::POIType* geo_poi_type, const type::Data &, pbnavitia::PoiType* poi_type, int,
        const pt::ptime&, const pt::time_period&) {    
    poi_type->set_name(geo_poi_type->name);
    poi_type->set_uri(geo_poi_type->uri);
}

void fill_pb_object(const georef::POI* geopoi, const type::Data &data, pbnavitia::Poi* poi, int max_depth,
        const pt::ptime& now, const pt::time_period& action_period){
    poi->set_name(geopoi->name);
    poi->set_uri(geopoi->uri);
    if(geopoi->coord.is_initialized()) {
        poi->mutable_coord()->set_lat(geopoi->coord.lat());
        poi->mutable_coord()->set_lon(geopoi->coord.lon());
    }

    if(max_depth > 0){
        fill_pb_object(data.geo_ref.poitypes[geopoi->poitype_idx], data, poi->mutable_poi_type(), max_depth-1, now, action_period);
        for(georef::Admin * admin : geopoi->admin_list){
            fill_pb_object(admin, data,  poi->add_administrative_regions(), max_depth-1, now, action_period);
        }
    }
}

void fill_pb_object(const navitia::type::StopTime* stop_time, const nt::Data& data, pbnavitia::RouteScheduleRow* row, int,
                    const boost::posix_time::ptime&,
                    const boost::posix_time::time_period&,
                    const type::DateTime& date_time){

    if(stop_time != nullptr) {
        pbnavitia::RouteScheduleStopTime* rs_stop_time = row->add_stop_times();
        rs_stop_time->set_stop_time(iso_string(date_time.date(),  date_time.hour(), data));

        if ((!stop_time->drop_off_allowed()) && stop_time->pick_up_allowed()){
            rs_stop_time->add_additional_informations(pbnavitia::RouteScheduleStopTime::PICK_UP_ONLY);
        }
        if (stop_time->drop_off_allowed() && (!stop_time->pick_up_allowed())){
            rs_stop_time->add_additional_informations(pbnavitia::RouteScheduleStopTime::DROP_OFF_ONLY);
        }
        if (stop_time->odt()){
            rs_stop_time->add_additional_informations(pbnavitia::RouteScheduleStopTime::ON_DEMAND_TRANSPORT);
        }
        if (stop_time->date_time_estimated()){
            rs_stop_time->add_additional_informations(pbnavitia::RouteScheduleStopTime::DATE_TIME_ESTIMATED);
        }
        if(!stop_time->comment.empty()){
            pbnavitia::Note* note = rs_stop_time->add_notes();
            note->set_uri("note:"+std::to_string(stop_time->journey_pattern_point->idx) + std::to_string(stop_time->vehicle_journey->idx));
            note->set_note(stop_time->comment);
        }

    }

}

void fill_pb_object(const navitia::type::VehicleJourney* vehiclejourney, const nt::Data& data,  pbnavitia::Header* header, int,
                    const boost::posix_time::ptime&,
                    const boost::posix_time::time_period&,
                    const type::DateTime&){

    if(vehiclejourney != nullptr){
        fill_pb_object(vehiclejourney, data, header->mutable_vehiclejourney());
        header->set_direction(vehiclejourney->get_direction());
    }
}
}//namespace navitia
