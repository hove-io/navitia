#pragma once
#include "types.h"
#include <boost/foreach.hpp>

#include "type/type.h"
#include "type/data.h"
#include "type/datetime.h"

namespace nt = navitia::type;
/** Ce namespace contient toutes les structures de données \b temporaires, à remplir par le connecteur */
namespace navimake{

template<typename T>
void normalize_uri(std::vector<T*>& vec){
    std::string prefix = navitia::type::static_data::get()->captionByType(T::type);
    for(auto* element : vec){
        element->uri = prefix + ":" + element->uri;
    }
}

bool same_journey_pattern(types::VehicleJourney * vj1, types::VehicleJourney * vj2);

/// Ajoute une connection entre deux journey_pattern_point
void  add_journey_pattern_point_connection(types::JourneyPatternPoint *rp1, types::JourneyPatternPoint *rp2, int length,
                           std::multimap<std::string, types::JourneyPatternPointConnection> &journey_pattern_point_connections);

/** Structure de donnée temporaire destinée à être remplie par un connecteur
      *
      * Les vecteurs contiennent des pointeurs vers un objet TC.
      * Les relations entre objets TC sont gèrés par des pointeurs
      *
      */
class Data{
public:
    std::vector<types::Network*> networks;
    std::vector<types::Company*> companies;
    std::vector<types::CommercialMode*> commercial_modes;
    std::vector<types::Line*> lines;
    std::vector<types::PhysicalMode*> physical_modes;
    std::vector<types::City*> cities;
    std::vector<types::Country*> countries;
    std::vector<types::StopArea*> stop_areas;
    std::vector<types::StopPoint*> stop_points;
    std::vector<types::VehicleJourney*> vehicle_journeys;
    std::vector<types::JourneyPattern*> journey_patterns;
    std::vector<types::StopTime*> stops;
    std::vector<types::Connection*> connections;
    std::vector<types::JourneyPatternPoint*> journey_pattern_points;
    std::vector<types::District*> districts;
    std::vector<types::Department*> departments;
    std::vector<types::ValidityPattern*> validity_patterns;
    std::vector<types::Route*> routes;
    std::vector<types::JourneyPatternPointConnection*> journey_pattern_point_connections;


    /** Foncteur permettant de comparer les objets en passant des pointeurs vers ces objets */
    struct Less{
        template<class T>
        bool operator() (T* x, T* y) const{
            return *x < *y;
        }
    };

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

    /**
         * trie les différentes donnée et affecte l'idx
         *
         */
    void sort();

    // Sort qui fait erreur valgrind
    struct sort_vehicle_journey_list {
        const navitia::type::PT_Data & data;
        sort_vehicle_journey_list(const navitia::type::PT_Data & data) : data(data){}
        bool operator ()(const navitia::type::idx_t & i, const  navitia::type::idx_t & j) const {
            if((data.vehicle_journeys.at(i).stop_time_list.size() > 0) && (data.vehicle_journeys.at(j).stop_time_list.size() > 0)) {
                unsigned int dt1 = data.stop_times.at(data.vehicle_journeys.at(i).stop_time_list.front()).departure_time;
                unsigned int dt2 = data.stop_times.at(data.vehicle_journeys.at(j).stop_time_list.front()).departure_time;
                unsigned int at1 = data.stop_times.at(data.vehicle_journeys.at(i).stop_time_list.back()).arrival_time;
                unsigned int at2 = data.stop_times.at(data.vehicle_journeys.at(j).stop_time_list.back()).arrival_time;
                if(dt1 != dt2)
                    return dt1 < dt2;
                else
                    return at1 < at2;
            } else
                return false;
        }
    };

    // sort qui fait out of range
    struct sort_vehicle_journey_list_rp {
        const navitia::type::PT_Data & data;
        unsigned int order;
        sort_vehicle_journey_list_rp(const navitia::type::PT_Data & data, unsigned int order) : data(data), order(order){}

        bool operator ()(navitia::type::idx_t i, navitia::type::idx_t j) const {
            if((data.vehicle_journeys.at(i).stop_time_list.size() > 0) && (data.vehicle_journeys.at(j).stop_time_list.size() > 0)) {
                navitia::type::idx_t i_stop_time_idx =  data.vehicle_journeys.at(i).stop_time_list.at(order);
                navitia::type::idx_t j_stop_time_idx =  data.vehicle_journeys.at(j).stop_time_list.at(order);
                unsigned int dt1 = data.stop_times.at(i_stop_time_idx).departure_time % nt::DateTime::NB_SECONDS_DAY;
                unsigned int dt2 = data.stop_times.at(j_stop_time_idx).departure_time % nt::DateTime::NB_SECONDS_DAY;
                return dt1 < dt2;
            } else
                return false;
        }
    };

    // sort qui fait out of range
    struct sort_vehicle_journey_list_rp_arrival {
        const navitia::type::PT_Data & data;
        unsigned int order;
        sort_vehicle_journey_list_rp_arrival(const navitia::type::PT_Data & data, unsigned int order) : data(data), order(order){}

        bool operator ()(navitia::type::idx_t i, navitia::type::idx_t j) const {
            if((data.vehicle_journeys.at(i).stop_time_list.size() > 0) && (data.vehicle_journeys.at(j).stop_time_list.size() > 0)) {
                navitia::type::idx_t i_stop_time_idx =  data.vehicle_journeys.at(i).stop_time_list.at(order);
                navitia::type::idx_t j_stop_time_idx =  data.vehicle_journeys.at(j).stop_time_list.at(order);
                unsigned int dt1 = data.stop_times.at(i_stop_time_idx).arrival_time % nt::DateTime::NB_SECONDS_DAY;
                unsigned int dt2 = data.stop_times.at(j_stop_time_idx).arrival_time % nt::DateTime::NB_SECONDS_DAY;
                return dt1 < dt2;
            } else
                return false;
        }
    };

    struct sort_journey_pattern_points_list {
        const navitia::type::PT_Data & data;
        sort_journey_pattern_points_list(const navitia::type::PT_Data & data) : data(data){}
        bool operator ()(unsigned int i, unsigned int j) const {
            return data.journey_pattern_points.at(i).order < data.journey_pattern_points.at(j).order;
        }
    };

    /// Construit les journey_patterns en retrouvant les paterns à partir des VJ
    void build_journey_patterns();

    /// Construit les journey_patternpoint
    void build_journey_pattern_points();

    /// Construit les connections pour les correspondances garanties
    void build_journey_pattern_point_connections();

    void normalize_uri();

    /**
     * Ajoute des objets
     */
    void complete();


    /**
     * supprime les objets inutiles
     */
    void clean();

    /**
          * Transforme les les pointeurs en données
          */
    void transform(navitia::type::PT_Data& data);

    /**
          * Gère les relations
          */
    void build_relations(navitia::type::PT_Data & data);

    /// Construit le contour de la région à partir des stops points
    std::string compute_bounding_box(navitia::type::PT_Data &data);
    ~Data(){
#define DELETE_ALL_ELEMENTS(type_name, collection_name) for(auto element : collection_name) delete element;
        ITERATE_NAVITIA_PT_TYPES(DELETE_ALL_ELEMENTS)
        for(navimake::types::StopTime* stop : stops){
            delete stop;
        }
    }

};
}
