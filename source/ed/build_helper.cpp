/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
This file is part of Navitia,
    the software to build cool stuff with public transport.
 
Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!
  
LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
   
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.
   
You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
  
Stay tuned using
twitter @navitia 
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "build_helper.h"
#include "ed/connectors/gtfs_parser.h"
#include <boost/range/algorithm/find_if.hpp>

namespace pt = boost::posix_time;

namespace ed {

nt::MetaVehicleJourney* builder::get_or_create_metavj(const std::string name) {
    auto it = data->pt_data->meta_vj.find(name);


    if (it == data->pt_data->meta_vj.end()) {
        auto mvj = new nt::MetaVehicleJourney;
        data->pt_data->meta_vj.insert({name, mvj});
        return mvj;
    }
    return it->second;
}

static nt::JourneyPattern* get_or_create_journey_pattern(builder& b, const std::string& uri) {
    auto it = std::find_if(b.data->pt_data->journey_patterns.begin(),
                        b.data->pt_data->journey_patterns.end(),
                        [uri](nt::JourneyPattern* jp) {
                            return jp->uri == uri;
                        });
    if (it == b.data->pt_data->journey_patterns.end()) {
        auto jp = new nt::JourneyPattern;
        jp->idx = b.data->pt_data->journey_patterns.size();
        jp->uri = uri;
        jp->name = uri;
        b.data->pt_data->journey_patterns.push_back(jp);
        return jp;
    }
    return *it;
}


VJ::VJ(builder & b, const std::string &line_name, const std::string &validity_pattern,
       bool is_frequency,
       const std::string &/*block_id*/, bool wheelchair_boarding, const std::string& uri,
       std::string meta_vj_name, std::string jp_uri, const std::string& physical_mode): b(b) {
    // the vj is owned by the jp, so we need to have the jp before everything else
    auto it = b.lines.find(line_name);
    nt::Route* route = nullptr;
    if (it == b.lines.end()) {
        navitia::type::Line* line = new navitia::type::Line();
        line->idx = b.data->pt_data->lines.size();
        line->uri = line_name;
        b.lines[line_name] = line;
        line->name = line_name;
        b.data->pt_data->lines.push_back(line);

        route = new navitia::type::Route();
        route->idx = b.data->pt_data->routes.size();
        route->name = line->name;
        route->uri = line_name + ":" + std::to_string(b.data->pt_data->routes.size());
        b.data->pt_data->routes.push_back(route);
        line->route_list.push_back(route);
        route->line = line;
        b.data->pt_data->routes_map[route->uri] = route;
    } else {
        route = it->second->route_list.front();
    }
    if (jp_uri.empty()) {
        jp_uri = route->uri + ":0";
    }
    auto jp = get_or_create_journey_pattern(b, jp_uri);

    route->journey_pattern_list.push_back(jp);
    jp->route = route;

    //add physical mode
    if (!physical_mode.empty()){
        auto it = b.data->pt_data->physical_modes_map.find(physical_mode);
        if (it != b.data->pt_data->physical_modes_map.end()){
            jp->physical_mode = it->second;
        }
    }
    if(!jp->physical_mode){
        if (physical_mode.empty() && b.data->pt_data->physical_modes.size()){
            jp->physical_mode = b.data->pt_data->physical_modes.front();
        } else {
            const auto name = physical_mode.empty() ? "physical_mode:0" : physical_mode;
            jp->physical_mode = new navitia::type::PhysicalMode();
            jp->physical_mode->idx = b.data->pt_data->physical_modes.size();
            jp->physical_mode->uri = name;
            jp->physical_mode->name = "name " + name;
            b.data->pt_data->physical_modes.push_back(jp->physical_mode);
        }
    }

    jp->physical_mode->journey_pattern_list.push_back(jp);

    if (is_frequency) {
        auto f_vj = std::make_unique<nt::FrequencyVehicleJourney>();
        f_vj->idx = jp->frequency_vehicle_journey_list.size() - 1;
        vj = f_vj.get();
        jp->frequency_vehicle_journey_list.push_back(std::move(f_vj));
    } else {
        auto d_vj = std::make_unique<nt::DiscreteVehicleJourney>();
        d_vj->idx = jp->discrete_vehicle_journey_list.size() - 1;
        vj = d_vj.get();
        jp->discrete_vehicle_journey_list.push_back(std::move(d_vj));
    }

    vj->journey_pattern = jp;
    //if we have a meta_vj, we add it in that
    nt::MetaVehicleJourney* mvj;
    if (! meta_vj_name.empty()) {
        mvj = b.get_or_create_metavj(meta_vj_name);
    } else {
        mvj = b.get_or_create_metavj(uri);
    }
    mvj->theoric_vj.push_back(vj);
    vj->meta_vj = mvj;
    vj->idx = b.data->pt_data->vehicle_journeys.size();
    vj->name = "vehicle_journey " + std::to_string(vj->idx);
    vj->uri = vj->name;
    b.data->pt_data->vehicle_journeys.push_back(vj);
    b.data->pt_data->vehicle_journeys_map[vj->uri] = vj;

    nt::ValidityPattern* vp = new nt::ValidityPattern(b.begin, validity_pattern);
    auto find_vp_predicate = [&](nt::ValidityPattern* vp1) { return vp->days == vp1->days;};
    auto it_vp = std::find_if(b.data->pt_data->validity_patterns.begin(),
                        b.data->pt_data->validity_patterns.end(), find_vp_predicate);
    if(it_vp != b.data->pt_data->validity_patterns.end()) {
        delete vp;
        vj->validity_pattern = *(it_vp);
    } else {
         b.data->pt_data->validity_patterns.push_back(vp);
         vj->validity_pattern = vp;
    }
    b.vps[validity_pattern] = vj->validity_pattern;
    vj->adapted_validity_pattern = vj->validity_pattern;

    if(wheelchair_boarding){
        vj->set_vehicle(navitia::type::hasVehicleProperties::WHEELCHAIR_ACCESSIBLE);
    }
    vj->uri = uri;

    if(!b.data->pt_data->companies.empty())
        vj->company = b.data->pt_data->companies.front();
}

VJ& VJ::st_shape(const navitia::type::LineString& shape) {
    assert(shape.size() >= 2);
    assert(vj->stop_time_list.size() >= 2);
    assert(vj->stop_time_list.back().journey_pattern_point->stop_point->coord == shape.back());
    assert(vj->stop_time_list.at(vj->stop_time_list.size() - 2).journey_pattern_point->stop_point->coord
           == shape.front());
    vj->stop_time_list.back().journey_pattern_point->shape_from_prev = shape;
    return *this;
}

VJ& VJ::operator()(const std::string &stopPoint,const std::string& arrivee, const std::string& depart,
            uint16_t local_traffic_zone, bool drop_off_allowed, bool pick_up_allowed){
    return (*this)(stopPoint, pt::duration_from_string(arrivee).total_seconds(),
            pt::duration_from_string(depart).total_seconds(), local_traffic_zone,
            drop_off_allowed, pick_up_allowed);
}

VJ & VJ::operator()(const std::string & sp_name, int arrivee, int depart, uint16_t local_trafic_zone,
                    bool drop_off_allowed, bool pick_up_allowed){
    auto it = b.sps.find(sp_name);
    navitia::type::StopPoint* sp = nullptr;
    navitia::type::JourneyPatternPoint* jpp = nullptr;
    if(it == b.sps.end()){
        sp = new navitia::type::StopPoint();
        sp->idx = b.data->pt_data->stop_points.size();
        sp->name = sp_name;
        sp->uri = sp_name;
        if(!b.data->pt_data->networks.empty())
            sp->network = b.data->pt_data->networks.front();

        b.sps[sp_name] = sp;
        b.data->pt_data->stop_points.push_back(sp);
        auto sa_it = b.sas.find(sp_name);
        if(sa_it == b.sas.end()) {
            navitia::type::StopArea* sa = new navitia::type::StopArea();
            sa->idx = b.data->pt_data->stop_areas.size();
            sa->name = sp_name;
            sa->uri = sp_name;
            sa->set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);
            sp->stop_area = sa;
            b.sas[sp_name] = sa;
            b.data->pt_data->stop_areas.push_back(sa);
            sp->set_properties(sa->properties());
            sa->stop_point_list.push_back(sp);
        } else {
            sp->stop_area = sa_it->second;
            sp->coord.set_lat(sp->stop_area->coord.lat());
            sp->coord.set_lon(sp->stop_area->coord.lon());
            sp->set_properties(sa_it->second->properties());
            sa_it->second->stop_point_list.push_back(sp);
        }
    } else {
        sp = it->second;
        auto find_jpp = std::find_if(sp->journey_pattern_point_list.begin(),
                                     sp->journey_pattern_point_list.end(),
                                     [&](navitia::type::JourneyPatternPoint* jpp){return jpp->journey_pattern == vj->journey_pattern;});
        if(find_jpp != sp->journey_pattern_point_list.end())
            jpp = *find_jpp;
    }
    if(jpp == nullptr) {
        jpp = new navitia::type::JourneyPatternPoint();
        jpp->idx = b.data->pt_data->journey_pattern_points.size();
        vj->journey_pattern->journey_pattern_point_list.push_back(jpp);
        jpp->stop_point = sp;
        sp->journey_pattern_point_list.push_back(jpp);
        jpp->journey_pattern = vj->journey_pattern;
        std::stringstream ss;
        ss << "stop:" << sp->uri << "::jp:" << vj->journey_pattern->uri;
        jpp->uri = ss.str();
        if (!vj->stop_time_list.empty()) {
            jpp->shape_from_prev.push_back(vj->stop_time_list.back().journey_pattern_point->stop_point->coord);
            jpp->shape_from_prev.push_back(jpp->stop_point->coord);
        }
        b.data->pt_data->journey_pattern_points.push_back(jpp);
    }

