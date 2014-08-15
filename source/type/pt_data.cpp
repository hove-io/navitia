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
namespace navitia{namespace type {


PT_Data& PT_Data::operator=(PT_Data&& other){
#define COPY_FROM_OTHER(type_name, collection_name) collection_name = other.collection_name; collection_name##_map = other.collection_name##_map;
    ITERATE_NAVITIA_PT_TYPES(COPY_FROM_OTHER)

    stop_point_connections = other.stop_point_connections;
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


void PT_Data::sort(){
#define SORT_AND_INDEX(type_name, collection_name) size_t collection_name##_size = collection_name.size();\
    std::sort(collection_name.begin(), collection_name.end(), Less());\
    BOOST_ASSERT(collection_name.size() == collection_name##_size);\
    std::for_each(collection_name.begin(), collection_name.end(), Indexer<nt::idx_t>());\
    BOOST_ASSERT(collection_name.size() == collection_name##_size);
    ITERATE_NAVITIA_PT_TYPES(SORT_AND_INDEX)

    size_t stop_point_connections_size = stop_point_connections.size();
    std::sort(stop_point_connections.begin(), stop_point_connections.end());
    BOOST_ASSERT(stop_point_connections.size() == stop_point_connections_size);
    std::for_each(stop_point_connections.begin(), stop_point_connections.end(), Indexer<idx_t>());
    BOOST_ASSERT(stop_point_connections.size() == stop_point_connections_size);

    for(auto* vj: this->vehicle_journeys){
        size_t stop_times_size = vj->stop_time_list.size();
        std::sort(vj->stop_time_list.begin(), vj->stop_time_list.end(), Less());
        BOOST_ASSERT(vj->stop_time_list.size() == stop_times_size);
    }
    #define ASSERT_SIZE(type_name, collection_name) BOOST_ASSERT(collection_name.size() == collection_name##_size);
    ITERATE_NAVITIA_PT_TYPES(ASSERT_SIZE)
}


void PT_Data::build_autocomplete(const navitia::georef::GeoRef & georef){
    this->stop_area_autocomplete.clear();
    for(const StopArea* sa : this->stop_areas){
        // A ne pas ajouter dans le disctionnaire si pas ne nom
        //if ((!sa->name.empty()) && (sa->admin_list.size() > 0)){
        if ((!sa->name.empty()) && (sa->visible)) {
            std::string key="";
            for( navitia::georef::Admin* admin : sa->admin_list){
                if (admin->level ==8){key +=" " + admin->name;}
            }
            this->stop_area_autocomplete.add_string(sa->name + " " + key, sa->idx, georef.synonyms);
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
            this->stop_point_autocomplete.add_string(sp->name + " " + key, sp->idx, georef.synonyms);
        }
    }
    this->stop_point_autocomplete.build();

    this->line_autocomplete.clear();
    for(const Line* line : this->lines){
        if (!line->name.empty()){
            this->line_autocomplete.add_string(line->name, line->idx, georef.synonyms);
        }
    }
    this->line_autocomplete.build();
}

void PT_Data::compute_score_autocomplete(navitia::georef::GeoRef& georef){

    //Commencer par calculer le score des admin
    georef.fl_admin.compute_score((*this), georef, type::Type_e::Admin);
    //Affecter le score de chaque admin à ses ObjectTC
    georef.fl_way.compute_score((*this), georef, type::Type_e::Way);
    georef.fl_poi.compute_score((*this), georef, type::Type_e::POI);
    this->stop_point_autocomplete.compute_score((*this), georef, type::Type_e::StopPoint);
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

PT_Data::~PT_Data() {
#define DELETE_PTDATA(type_name, collection_name) \
        std::for_each(collection_name.begin(), collection_name.end(),\
                [](type_name* obj){delete obj;});
    ITERATE_NAVITIA_PT_TYPES(DELETE_PTDATA)

    for(StopTime* st : stop_times) {
        delete st;
    }
    for (auto metavj: meta_vj) {
        delete metavj.second;
    }
    for (auto cal: associated_calendars) {
        delete cal;
    }
}
}}
