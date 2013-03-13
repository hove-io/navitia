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


VJ::VJ(builder & b, const std::string &line_name, const std::string &validity_pattern, const std::string &block_id, bool wheelchair_boarding) : b(b){
    vj = new types::VehicleJourney();
    b.data.vehicle_journeys.push_back(vj);

    auto it = b.lines.find(line_name);
    if(it == b.lines.end()){
        vj->tmp_line = new types::Line();
        vj->tmp_line->uri = line_name;
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
    vj->wheelchair_boarding = wheelchair_boarding;
    if(!b.data.physical_modes.empty())
        vj->physical_mode = b.data.physical_modes.front();

    if(!b.data.companies.empty())
        vj->company = b.data.companies.front();
}

VJ & VJ::operator()(const std::string & sp_name, int arrivee, int depart, uint32_t local_trafic_zone, bool drop_off_allowed, bool pick_up_allowed){
    types::StopTime * st = new types::StopTime();
    b.data.stops.push_back(st);
    auto it = b.sps.find(sp_name);
    if(it == b.sps.end()){
        st->tmp_stop_point = new types::StopPoint();        
        st->tmp_stop_point->name = sp_name;
        st->tmp_stop_point->uri = sp_name;

        if(!b.data.networks.empty())
            st->tmp_stop_point->network = b.data.networks.front();

        b.sps[sp_name] = st->tmp_stop_point;
        b.data.stop_points.push_back(st->tmp_stop_point);
        auto sa_it = b.sas.find(sp_name);
        if(sa_it == b.sas.end()) {
            st->tmp_stop_point->stop_area = new types::StopArea();
            st->tmp_stop_point->stop_area->name = sp_name;
            st->tmp_stop_point->stop_area->uri = sp_name;
            st->tmp_stop_point->stop_area->wheelchair_boarding = true;
            b.sas[sp_name] = st->tmp_stop_point->stop_area;
            b.data.stop_areas.push_back(st->tmp_stop_point->stop_area);
            st->tmp_stop_point->wheelchair_boarding = true;
        } else {
            st->tmp_stop_point->stop_area = sa_it->second;
            st->tmp_stop_point->wheelchair_boarding = st->tmp_stop_point->stop_area->wheelchair_boarding;
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
    st->wheelchair_boarding = st->vehicle_journey->wheelchair_boarding;

    return *this;
}

SA::SA(builder & b, const std::string & sa_name, double x, double y, bool wheelchair_boarding) : b(b) {
    sa = new types::StopArea();
    b.data.stop_areas.push_back(sa);
    sa->name = sa_name;
    sa->uri = sa_name;
    sa->coord.set_lon(x);
    sa->coord.set_lat(y);
    sa->wheelchair_boarding = wheelchair_boarding;
    b.sas[sa_name] = sa;
}

SA & SA::operator()(const std::string & sp_name, double x, double y, bool wheelchair_boarding){
    types::StopPoint * sp = new types::StopPoint();
    b.data.stop_points.push_back(sp);
    sp->name = sp_name;
    sp->uri = sp_name;
    sp->wheelchair_boarding = wheelchair_boarding;
    sa->coord.set_lon(x);
    sa->coord.set_lat(y);
    sp->wheelchair_boarding = wheelchair_boarding;
    sp->stop_area = this->sa;
    b.sps[sp_name] = sp;
    return *this;
}

VJ builder::vj(const std::string &line_name, const std::string &validity_pattern, const std::string & block_id, const bool wheelchair_boarding){
    return VJ(*this, line_name, validity_pattern, block_id, wheelchair_boarding);
}

SA builder::sa(const std::string &name, double x, double y, const bool wheelchair_boarding){
    return SA(*this, name, x, y, wheelchair_boarding);
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

 void builder::generate_dummy_basis() {
    types::Country *country = new types::Country();
    this->data.countries.push_back(country);
    country->name = "base_country";
    country->uri = "country:base_country";

    types::District *district = new types::District();
    this->data.districts.push_back(district);
    district->name = "base_district";
    district->uri = "district:base_district";
    district->country = country;
     

    types::Department *department = new types::Department();
    this->data.departments.push_back(department);
    department->name= "base_department";
    department->uri = "department:base_department";
    department->district = district;    

    types::City *city = new types::City();
    this->data.cities.push_back(city);
    city->name = "base_city";
    city->uri = "city:base_city";
    city->department = department;

    types::Company *company = new types::Company();
    this->data.companies.push_back(company);
    company->name = "base_company";
    company->uri = "company:base_company";

    types::Network *network = new types::Network();
    this->data.networks.push_back(network);
    network->name = "base_network";
    network->uri = "network:base_network";



    types::CommercialMode *commercial_mode = new types::CommercialMode();
    commercial_mode->id = "0";
    commercial_mode->name = "Tram";
    commercial_mode->uri = "0x0";
    this->data.commercial_modes.push_back(commercial_mode);

    commercial_mode = new navimake::types::CommercialMode();
    commercial_mode->id = "1";
    commercial_mode->name = "Metro";
    commercial_mode->uri = "0x1";
    this->data.commercial_modes.push_back(commercial_mode);

    commercial_mode = new types::CommercialMode();
    commercial_mode->id = "2";
    commercial_mode->name = "Rail";
    commercial_mode->uri = "0x2";
    this->data.commercial_modes.push_back(commercial_mode);

    commercial_mode = new types::CommercialMode();
    commercial_mode->id = "3";
    commercial_mode->name = "Bus";
    commercial_mode->uri = "0x3";
    this->data.commercial_modes.push_back(commercial_mode);

    commercial_mode = new types::CommercialMode();
    commercial_mode->id = "4";
    commercial_mode->name = "Ferry";
    commercial_mode->uri = "0x4";
    this->data.commercial_modes.push_back(commercial_mode);

    commercial_mode = new types::CommercialMode();
    commercial_mode->id = "5";
    commercial_mode->name = "Cable car";
    commercial_mode->uri = "0x5";
    this->data.commercial_modes.push_back(commercial_mode);

    commercial_mode = new types::CommercialMode();
    commercial_mode->id = "6";
    commercial_mode->name = "Gondola";
    commercial_mode->uri = "0x6";
    this->data.commercial_modes.push_back(commercial_mode);

    commercial_mode = new types::CommercialMode();
    commercial_mode->id = "7";
    commercial_mode->name = "Funicular";
    commercial_mode->uri = "0x7";
    this->data.commercial_modes.push_back(commercial_mode);


    BOOST_FOREACH(types::CommercialMode *mt, this->data.commercial_modes) {
        types::PhysicalMode* mode = new types::PhysicalMode();
        mode->id = mt->id;
        mode->name = mt->name;
        mode->uri = mt->uri;
        this->data.physical_modes.push_back(mode);
    }

 }

 void builder::build(navitia::type::PT_Data & pt_data) {
    data.normalize_uri();
    data.complete();
    data.clean();
    data.sort();
    data.transform(pt_data);
    pt_data.build_uri();
}




}