    //on construit un nouveau journey pattern point à chaque fois
    navitia::type::StopTime st;
    st.journey_pattern_point = jpp;

    if(depart == -1) depart = arrivee;
    st.arrival_time = arrivee;
    st.departure_time = depart;
    st.vehicle_journey = vj;
    jpp->order = vj->stop_time_list.size();
    st.local_traffic_zone = local_trafic_zone;
    st.set_drop_off_allowed(drop_off_allowed);
    st.set_pick_up_allowed(pick_up_allowed);

    vj->stop_time_list.push_back(st);
    return *this;
}

SA::SA(builder & b, const std::string & sa_name, double x, double y,
       bool create_sp, bool wheelchair_boarding)
       : b(b) {
    sa = new navitia::type::StopArea();
    sa->idx = b.data->pt_data->stop_areas.size();
    b.data->pt_data->stop_areas.push_back(sa);
    sa->name = sa_name;
    sa->uri = sa_name;
    sa->coord.set_lon(x);
    sa->coord.set_lat(y);
    if(wheelchair_boarding)
        sa->set_property(types::hasProperties::WHEELCHAIR_BOARDING);
    b.sas[sa_name] = sa;

    if (create_sp) {
        auto sp = new navitia::type::StopPoint();
        sp->idx = b.data->pt_data->stop_points.size();
        b.data->pt_data->stop_points.push_back(sp);
        sp->name = "stop_point:"+ sa_name;
        sp->uri = sp->name;
        if(wheelchair_boarding)
            sp->set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);
        sp->coord.set_lon(x);
        sp->coord.set_lat(y);

        sp->stop_area = sa;
        b.sps[sp->name] = sp;
        sa->stop_point_list.push_back(sp);
    }
}

