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

#include "pt_data.h"
#include "utils/functions.h"

#include <boost/range/algorithm/find_if.hpp>

namespace navitia { namespace type {

ValidityPattern* PT_Data::get_or_create_validity_pattern(const ValidityPattern& vp_ref) {
    for (auto vp : validity_patterns) {
        if (vp->days == vp_ref.days && vp->beginning_date == vp_ref.beginning_date) {
            return vp;
        }
    }
    auto vp = new nt::ValidityPattern();
    vp->idx = validity_patterns.size();
    vp->uri = make_adapted_uri(vp->uri);
    vp->beginning_date = vp_ref.beginning_date;
    vp->days = vp_ref.days;
    validity_patterns.push_back(vp);
    validity_patterns_map[vp->uri] = vp;
    return vp;
}

void PT_Data::sort(){

#define SORT_AND_INDEX(type_name, collection_name)\
    std::stable_sort(collection_name.begin(), collection_name.end(), Less());\
    std::for_each(collection_name.begin(), collection_name.end(), Indexer<nt::idx_t>());
    ITERATE_NAVITIA_PT_TYPES(SORT_AND_INDEX)
#undef SORT_AND_INDEX

    std::stable_sort(stop_point_connections.begin(), stop_point_connections.end());
    std::for_each(stop_point_connections.begin(), stop_point_connections.end(), Indexer<idx_t>());
}


void PT_Data::build_autocomplete(const navitia::georef::GeoRef & georef){
    this->stop_area_autocomplete.clear();
    for(const StopArea* sa : this->stop_areas){
        // A ne pas ajouter dans le disctionnaire si pas ne nom
        if ((!sa->name.empty()) && (sa->visible)) {
            std::string key="";
            for( navitia::georef::Admin* admin : sa->admin_list){
                if (admin->level ==8){key +=" " + admin->name;}
            }
            this->stop_area_autocomplete.add_string(sa->name + " " + key, sa->idx, georef.ghostwords, georef.synonyms);
        }
    }
    this->stop_area_autocomplete.build();

    this->stop_point_autocomplete.clear();
    for(const StopPoint* sp : this->stop_points){
        // A ne pas ajouter dans le disctionnaire si pas ne nom
        if ((!sp->name.empty()) && ((sp->stop_area == nullptr) || (sp->stop_area->visible))) {
            std::string key="";
            for(navitia::georef::Admin* admin : sp->admin_list){
                if (admin->level == 8){key += key + " " + admin->name;}
            }
            this->stop_point_autocomplete.add_string(sp->name + " " + key, sp->idx, georef.ghostwords, georef.synonyms);
        }
    }
    this->stop_point_autocomplete.build();

    this->line_autocomplete.clear();
    for(const Line* line : this->lines){
        if (!line->name.empty()){
            std::string key="";
            if (line->network){key = line->network->name;}
            if (line->commercial_mode) {key += " " + line->commercial_mode->name;}
            key += " " + line->code;
            this->line_autocomplete.add_string(key + " " + line->name, line->idx, georef.ghostwords, georef.synonyms);
        }
    }
    this->line_autocomplete.build();

    this->network_autocomplete.clear();
    for(const Network* network : this->networks){
        if (!network->name.empty()){
            this->network_autocomplete.add_string(network->name, network->idx, georef.ghostwords, georef.synonyms);
        }
    }
    this->network_autocomplete.build();

    this->mode_autocomplete.clear();
    for(const CommercialMode* mode : this->commercial_modes){
        if (!mode->name.empty()){
            this->mode_autocomplete.add_string(mode->name, mode->idx, georef.ghostwords, georef.synonyms);
        }
    }
    this->mode_autocomplete.build();

    this->route_autocomplete.clear();
    for(const Route* route : this->routes){
        if (!route->name.empty()){
            std::string key="";
            if (route->line){
                if (route->line->network){key = route->line->network->name;}
                if (route->line->commercial_mode) {key += " " + route->line->commercial_mode->name;}
                key += " " + route->line->code;
            }
            this->route_autocomplete.add_string(key + " " + route->name, route->idx, georef.ghostwords, georef.synonyms);
        }
    }
    this->route_autocomplete.build();
}

void PT_Data::compute_score_autocomplete(navitia::georef::GeoRef& georef){
    //Compute admin score using stop_point count in each admin
    georef.fl_admin.compute_score((*this), georef, type::Type_e::Admin);
    //use the score of each admin for it's objects like "POI", "way" and "stop_point"
    georef.fl_way.compute_score((*this), georef, type::Type_e::Way);
    georef.fl_poi.compute_score((*this), georef, type::Type_e::POI);
    this->stop_point_autocomplete.compute_score((*this), georef, type::Type_e::StopPoint);
    //Compute stop_area score using it's stop_point count
    this->stop_area_autocomplete.compute_score((*this), georef, type::Type_e::StopArea);
}


void PT_Data::build_proximity_list() {
    this->stop_area_proximity_list.clear();
    for(const StopArea* stop_area : this->stop_areas){
        this->stop_area_proximity_list.add(stop_area->coord, stop_area->idx);
    }
    this->stop_area_proximity_list.build();

    this->stop_point_proximity_list.clear();
    for(const StopPoint* stop_point : this->stop_points){
        this->stop_point_proximity_list.add(stop_point->coord, stop_point->idx);
    }
    this->stop_point_proximity_list.build();
}

void PT_Data::build_admins_stop_areas(){
    for(navitia::type::StopPoint* stop_point : this->stop_points){
        if(!stop_point->stop_area){
            continue;
        }
        for(navitia::georef::Admin* admin : stop_point->admin_list){
            auto find_predicate = [&](navitia::georef::Admin* adm) {
                return adm->idx == admin->idx;
            };
            auto it = std::find_if(stop_point->stop_area->admin_list.begin(),
                                   stop_point->stop_area->admin_list.end(),
                                   find_predicate);
            if(it == stop_point->stop_area->admin_list.end()){
                stop_point->stop_area->admin_list.push_back(admin);
            }
        }
    }
}

void PT_Data::build_uri() {
#define NORMALIZE_EXT_CODE(type_name, collection_name) for(auto element : collection_name) collection_name##_map[element->uri] = element;
    ITERATE_NAVITIA_PT_TYPES(NORMALIZE_EXT_CODE)
}

/** Foncteur fixe le membre "idx" d'un objet en incrémentant toujours de 1
      *
      * Cela permet de numéroter tous les objets de 0 à n-1 d'un vecteur de pointeurs
      */
struct Indexer{
    idx_t idx;
    Indexer(): idx(0){}

