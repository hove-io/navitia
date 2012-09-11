#include "build_helper.h"
#include "gtfs_parser.h"
namespace navimake {

VJ::VJ(builder & b, const std::string &line_name, const std::string &validity_pattern) : b(b){
    vj = new types::VehicleJourney();
    b.data.vehicle_journeys.push_back(vj);

    auto it = b.lines.find(line_name);
    if(it == b.lines.end()){
        vj->tmp_line = new types::Line();
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
}

VJ & VJ::operator()(const std::string & sp_name, int arrivee, int depart){
    types::StopTime * st = new types::StopTime();
    b.data.stops.push_back(st);
    auto it = b.sps.find(sp_name);
    if(it == b.sps.end()){
        st->tmp_stop_point = new types::StopPoint();        
        st->tmp_stop_point->name = sp_name;
        b.sps[sp_name] = st->tmp_stop_point;
        b.data.stop_points.push_back(st->tmp_stop_point);
        auto sa_it = b.sas.find(sp_name);
        if(sa_it == b.sas.end()) {
            st->tmp_stop_point->stop_area = new types::StopArea();
            st->tmp_stop_point->stop_area->name = sp_name;
            b.sas[sp_name] = st->tmp_stop_point->stop_area;
            b.data.stop_areas.push_back(st->tmp_stop_point->stop_area);
        } else {
            st->tmp_stop_point->stop_area = sa_it->second;
        }
    } else {
        st->tmp_stop_point = it->second;
    }

    if(depart == -1) depart = arrivee;
    st->arrival_time = arrivee;
    st->departure_time = depart;
    st->vehicle_journey = vj;
    st->order = vj->stop_time_list.size();
    vj->stop_time_list.push_back(st);

    return *this;
}

SA::SA(builder & b, const std::string & sa_name, double x, double y) : b(b) {
    sa = new types::StopArea();
    b.data.stop_areas.push_back(sa);
    sa->name = sa_name;
    sa->coord.x = x;
    sa->coord.y = y;
    b.sas[sa_name] = sa;
}

SA & SA::operator()(const std::string & sp_name, double x, double y){
    types::StopPoint * sp = new types::StopPoint();
    b.data.stop_points.push_back(sp);
    sp->name = sp_name;
    sp->coord.x = x;
    sp->coord.y = y;
    sp->stop_area = this->sa;
    b.sps[sp_name] = sp;
    return *this;
}

VJ builder::vj(const std::string &line_name, const std::string &validity_pattern){
    return VJ(*this, line_name, validity_pattern);
}

SA builder::sa(const std::string &name, double x, double y){
    return SA(*this, name, x, y);
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

navitia::type::PT_Data builder::build() {
    navitia::type::PT_Data result;
    connectors::build_routes(data);
    connectors::build_route_points(data);

    data.clean();
    data.sort();
    data.transform(result);

    return result;
}




}
