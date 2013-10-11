#include "pt_data.h"
#include "utils/functions.h"
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


void PT_Data::sort(){
#define SORT_AND_INDEX(type_name, collection_name) std::sort(collection_name.begin(), collection_name.end(), Less());\
    std::for_each(collection_name.begin(), collection_name.end(), Indexer<nt::idx_t>());
    ITERATE_NAVITIA_PT_TYPES(SORT_AND_INDEX)

    std::sort(journey_pattern_point_connections.begin(), journey_pattern_point_connections.end());
    std::for_each(journey_pattern_point_connections.begin(), journey_pattern_point_connections.end(), Indexer<idx_t>());

    std::sort(stop_point_connections.begin(), stop_point_connections.end());
    std::for_each(stop_point_connections.begin(), stop_point_connections.end(), Indexer<idx_t>());

    for(auto* vj: this->vehicle_journeys){
        std::sort(vj->stop_time_list.begin(), vj->stop_time_list.end(), Less());
    }
}


//void PT_Data::build_autocomplete(const std::map<std::string, std::string> & map_alias, const std::map<std::string, std::string> & map_synonymes){
void PT_Data::build_autocomplete(const navitia::georef::GeoRef & georef){
    for(const StopArea* sa : this->stop_areas){
        // A ne pas ajouter dans le disctionnaire si pas ne nom ou n'a pas d'admin
        if ((!sa->name.empty()) && (sa->admin_list.size() > 0)){
            std::string key="";
            for( navitia::georef::Admin* admin : sa->admin_list){
                if (admin->level ==8){key +=" " + admin->name;}
            }
            this->stop_area_autocomplete.add_string(sa->name + " " + key, sa->idx, georef.alias, georef.synonymes);
        }
    }
    this->stop_area_autocomplete.build();
    //this->stop_area_autocomplete.compute_score((*this), georef, type::Type_e::StopArea);

    for(const StopPoint* sp : this->stop_points){
        // A ne pas ajouter dans le disctionnaire si pas ne nom ou n'a pas d'admin
        if ((!sp->name.empty()) && (sp->admin_list.size() > 0)){
            std::string key="";
            for(navitia::georef::Admin* admin : sp->admin_list){
                if (admin->level == 8){key += key + " " + admin->name;}
            }
            this->stop_point_autocomplete.add_string(sp->name + " " + key, sp->idx, georef.alias, georef.synonymes);
        }
    }
    this->stop_point_autocomplete.build();

    for(const Line* line : this->lines){
        if (!line->name.empty()){
            this->line_autocomplete.add_string(line->name, line->idx, georef.alias, georef.synonymes);
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
    this->stop_area_autocomplete.compute_score((*this), georef, type::Type_e::StopArea);
    this->stop_point_autocomplete.compute_score((*this), georef, type::Type_e::StopPoint);
}


void PT_Data::build_proximity_list() {
    for(const StopArea* stop_area : this->stop_areas){
        this->stop_area_proximity_list.add(stop_area->coord, stop_area->idx);
    }
    this->stop_area_proximity_list.build();

    for(const StopPoint* stop_point : this->stop_points){
        this->stop_point_proximity_list.add(stop_point->coord, stop_point->idx);
    }
    this->stop_point_proximity_list.build();
}

void PT_Data::build_uri() {
#define NORMALIZE_EXT_CODE(type_name, collection_name) for(auto &element : collection_name) collection_name##_map[element->uri] = element;
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
    auto func_delete = 
#define DELETE_PTDATA(type_name, collection_name) \
        std::for_each(collection_name.begin(), collection_name.end(),\
                [](type_name* obj){delete obj;});
    ITERATE_NAVITIA_PT_TYPES(DELETE_PTDATA)

    for(StopTime* st : stop_times) {
        delete st;
    }
}
}}
