#include "pb_converter.h"
#include "georef/georef.h"
#include "georef/street_network.h"
namespace nt = navitia::type;
namespace navitia{

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::City* city, int){
    nt::City city_n = data.pt_data.cities.at(idx);
    city->set_id(city_n.id);
    city->set_id(city_n.id);
    city->set_zip_code(city_n.main_postal_code);
    city->set_uri(city_n.uri);
    city->set_name(city_n.name);
    city->mutable_coord()->set_lon(city_n.coord.lon());
    city->mutable_coord()->set_lat(city_n.coord.lat());
}

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::StopArea* stop_area, int max_depth){
    nt::StopArea sa = data.pt_data.stop_areas.at(idx);
    stop_area->set_id(sa.id);
    stop_area->set_uri(sa.uri);
    stop_area->set_name(sa.name);
    stop_area->mutable_coord()->set_lon(sa.coord.lon());
    stop_area->mutable_coord()->set_lat(sa.coord.lat());
    stop_area->set_is_adapted(sa.is_adapted);
    if(max_depth > 0 && sa.city_idx != nt::invalid_idx)
        fill_pb_object(sa.city_idx, data, stop_area->mutable_city(), max_depth-1);
}

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::StopPoint* stop_point, int max_depth){
    nt::StopPoint sp = data.pt_data.stop_points.at(idx);
    stop_point->set_id(sp.id);
    stop_point->set_uri(sp.uri);
    stop_point->set_name(sp.name);
    stop_point->mutable_coord()->set_lon(sp.coord.lon());
    stop_point->mutable_coord()->set_lat(sp.coord.lat());
    stop_point->set_is_adapted(sp.is_adapted);
    if(max_depth > 0 && sp.city_idx != nt::invalid_idx)
            fill_pb_object(sp.city_idx, data, stop_point->mutable_city(), max_depth-1);
    if(max_depth > 0 && sp.stop_area_idx != nt::invalid_idx)
            fill_pb_object(sp.stop_area_idx, data, stop_point->mutable_stop_area(), max_depth-1);
}

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::Address * address, int house_number,type::GeographicalCoord& coord, int max_depth){
    navitia::georef::Way way = data.geo_ref.ways.at(idx);    
    address->set_name(way.name);
    if(house_number >= 0){
        address->set_house_number(house_number);
    }
    address->mutable_coord()->set_lon(coord.lon());
    address->mutable_coord()->set_lat(coord.lat());
    address->set_uri(way.uri);
    if(max_depth > 0 and way.city_idx != nt::invalid_idx)
        fill_pb_object(way.city_idx, data,  address->mutable_city());
}

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::Line * line, int){
    navitia::type::Line l = data.pt_data.lines.at(idx);
    line->set_forward_name(l.forward_name);
    line->set_backward_name(l.backward_name);
    line->set_code(l.code);
    line->set_color(l.color);
    line->set_name(l.name);
    line->set_uri(l.uri);
}

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::Route * route, int max_depth){
    navitia::type::Route r = data.pt_data.routes.at(idx);
    route->set_name(r.name);
    route->set_id(r.id);
    route->set_uri(r.uri);
    if(max_depth > 0 && r.line_idx != type::invalid_idx)
        fill_pb_object(r.line_idx, data, route->mutable_line(), max_depth - 1);
}

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::Network * network, int){
    navitia::type::Network n = data.pt_data.networks.at(idx);
    network->set_name(n.name);
    network->set_id(n.id);
    network->set_uri(n.uri);
}

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::CommercialMode * commercial_mode, int){
    navitia::type::CommercialMode m = data.pt_data.commercial_modes.at(idx);
    commercial_mode->set_name(m.name);
    commercial_mode->set_id(m.id);
    commercial_mode->set_uri(m.uri);
}

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::PhysicalMode * physical_mode, int){
    navitia::type::PhysicalMode m = data.pt_data.physical_modes.at(idx);
    physical_mode->set_name(m.name);
    physical_mode->set_id(m.id);
    physical_mode->set_uri(m.uri);
}

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::Company * company, int){
    navitia::type::Company c = data.pt_data.companies.at(idx);
    company->set_name(c.name);
    company->set_id(c.id);
    company->set_uri(c.uri);
}

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::Connection * connection, int max_depth){
    navitia::type::Connection c = data.pt_data.connections.at(idx);
    connection->set_seconds(c.duration);
    if(c.departure_stop_point_idx != type::invalid_idx && c.destination_stop_point_idx != type::invalid_idx && max_depth > 0){
        fill_pb_object(c.departure_stop_point_idx, data, connection->mutable_origin(), max_depth - 1);
        fill_pb_object(c.destination_stop_point_idx, data, connection->mutable_destination(), max_depth - 1);
    }
}

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::VehicleJourney * vehicle_journey, int max_depth){
    navitia::type::VehicleJourney vj = data.pt_data.vehicle_journeys.at(idx);
    vehicle_journey->set_name(vj.name);
    vehicle_journey->set_uri(vj.uri);
    vehicle_journey->set_is_adapted(vj.is_adapted);
    if(vj.route_idx != type::invalid_idx && max_depth > 0)
        fill_pb_object(vj.route_idx, data, vehicle_journey->mutable_route(), max_depth-1);

    if(max_depth > 0) {
        for(type::idx_t stop_time_idx : vj.stop_time_list) {
            fill_pb_object(stop_time_idx, data, vehicle_journey->add_stop_times(), max_depth -1);
        }
    }
}