SA & SA::operator()(const std::string & sp_name, double x, double y, bool wheelchair_boarding){
    navitia::type::StopPoint * sp = new navitia::type::StopPoint();
    sp->idx = b.data->pt_data->stop_points.size();
    b.data->pt_data->stop_points.push_back(sp);
    sp->name = sp_name;
    sp->uri = sp_name;
    if(wheelchair_boarding)
        sp->set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);
    sp->coord.set_lon(x);
    sp->coord.set_lat(y);

    sp->stop_area = this->sa;
    this->sa->stop_point_list.push_back(sp);
    b.sps[sp_name] = sp;
    return *this;
}


VJ builder::vj(const std::string& line_name,
               const std::string& validity_pattern,
               const std::string& block_id,
               const bool wheelchair_boarding,
               const std::string& uri,
               const std::string& meta_vj,
               const std::string& jp_uri,
               const std::string& physical_mode){
    return vj_with_network("base_network", line_name, validity_pattern, block_id, wheelchair_boarding, uri, meta_vj, jp_uri, physical_mode);
}

VJ builder::vj_with_network(const std::string& network_name,
               const std::string& line_name,
               const std::string& validity_pattern,
               const std::string& block_id,
               const bool wheelchair_boarding,
               const std::string& uri,
               const std::string& meta_vj,
               const std::string& jp_uri,
               const std::string& physical_mode,
               bool is_frequency) {
    auto res = VJ(*this, line_name, validity_pattern, is_frequency, block_id, wheelchair_boarding, uri, meta_vj, jp_uri, physical_mode);
    auto vj = this->data->pt_data->vehicle_journeys.back();
    auto it = this->nts.find(network_name);
    if(it == this->nts.end()){
        navitia::type::Network* network = new navitia::type::Network();
        network->idx = this->data->pt_data->networks.size();
        network->uri = network_name;
        network->name = network_name;
        this->nts[network_name] = network;
        this->data->pt_data->networks.push_back(network);
        vj->journey_pattern->route->line->network = network;
        network->line_list.push_back(vj->journey_pattern->route->line);
    } else {
        vj->journey_pattern->route->line->network = it->second;

        auto line = vj->journey_pattern->route->line;
        if (boost::find_if(it->second->line_list, [line](navitia::type::Line* l) { return l->uri == line->uri;})
                == it->second->line_list.end()) {
            it->second->line_list.push_back(vj->journey_pattern->route->line);
        }
    }
    if(block_id != "") {
        block_vjs.insert(std::make_pair(block_id, vj));
    }
    return res;
}


