#include "build_helper.h"
#include "gtfs_parser.h"
namespace navimake {
VJ & VJ::frequency(uint32_t start_time, uint32_t end_time, uint32_t headway_secs) {

    uint32_t first_time = vj->stop_time_list.front()->arrival_time;
    for(types::StopTime *st : vj->stop_time_list) {
        st->is_frequency = true;
        st->start_time = start_time+(st->arrival_time - first_time);
        st->end_time = end_time+(st->departure_time - first_time);
        st->headway_secs = headway_secs;
    }

    return *this;

}


VJ::VJ(builder & b, const std::string &line_name, const std::string &validity_pattern, const std::string &block_id, bool is_adapted) : b(b){
    vj = new types::VehicleJourney();
    b.data.vehicle_journeys.push_back(vj);

    auto it = b.lines.find(line_name);
    if(it == b.lines.end()){
        vj->tmp_line = new types::Line();
        vj->tmp_line->external_code = line_name;
        b.lines[line_name] = vj->tmp_line;
        vj->tmp_line->name = line_name;
        b.data.lines.push_back(vj->tmp_line);
    } else {
        vj->tmp_line = it->second;
    }

    auto vp_it = b.vps.find(validity_pattern);
    if(vp_it == b.vps.end()){
        vj->validity_pattern = new types::ValidityPattern(b.begin, validity_pattern);
        b.vps[validity_pattern] = vj->validity_pattern;
        b.data.validity_patterns.push_back(vj->validity_pattern);
    } else {
        vj->validity_pattern = vp_it->second;
    }
    vj->block_id = block_id;
    vj->is_adapted = is_adapted;
}

VJ & VJ::operator()(const std::string & sp_name, int arrivee, int depart, uint32_t local_trafic_zone, bool drop_off_allowed, bool pick_up_allowed){
    types::StopTime * st = new types::StopTime();
    b.data.stops.push_back(st);
    auto it = b.sps.find(sp_name);
    if(it == b.sps.end()){
        st->tmp_stop_point = new types::StopPoint();        
        st->tmp_stop_point->name = sp_name;
        st->tmp_stop_point->external_code = sp_name;
        b.sps[sp_name] = st->tmp_stop_point;
        b.data.stop_points.push_back(st->tmp_stop_point);
        auto sa_it = b.sas.find(sp_name);
        if(sa_it == b.sas.end()) {
            st->tmp_stop_point->stop_area = new types::StopArea();
            st->tmp_stop_point->stop_area->name = sp_name;
            st->tmp_stop_point->stop_area->external_code = sp_name;
            st->tmp_stop_point->stop_area->is_adapted = true;
            b.sas[sp_name] = st->tmp_stop_point->stop_area;
            b.data.stop_areas.push_back(st->tmp_stop_point->stop_area);
            st->tmp_stop_point->is_adapted = true;
        } else {
            st->tmp_stop_point->stop_area = sa_it->second;
            st->tmp_stop_point->is_adapted = st->tmp_stop_point->stop_area->is_adapted;
        }
    } else {
        st->tmp_stop_point = it->second;
    }

    if(depart == -1) depart = arrivee;
    st->arrival_time = arrivee;
    st->departure_time = depart;
    st->vehicle_journey = vj;
    st->order = vj->stop_time_list.size();
    st->local_traffic_zone = local_trafic_zone;
    st->drop_off_allowed = drop_off_allowed;
    st->pick_up_allowed = pick_up_allowed;
    vj->stop_time_list.push_back(st);
    st->is_adapted = st->vehicle_journey->is_adapted;

    return *this;
}

SA::SA(builder & b, const std::string & sa_name, double x, double y, bool is_adapted) : b(b) {
    sa = new types::StopArea();
    b.data.stop_areas.push_back(sa);
    sa->name = sa_name;
    sa->external_code = sa_name;
    sa->coord.set_lon(x);
    sa->coord.set_lat(y);
    sa->is_adapted = is_adapted;
    b.sas[sa_name] = sa;
}

SA & SA::operator()(const std::string & sp_name, double x, double y, bool is_adapted){
    types::StopPoint * sp = new types::StopPoint();
    b.data.stop_points.push_back(sp);
    sp->name = sp_name;
    sp->external_code = sp_name;
    sp->is_adapted = is_adapted;
    sa->coord.set_lon(x);
    sa->coord.set_lat(y);
    sp->is_adapted = is_adapted;
    sp->stop_area = this->sa;
    b.sps[sp_name] = sp;
    return *this;
}

VJ builder::vj(const std::string &line_name, const std::string &validity_pattern, const std::string & block_id, const bool is_adapted){
    return VJ(*this, line_name, validity_pattern, block_id, is_adapted);
}

SA builder::sa(const std::string &name, double x, double y, const bool is_adapted){
    return SA(*this, name, x, y, is_adapted);
}

void builder::connection(const std::string & name1, const std::string & name2, float length) {
    types::Connection * connexion = new types::Connection();
    if(sps.count(name1) == 0 || sps.count(name2) == 0)
        return ;
    connexion->departure_stop_point = (*(sps.find(name1))).second;
    connexion->destination_stop_point = (*(sps.find(name2))).second;

    connexion->connection_kind = types::Connection::LinkConnection;
    connexion->duration = length;

    data.connections.push_back(connexion);

}

 void builder::build(navitia::type::PT_Data & pt_data) {
    navitia::type::PT_Data result;
    connectors::build_routes(data);
    connectors::build_route_points(data);
    connectors::build_route_point_connections(data);

    data.clean();
    data.sort();
    data.transform(pt_data);
    pt_data.build_external_code();
}




}
