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
    city->mutable_coord()->set_x(city_n.coord.x);
    city->mutable_coord()->set_y(city_n.coord.y);
}

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::StopArea* stop_area, int max_depth){
    nt::StopArea sa = data.pt_data.stop_areas.at(idx);
    stop_area->set_id(sa.id);
    stop_area->set_external_code(sa.external_code);
    stop_area->set_name(sa.name);
    stop_area->mutable_coord()->set_x(sa.coord.x);
    stop_area->mutable_coord()->set_y(sa.coord.y);
    if(max_depth > 0){
        try{
            fill_pb_object(sa.city_idx, data, stop_area->mutable_child()->add_city(), max_depth-1);
        }catch(std::out_of_range e){}
    }
}

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::StopPoint* stop_point, int max_depth){
    nt::StopPoint sp = data.pt_data.stop_points.at(idx);
    stop_point->set_id(sp.id);
    stop_point->set_external_code(sp.external_code);
    stop_point->set_name(sp.name);
    stop_point->mutable_coord()->set_x(sp.coord.x);
    stop_point->mutable_coord()->set_y(sp.coord.y);
    if(max_depth > 0){
        try{
            fill_pb_object(sp.city_idx, data, stop_point->mutable_city(), max_depth-1);
        }catch(std::out_of_range e){}
        
    }
}

void fill_pb_object(nt::idx_t idx, const nt::Data& data, pbnavitia::Way * way, int max_depth){
    navitia::georef::Way w = data.geo_ref.ways.at(idx);
    way->set_name(w.name);
    /*stop_point->mutable_coord()->set_x(sp.coord.x);
    stop_point->mutable_coord()->set_y(sp.coord.y);*/
    if(max_depth > 0){
        try{
            fill_pb_object(w.city_idx, data, way->mutable_child()->add_city(), max_depth-1);
        }catch(std::out_of_range e){
            std::cout << w.city_idx << std::endl;
        }
        
    }
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

void fill_pb_placemark(const type::StopPoint & stop_point, const type::Data &data, pbnavitia::PlaceMark* pm, int max_depth){
    pm->set_type(pbnavitia::STOPPOINT);
    fill_pb_object(stop_point.idx, data, pm->mutable_stop_point(), max_depth);
}

void fill_pb_placemark(const georef::Way & way, const type::Data &data, pbnavitia::PlaceMark* pm, int max_depth, int house_number){
    pm->set_type(pbnavitia::ADDRESS);
    pbnavitia::Address * address = pm->mutable_address();
    pbnavitia::Way * pb_way = address->mutable_way();
    pb_way->set_name(way.name);
    if(house_number >= 0){
        address->set_house_number(house_number);
    }

    if(max_depth > 0)
        fill_pb_object(way.city_idx, data,  pb_way->mutable_city());

}

void fill_road_section(const georef::Path &path, const type::Data &data, pbnavitia::Section* section, int max_depth){
    section->set_type(pbnavitia::ROAD_NETWORK);
    pbnavitia::StreetNetwork * sn = section->mutable_street_network();
    streetnetwork::create_pb(path, data, sn);

    if(path.path_items.size() > 1){
        fill_pb_placemark(data.geo_ref.ways[path.path_items.front().way_idx], data, section->mutable_origin(), max_depth);
        fill_pb_placemark(data.geo_ref.ways[path.path_items.back().way_idx], data, section->mutable_destination(), max_depth);
    }
}

}//namespace navitia