VJ builder::frequency_vj(const std::string& line_name,
                uint32_t start_time,
                uint32_t end_time,
                uint32_t headway_secs,
                const std::string& network_name,
                const std::string& validity_pattern,
                const std::string& block_id,
                const bool wheelchair_boarding,
                const std::string& uri,
                const std::string& meta_vj,
                const std::string &jp_uri,
                const std::string& physical_mode){
    auto res = vj_with_network(network_name, line_name, validity_pattern, block_id, wheelchair_boarding, uri, meta_vj, jp_uri, physical_mode, true);
    auto vj = static_cast<nt::FrequencyVehicleJourney*>(this->data->pt_data->vehicle_journeys.back());

    //we get the last frequency vj of the jp, it's the one we just created
    vj->start_time = start_time;

    size_t nb_trips = std::ceil((end_time - start_time) / headway_secs);
    vj->end_time = start_time + ( nb_trips * headway_secs );
    vj->headway_secs = headway_secs;

    return res;
}


SA builder::sa(const std::string &name, double x, double y,
               const bool create_sp, const bool wheelchair_boarding) {
    return SA(*this, name, x, y, create_sp, wheelchair_boarding);
}


void builder::connection(const std::string & name1, const std::string & name2, float length) {
    navitia::type::StopPointConnection* connexion = new navitia::type::StopPointConnection();
    connexion->idx = data->pt_data->stop_point_connections.size();
    if(sps.count(name1) == 0 || sps.count(name2) == 0)
        return ;
    connexion->departure = (*(sps.find(name1))).second;
    connexion->destination = (*(sps.find(name2))).second;

//connexion->connection_kind = types::ConnectionType::Walking;
    connexion->duration = length;
    connexion->display_duration = length;

    data->pt_data->stop_point_connections.push_back(connexion);
    connexion->departure->stop_point_connection_list.push_back(connexion);
    connexion->destination->stop_point_connection_list.push_back(connexion);
}

 static double get_co2_emission(const std::string& uri){
     if (uri == "0x0"){
         return 4.;
     }
     if (uri == "Bss"){
         return 0.;
     }
     if (uri == "Bike"){
         return 0.;
     }
     if (uri == "Bus"){
         return 132.;
     }
     if (uri == "Car"){
         return 184.;
     }
    return -1.;
 }

 void builder::generate_dummy_basis() {
    navitia::type::Company *company = new navitia::type::Company();
    company->idx = this->data->pt_data->companies.size();
    company->name = "base_company";
    company->uri = "base_company";
    this->data->pt_data->companies.push_back(company);

    const std::string default_network_name = "base_network";
    if (this->nts.find(default_network_name) == this->nts.end()) {
        navitia::type::Network *network = new navitia::type::Network();
        network->idx = this->data->pt_data->networks.size();
        network->name = default_network_name;
        network->uri = default_network_name;
        this->data->pt_data->networks.push_back(network);
        this->nts.insert({network->uri, network});
    }

    navitia::type::CommercialMode *commercial_mode = new navitia::type::CommercialMode();
    commercial_mode->idx = this->data->pt_data->commercial_modes.size();
    commercial_mode->name = "Tram";
    commercial_mode->uri = "0x0";
    this->data->pt_data->commercial_modes.push_back(commercial_mode);

    commercial_mode = new navitia::type::CommercialMode();
    commercial_mode->idx = this->data->pt_data->commercial_modes.size();
    commercial_mode->name = "Metro";
    commercial_mode->uri = "0x1";
    this->data->pt_data->commercial_modes.push_back(commercial_mode);

    commercial_mode = new navitia::type::CommercialMode();
    commercial_mode->idx = this->data->pt_data->commercial_modes.size();
    commercial_mode->name = "Bss";
    commercial_mode->uri = "Bss";
    this->data->pt_data->commercial_modes.push_back(commercial_mode);

    commercial_mode = new navitia::type::CommercialMode();
    commercial_mode->idx = this->data->pt_data->commercial_modes.size();
    commercial_mode->name = "Bike";
    commercial_mode->uri = "Bike";
    this->data->pt_data->commercial_modes.push_back(commercial_mode);

    commercial_mode = new navitia::type::CommercialMode();
    commercial_mode->idx = this->data->pt_data->commercial_modes.size();
    commercial_mode->name = "Bus";
    commercial_mode->uri = "Bus";
    this->data->pt_data->commercial_modes.push_back(commercial_mode);

    commercial_mode = new navitia::type::CommercialMode();
    commercial_mode->idx = this->data->pt_data->commercial_modes.size();
    commercial_mode->name = "Car";
    commercial_mode->uri = "Car";
    this->data->pt_data->commercial_modes.push_back(commercial_mode);

    for(navitia::type::CommercialMode *mt : this->data->pt_data->commercial_modes) {
        navitia::type::PhysicalMode* mode = new navitia::type::PhysicalMode();
        double co2_emission;
        mode->idx = mt->idx;
        mode->name = mt->name;
        mode->uri = "physical_mode:" + mt->uri;
        co2_emission = get_co2_emission(mt->uri);
        if (co2_emission >=0.){
            mode->co2_emission = co2_emission;
        }
        this->data->pt_data->physical_modes.push_back(mode);
        this->data->pt_data->physical_modes_map[mode->uri] = mode;
    }
 }

 void builder::build(navitia::type::PT_Data & /*pt_data*/) {
    /*data.normalize_uri();
    data.complete();
    data.clean();
    data.sort();
    //data.transform(pt_data);
    pt_data->build_uri();*/
}

 void builder::build_relations(navitia::type::PT_Data & data){
     for(navitia::type::StopPoint* sp : data.stop_points){
         sp->journey_pattern_point_list.clear();
     }
     for(navitia::type::JourneyPatternPoint* jpp : data.journey_pattern_points){
         jpp->stop_point->journey_pattern_point_list.push_back(jpp);
     }
 }