    template<class T>
    void operator()(T* obj){obj->idx = idx; idx++;}
};

void PT_Data::index(){
#define INDEX(type_name, collection_name) std::for_each(collection_name.begin(), collection_name.end(), Indexer());
    ITERATE_NAVITIA_PT_TYPES(INDEX)
}

void PT_Data::clean_weak_impacts() {
    for (const auto& obj: stop_points) { obj->clean_weak_impacts(); }
    for (const auto& obj: stop_areas) { obj->clean_weak_impacts(); }
    for (const auto& obj: networks) { obj->clean_weak_impacts(); }
    for (const auto& obj: lines) { obj->clean_weak_impacts(); }
    for (const auto& obj: routes) { obj->clean_weak_impacts(); }
    for (const auto& obj: meta_vjs) { obj->clean_weak_impacts(); }
}

Indexes
PT_Data::get_impacts_idx(const std::vector<boost::shared_ptr<disruption::Impact>>& impacts) const {
    Indexes result;
    idx_t i = 0;
    const auto & impacts_pool = disruption_holder.get_weak_impacts();
    for (const auto& impact: impacts_pool) {
        auto impact_sptr = impact.lock();
        assert(impact_sptr);
        if (navitia::contains(impacts, impact_sptr)){
            result.insert(i); //TODO use bulk insert ?
        }
        ++i;
    }
    return result;
}

const StopPointConnection*
PT_Data::get_stop_point_connection(const StopPoint& from, const StopPoint& to) const {
    const auto& connections = from.stop_point_connection_list;
    auto is_the_one = [&](const type::StopPointConnection* conn) {
        return &from == conn->departure && &to == conn->destination;
    };
    const auto search = boost::find_if(connections, is_the_one);
    if (search == connections.end()) {
        return nullptr;
    } else {
        return *search;
    }
}


PT_Data::~PT_Data() {
    //big uggly hack :(
    // the vj are objects owned by the jp,
    // TODO change all pt object (but vj and st) to unique ptr!
    vehicle_journeys.clear();


#define DELETE_PTDATA(type_name, collection_name) \
        std::for_each(collection_name.begin(), collection_name.end(),\
                [](type_name* obj){delete obj;});
    ITERATE_NAVITIA_PT_TYPES(DELETE_PTDATA)

    for (auto conn: stop_point_connections) {
        delete conn;
    }
    for (auto cal: associated_calendars) {
        delete cal;
    }
}
}}