void fill_pb_object(type::idx_t idx, const type::Data &data, pbnavitia::StopTime *stop_time, int max_depth) {
    navitia::type::StopTime st = data.pt_data.stop_times.at(idx);
    boost::posix_time::ptime d = boost::posix_time::from_iso_string("19700101");
    boost::posix_time::ptime p = d +  boost::posix_time::seconds(st.arrival_time);
    stop_time->set_arrival_time(boost::posix_time::to_iso_string(p));
    p = d +  boost::posix_time::seconds(st.departure_time);
    stop_time->set_arrival_time(boost::posix_time::to_iso_string(p));
    stop_time->set_is_adapted(st.is_adapted());
    stop_time->set_pickup_allowed(st.pick_up_allowed());
    stop_time->set_drop_off_allowed(st.drop_off_allowed());
    if(st.route_point_idx != type::invalid_idx && max_depth > 0)
        fill_pb_object(st.route_point_idx, data, stop_time->mutable_route_point(), max_depth-1);
    if(st.vehicle_journey_idx != type::invalid_idx && max_depth > 0)
        fill_pb_object(st.vehicle_journey_idx, data, stop_time->mutable_vehicle_journey(), max_depth-1);
}


void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::RoutePoint * route_point, int max_depth){
    navitia::type::RoutePoint rp = data.pt_data.route_points.at(idx);
    route_point->set_id(rp.id);
    route_point->set_uri(rp.uri);
    route_point->set_order(rp.order);
    if(rp.stop_point_idx != type::invalid_idx && max_depth > 0)
        fill_pb_object(rp.stop_point_idx, data, route_point->mutable_stop_point(), max_depth - 1);
    if(rp.route_idx != type::invalid_idx && max_depth > 0)
        fill_pb_object(rp.route_idx, data, route_point->mutable_route(), max_depth - 1);
}


void fill_pb_placemark(const type::StopPoint & stop_point, const type::Data &data, pbnavitia::PlaceMark* pm, int max_depth){
    pm->set_type(pbnavitia::STOPPOINT);
    fill_pb_object(stop_point.idx, data, pm->mutable_stop_point(), max_depth);
}

void fill_road_section(const georef::Path &path, const type::Data &data, pbnavitia::Section* section, int max_depth){
    if(path.path_items.size() > 0) {
        section->set_type(pbnavitia::ROAD_NETWORK);
        pbnavitia::StreetNetwork * sn = section->mutable_street_network();
        create_pb(path, data, sn);
        pbnavitia::PlaceMark* pm;
        navitia::georef::Way way;
        type::GeographicalCoord coord;

        if(path.path_items.size() > 1){

            way = data.geo_ref.ways[path.path_items.front().way_idx];
            coord = path.coordinates.front();
            pm = section->mutable_origin();
            pm->set_type(pbnavitia::ADDRESS);
            fill_pb_object(way.idx, data, pm->mutable_address(), way.nearest_number(coord),coord , max_depth);

            way = data.geo_ref.ways[path.path_items.back().way_idx];
            coord = path.coordinates.back();
            pm = section->mutable_destination();
            pm->set_type(pbnavitia::ADDRESS);
            fill_pb_object(way.idx, data, pm->mutable_address(), way.nearest_number(coord),coord , max_depth);
        }
    }
}

void create_pb(const navitia::georef::Path& path, const navitia::type::Data& data, pbnavitia::StreetNetwork* sn){
    sn->set_length(path.length);
    for(auto item : path.path_items){
        if(item.way_idx < data.geo_ref.ways.size()){
            pbnavitia::PathItem * path_item = sn->add_path_items();
            path_item->set_name(data.geo_ref.ways[item.way_idx].name);
            path_item->set_length(item.length);
        }else{
            std::cout << "Way étrange : " << item.way_idx << std::endl;
        }

    }
    for(auto coord : path.coordinates){
        pbnavitia::GeographicalCoord * pb_coord = sn->add_coordinates();
        pb_coord->set_lon(coord.lon());
        pb_coord->set_lat(coord.lat());
    }
}
}//namespace navitia
