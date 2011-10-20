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
        std::vector<navimake::types::ModeType*> mode_types; //OK
        std::vector<navimake::types::Line*> lines; //OK
        std::vector<navimake::types::Mode*> modes; //OK
        std::vector<navimake::types::City*> cities; //OK
        std::vector<navimake::types::StopArea*> stop_areas; //OK
        std::vector<navimake::types::StopPoint*> stop_points; //OK
        std::vector<navimake::types::VehicleJourney*> vehicle_journeys;
        std::vector<navimake::types::Route*> routes; //OK
        std::vector<navimake::types::StopTime*> stops; //OK
        std::vector<navimake::types::Connection*> connections; //OK
        std::vector<navimake::types::RoutePoint*> route_points; //OK
        std::vector<navimake::types::ValidityPattern*> validity_patterns; 


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
            navimake::types::idx_t idx;
            
            Indexer(): idx(0){}

            void operator()(T* obj){obj->idx = idx; idx++;}
        };

        /**
         * trie les différentes donnée et affecte l'idx
         *
         */
        void sort();

        /**
         * supprime les objets inutiles
         */
        void clean();

        /**
          * Transforme les les pointeurs en données
          */
        void transform(navitia::type::Data& data);

        /**
          * Gère les relations
          */
        void build_relations(navitia::type::Data & data);

        ~Data(){
            BOOST_FOREACH(navimake::types::Network* network, networks){
                delete network;
            }
            BOOST_FOREACH(navimake::types::ModeType* mode_type, mode_types){
                delete mode_type;
            }
            BOOST_FOREACH(navimake::types::Line* line, lines){
                delete line;
            }
            BOOST_FOREACH(navimake::types::Mode* mode, modes){
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
            BOOST_FOREACH(navimake::types::ValidityPattern* validity_pattern, validity_patterns){
                delete validity_pattern;
            }

        }

    };
}