void builder::build_blocks() {
    using navitia::type::VehicleJourney;

    std::map<std::string, std::vector<VehicleJourney*>> block_map;
    for (const auto& block_vj: block_vjs) {
        if (block_vj.first.empty()) { continue; }
        block_map[block_vj.first].push_back(block_vj.second);
    }

    for (auto& item: block_map) {
        auto& vehicle_journeys = item.second;
        std::sort(vehicle_journeys.begin(), vehicle_journeys.end(),
                  [](const VehicleJourney* vj1, const VehicleJourney* vj2) {
                      return vj1->stop_time_list.back().departure_time <=
                          vj2->stop_time_list.front().departure_time;
                  }
            );

        VehicleJourney* prev_vj = nullptr;
        for (auto* vj: vehicle_journeys) {
            if (prev_vj) {
                assert(prev_vj->stop_time_list.back().arrival_time
                       <= vj->stop_time_list.front().departure_time);
                prev_vj->next_vj = vj;
                vj->prev_vj = prev_vj;
            }
            prev_vj = vj;
        }
    }
}

 void builder::finish() {

     build_blocks();
     for(navitia::type::VehicleJourney* vj : this->data->pt_data->vehicle_journeys) {
         if(!vj->prev_vj) {
             vj->stop_time_list.front().set_drop_off_allowed(false);
         }
         if(!vj->next_vj) {
            vj->stop_time_list.back().set_pick_up_allowed(false);
         }
     }

     for (auto* jp: data->pt_data->journey_patterns) {
         for (auto& freq_vj: jp->frequency_vehicle_journey_list) {

             const auto start = freq_vj->stop_time_list.front().arrival_time;
             for (auto& st: freq_vj->stop_time_list) {
                 st.set_is_frequency(true); //we need to tag the stop time as a frequency stop time
                 st.arrival_time -= start;
                 st.departure_time -= start;
             }
         }
     }
 }

/*
1. Initilise the first admin in the list to all stop_area and way
2. Used for the autocomplete functional tests.
*/
 void builder::manage_admin() {
     if (!data->geo_ref->admins.empty()) {
         navitia::georef::Admin * admin = data->geo_ref->admins[0];
        for(navitia::type::StopArea* sa : data->pt_data->stop_areas){
            sa->admin_list.clear();
            sa->admin_list.push_back(admin);
        }

        for(navitia::georef::Way * way : data->geo_ref->ways) {
            way->admin_list.clear();
            way->admin_list.push_back(admin);
        }
     }
 }

 void builder::build_autocomplete() {
    data->pt_data->build_autocomplete(*(data->geo_ref));
    data->geo_ref->build_autocomplete_list();
    data->compute_labels();
}
}
