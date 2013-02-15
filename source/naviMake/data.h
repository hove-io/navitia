#pragma once
#include "types.h"
#include <boost/foreach.hpp>

#include "type/type.h"
#include "type/data.h"

/** Ce namespace contient toutes les structures de données \b temporaires, à remplir par le connecteur */
namespace navimake{

/** Structure de donnée temporaire destinée à être remplie par un connecteur
      *
      * Les vecteurs contiennent des pointeurs vers un objet TC.
      * Les relations entre objets TC sont gèrés par des pointeurs
      */
class Data{
public:
    std::vector<navimake::types::Network*> networks; //OK
    std::vector<navimake::types::CommercialMode*> mode_types; //OK
    std::vector<navimake::types::Line*> lines; //OK
    std::vector<navimake::types::PhysicalMode*> modes; //OK
    std::vector<navimake::types::City*> cities; //OK
    std::vector<navimake::types::StopArea*> stop_areas; //OK
    std::vector<navimake::types::StopPoint*> stop_points; //OK
    std::vector<navimake::types::VehicleJourney*> vehicle_journeys;
    std::vector<navimake::types::Route*> routes; //OK
    std::vector<navimake::types::StopTime*> stops; //OK
    std::vector<navimake::types::Connection*> connections; //OK
    std::vector<navimake::types::RoutePoint*> route_points; //OK
    std::vector<navimake::types::District*> districts; //OK
    std::vector<navimake::types::Department*> departments; //OK
    std::vector<navimake::types::ValidityPattern*> validity_patterns;
    std::vector<navimake::types::RoutePointConnection*> route_point_connections;


    /** Foncteur permettant de comparer les objets en passant des pointeurs vers ces objets */
    template<class T>
    struct Less{
        bool operator() (T* x, T* y) const{
            return *x < *y;
        }
    };

    /** Foncteur fixe le membre "idx" d'un objet en incrémentant toujours de 1
          *
          * Cela permet de numéroter tous les objets de 0 à n-1 d'un vecteur de pointeurs
          */
    template<class T>
    struct Indexer{
        idx_t idx;

        Indexer(): idx(0){}

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
                unsigned int dt1 = data.stop_times.at(i_stop_time_idx).departure_time % 86400;
                unsigned int dt2 = data.stop_times.at(j_stop_time_idx).departure_time % 86400;
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
                unsigned int dt1 = data.stop_times.at(i_stop_time_idx).arrival_time % 86400;
                unsigned int dt2 = data.stop_times.at(j_stop_time_idx).arrival_time % 86400;
                return dt1 < dt2;
            } else
                return false;
        }
    };

    struct sort_route_points_list {
        const navitia::type::PT_Data & data;
        sort_route_points_list(const navitia::type::PT_Data & data) : data(data){}
        bool operator ()(unsigned int i, unsigned int j) const {
            return data.route_points.at(i).order < data.route_points.at(j).order;
        }
    };

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
    std::string find_shape(navitia::type::PT_Data &data);
    ~Data(){
        BOOST_FOREACH(navimake::types::Network* network, networks){
            delete network;
        }
        BOOST_FOREACH(navimake::types::CommercialMode* mode_type, mode_types){
            delete mode_type;
        }
        BOOST_FOREACH(navimake::types::Line* line, lines){
            delete line;
        }
        BOOST_FOREACH(navimake::types::PhysicalMode* mode, modes){
            delete mode;
        }
        BOOST_FOREACH(navimake::types::City* city, cities){
            delete city;
        }
        BOOST_FOREACH(navimake::types::StopArea* stop_area, stop_areas){
            delete stop_area;
        }
        BOOST_FOREACH(navimake::types::StopPoint* stop_point, stop_points){
            delete stop_point;
        }
        BOOST_FOREACH(navimake::types::VehicleJourney* vehicle_journey, vehicle_journeys){
            delete vehicle_journey;
        }
        BOOST_FOREACH(navimake::types::Route* route, routes){
            delete route;
        }
        BOOST_FOREACH(navimake::types::RoutePoint* route_point, route_points){
            delete route_point;
        }
        BOOST_FOREACH(navimake::types::StopTime* stop, stops){
            delete stop;
        }
        BOOST_FOREACH(navimake::types::Connection* connection, connections){
            delete connection;
        }
        BOOST_FOREACH(auto district, districts){
            delete district;
        }
        BOOST_FOREACH(auto department, departments){
            delete department;
        }
        BOOST_FOREACH(navimake::types::ValidityPattern* validity_pattern, validity_patterns){
            delete validity_pattern;
        }

    }

};
}
