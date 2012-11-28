#include "pb_converter.h"
#include "street_network/street_network_api.h"
namespace nt = navitia::type;
namespace navitia{

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::City* city, int){
    nt::City city_n = data.pt_data.cities.at(idx);
    city->set_id(city_n.id);
    city->set_id(city_n.id);
    city->set_external_code(city_n.external_code);
    city->set_name(city_n.name);
    city->mutable_coord()->set_lon(city_n.coord.lon());
    city->mutable_coord()->set_lat(city_n.coord.lat());
}

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::StopArea* stop_area, int max_depth){
    nt::StopArea sa = data.pt_data.stop_areas.at(idx);
    stop_area->set_id(sa.id);
    stop_area->set_external_code(sa.external_code);
    stop_area->set_name(sa.name);
    stop_area->mutable_coord()->set_lon(sa.coord.lon());
    stop_area->mutable_coord()->set_lat(sa.coord.lat());
    if(max_depth > 0 && sa.city_idx != nt::invalid_idx)
        fill_pb_object(sa.city_idx, data, stop_area->mutable_city(), max_depth-1);
}

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::StopPoint* stop_point, int max_depth){
    nt::StopPoint sp = data.pt_data.stop_points.at(idx);
    stop_point->set_id(sp.id);
    stop_point->set_external_code(sp.external_code);
    stop_point->set_name(sp.name);
    stop_point->mutable_coord()->set_lon(sp.coord.lon());
    stop_point->mutable_coord()->set_lat(sp.coord.lat());
    if(max_depth > 0 && sp.city_idx != nt::invalid_idx)
            fill_pb_object(sp.city_idx, data, stop_point->mutable_city(), max_depth-1);
    if(max_depth > 0 && sp.stop_area_idx != nt::invalid_idx)
            fill_pb_object(sp.stop_area_idx, data, stop_point->mutable_stop_area(), max_depth-1);
}

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::Way * way, int max_depth){
    navitia::georef::Way w = data.geo_ref.ways.at(idx);
    way->set_name(w.name);
    if(max_depth && w.city_idx != nt::invalid_idx)
        fill_pb_object(w.city_idx, data, way->mutable_city(), max_depth-1);
}


void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::Address * address, int house_number,type::GeographicalCoord& coord, int max_depth){
    navitia::georef::Way way = data.geo_ref.ways.at(idx);
    pbnavitia::Way * pb_way = address->mutable_way();
    pb_way->set_name(way.name);
    if(house_number >= 0){
        address->set_house_number(house_number);
    }
    pb_way->mutable_coord()->set_lon(coord.lon());
    pb_way->mutable_coord()->set_lat(coord.lat());
    if(max_depth > 0)
        fill_pb_object(way.city_idx, data,  pb_way->mutable_city());
}

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::Line * line, int){
    navitia::type::Line l = data.pt_data.lines.at(idx);
    line->set_forward_name(l.forward_name);
    line->set_backward_name(l.backward_name);
    line->set_code(l.code);
    line->set_color(l.color);
    line->set_name(l.name);
    line->set_external_code(l.external_code);
}

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::Route * route, int max_depth){
    navitia::type::Route r = data.pt_data.routes.at(idx);
    route->set_name(r.name);
    route->set_id(r.id);
    route->set_external_code(r.external_code);
    if(max_depth > 0 && r.line_idx != type::invalid_idx)
        fill_pb_object(r.line_idx, data, route->mutable_line(), max_depth - 1);
}

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::Network * network, int){
    navitia::type::Network n = data.pt_data.networks.at(idx);
    network->set_name(n.name);
    network->set_id(n.id);
    network->set_external_code(n.external_code);
}

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::CommercialMode * commercial_mode, int){
    navitia::type::ModeType m = data.pt_data.mode_types.at(idx);
    commercial_mode->set_name(m.name);
    commercial_mode->set_id(m.id);
    commercial_mode->set_external_code(m.external_code);
}

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::PhysicalMode * physical_mode, int){
    navitia::type::Mode m = data.pt_data.modes.at(idx);
    physical_mode->set_name(m.name);
    physical_mode->set_id(m.id);
    physical_mode->set_external_code(m.external_code);
}

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::Company * company, int){
    navitia::type::Company c = data.pt_data.companies.at(idx);
    company->set_name(c.name);
    company->set_id(c.id);
    company->set_external_code(c.external_code);
}

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::Connection * connection, int max_depth){
    navitia::type::Connection c = data.pt_data.connections.at(idx);
    connection->set_seconds(c.duration);
    if(c.departure_stop_point_idx != type::invalid_idx && c.destination_stop_point_idx != type::invalid_idx && max_depth > 0){
        fill_pb_object(c.departure_stop_point_idx, data, connection->mutable_origin(), max_depth - 1);
        fill_pb_object(c.destination_stop_point_idx, data, connection->mutable_destination(), max_depth - 1);
    }
}

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::RoutePoint * route_point, int max_depth){
    navitia::type::RoutePoint rp = data.pt_data.route_points.at(idx);
    route_point->set_id(rp.id);
    route_point->set_external_code(rp.external_code);
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

void fill_pb_placemark(const georef::Way & way, const type::Data &data, pbnavitia::PlaceMark* pm, int max_depth, int house_number){
    pm->set_type(pbnavitia::ADDRESS);
    type::GeographicalCoord coord;
    fill_pb_object(way.idx, data, pm->mutable_address(), house_number,coord , max_depth);
    /*pbnavitia::Address * address = pm->mutable_address();
    pbnavitia::Way * pb_way = address->mutable_way();
    pb_way->set_name(way.name);
    if(house_number >= 0){
        address->set_house_number(house_number);
    }

    if(max_depth > 0 && way.city_idx != type::invalid_idx)
        fill_pb_object(way.city_idx, data,  pb_way->mutable_city());*/

}

void fill_road_section(const georef::Path &path, const type::Data &data, pbnavitia::Section* section, int max_depth){
    if(path.path_items.size() > 0) {
        section->set_type(pbnavitia::ROAD_NETWORK);
        pbnavitia::StreetNetwork * sn = section->mutable_street_network();
        streetnetwork::create_pb(path, data, sn);

        if(path.path_items.size() > 1){
            fill_pb_placemark(data.geo_ref.ways[path.path_items.front().way_idx], data, section->mutable_origin(), max_depth);
            fill_pb_placemark(data.geo_ref.ways[path.path_items.back().way_idx], data, section->mutable_destination(), max_depth);
        }
    }
}

}//namespace navitia
