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

#include <random>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/for_each.hpp>

namespace navitia{namespace type {


void PT_Data::sort(){

#define SORT_AND_INDEX(type_name, collection_name)\
    std::stable_sort(collection_name.begin(), collection_name.end(), Less());\
    std::for_each(collection_name.begin(), collection_name.end(), Indexer<nt::idx_t>());

    ITERATE_NAVITIA_PT_TYPES(SORT_AND_INDEX)

#undef SORT_AND_INDEX

    std::stable_sort(stop_point_connections.begin(), stop_point_connections.end());
    std::for_each(stop_point_connections.begin(), stop_point_connections.end(), Indexer<idx_t>());

    for(auto* vj: this->vehicle_journeys){
        std::stable_sort(vj->stop_time_list.begin(), vj->stop_time_list.end());
    }
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

template<typename T>
void fill_ext_code_map(navitia::type::ext_codes_map_type& ext_codes_map, const T& range, const pbnavitia::PlaceCodeRequest::Type& type) {
    for (auto atom: range)
        for (auto type_code: atom->codes)
            ext_codes_map[type][type_code.first][type_code.second] = atom->uri;
}

void PT_Data::build_uri() {
#define NORMALIZE_EXT_CODE(type_name, collection_name) for(auto element : collection_name) collection_name##_map[element->uri] = element;
    ITERATE_NAVITIA_PT_TYPES(NORMALIZE_EXT_CODE)
    fill_ext_code_map(ext_codes_map, stop_areas, pbnavitia::PlaceCodeRequest::StopArea);
    fill_ext_code_map(ext_codes_map, networks, pbnavitia::PlaceCodeRequest::Network);
    fill_ext_code_map(ext_codes_map, companies, pbnavitia::PlaceCodeRequest::Company);
    fill_ext_code_map(ext_codes_map, lines, pbnavitia::PlaceCodeRequest::Line);
    fill_ext_code_map(ext_codes_map, routes, pbnavitia::PlaceCodeRequest::Route);
    fill_ext_code_map(ext_codes_map, vehicle_journeys, pbnavitia::PlaceCodeRequest::VehicleJourney);
    fill_ext_code_map(ext_codes_map, stop_points, pbnavitia::PlaceCodeRequest::StopPoint);
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


nt::JourneyPattern* PT_Data::get_or_create_journey_pattern(const nt::JourneyPatternKey& key){
    const auto& it = journey_patterns_pool.find(key);
    if (it != journey_patterns_pool.cend()) {
        return it->second;
    }
    auto new_jp = new JourneyPattern{};

    new_jp->is_frequence = key.is_frequence;
    new_jp->route = key.route;
    new_jp->commercial_mode = key.commercial_mode;
    new_jp->physical_mode = key.physical_mode;
    new_jp->odt_properties = key.odt_properties;
    new_jp->name = key.name;
    auto new_uri = key.uri + ":adapted-" + std::to_string(journey_patterns.size()) ;

    // the new_uri may be already existent, to guarantee its unicity,
    // we add a random integer at the end of the uri.
    // Can we find a more proper way to do that?
    if (contains(journey_patterns_map, new_uri)) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(100, 999);
        new_uri = new_uri + "-" + std::to_string(dis(gen));
    }
    new_jp->uri = new_uri;
    new_jp->idx = journey_patterns.size() - 1;

    journey_patterns.push_back(new_jp);
    journey_patterns_map[new_jp->uri] = new_jp;
    journey_patterns_pool[key] = new_jp;

    for (auto* stop_point: key.stop_points) {

        auto new_jp_point = new nt::JourneyPatternPoint();

        new_jp_point->journey_pattern = new_jp;
        new_jp_point->stop_point = stop_point;
        // TODO: copy LineString

        new_jp_point->idx = journey_pattern_points.size();
        new_jp_point->uri = key.uri + ":" + std::to_string(new_jp->journey_pattern_point_list.size());
        new_jp_point->stop_point->journey_pattern_point_list.push_back(new_jp_point);

        new_jp_point->order = new_jp->journey_pattern_point_list.size();
        journey_pattern_points.push_back(new_jp_point);
        journey_pattern_points_map[new_jp_point->uri] = new_jp_point;
        new_jp->journey_pattern_point_list.push_back(new_jp_point);
    }
    return new_jp;
}

void PT_Data::remove_journey_pattern_from_jp_pool(const JourneyPattern& jp){

    std::vector<StopPoint*> stop_points_for_key{};
    boost::for_each(jp.journey_pattern_point_list, [&](const JourneyPatternPoint* jpp){
        stop_points_for_key.push_back(jpp->stop_point);
    });

    auto key = JourneyPatternKey{jp, std::move(stop_points_for_key)};
    journey_patterns_pool.erase(key);
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

    for (auto metavj: meta_vj) {
        delete metavj.second;
    }
    for (auto cal: associated_calendars) {
        delete cal;
    }
}
}}
