#include "pt_data.h"

namespace navitia{namespace type {


PT_Data& PT_Data::operator=(PT_Data&& other){
#define COPY_FROM_OTHER(type_name, collection_name) collection_name = other.collection_name; collection_name##_map = other.collection_name##_map;
    ITERATE_NAVITIA_PT_TYPES(COPY_FROM_OTHER)

    stop_point_connections = other.stop_point_connections;
    journey_pattern_point_connections = other.journey_pattern_point_connections;
    stop_times = other.stop_times;

    // First letter
    stop_area_autocomplete = other.stop_area_autocomplete;
    stop_point_autocomplete = other.stop_point_autocomplete;
    line_autocomplete = other.line_autocomplete;

    // Proximity list
    stop_area_proximity_list = other.stop_area_proximity_list;
    stop_point_proximity_list = other.stop_point_proximity_list;

    return *this;
}



//void PT_Data::build_autocomplete(const std::map<std::string, std::string> & map_alias, const std::map<std::string, std::string> & map_synonymes){
void PT_Data::build_autocomplete(const navitia::georef::GeoRef & georef){
    for(const StopArea & sa : this->stop_areas){
        std::string key="";
        for(idx_t idx : sa.admin_list){
            navitia::georef::Admin admin = georef.admins.at(idx);
            key +=" " + admin.name;
        }
        this->stop_area_autocomplete.add_string(sa.name + " " + key, sa.idx,georef.alias, georef.synonymes);
    }
    this->stop_area_autocomplete.build();

    for(const StopPoint & sp : this->stop_points){
        std::string key="";
        for(idx_t idx : sp.admin_list){
            navitia::georef::Admin admin = georef.admins.at(idx);
            key += key + " " + admin.name;
        }
        this->stop_point_autocomplete.add_string(sp.name + " " + key, sp.idx, georef.alias, georef.synonymes);
    }
    this->stop_point_autocomplete.build();

    for(const Line & line : this->lines){
        this->line_autocomplete.add_string(line.name, line.idx, georef.alias, georef.synonymes);
    }
    this->line_autocomplete.build();
}


void PT_Data::build_proximity_list() {
    for(const StopArea &stop_area : this->stop_areas){
        this->stop_area_proximity_list.add(stop_area.coord, stop_area.idx);
    }
    this->stop_area_proximity_list.build();

    for(const StopPoint & stop_point : this->stop_points){
        this->stop_point_proximity_list.add(stop_point.coord, stop_point.idx);
    }
    this->stop_point_proximity_list.build();
}

void PT_Data::build_uri() {
#define NORMALIZE_EXT_CODE(type_name, collection_name) for(auto &element : collection_name) collection_name##_map[element.uri] = element.idx;
    ITERATE_NAVITIA_PT_TYPES(NORMALIZE_EXT_CODE)
}

void PT_Data::build_connections() {
    stop_point_connections.resize(stop_points.size());
    for(Connection &conn : this->connections){
        const StopPoint & dep = this->stop_points[conn.departure_idx];
        const StopPoint & arr = this->stop_points[conn.destination_idx];
        if(dep.stop_area_idx == arr.stop_area_idx)
            conn.connection_type = ConnectionType::StopArea;
        else
            conn.connection_type = ConnectionType::Walking;
        stop_point_connections[dep.idx].push_back(conn);
    }
}

}}
