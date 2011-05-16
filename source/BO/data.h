#pragma once
#include "types.h"
#include <boost/foreach.hpp>
namespace BO{

    class Data{
    public:
        std::vector<BO::types::Network*> networks;
        std::vector<BO::types::ModeType*> mode_types;
        std::vector<BO::types::Line*> lines;
        std::vector<BO::types::Mode*> modes;
        std::vector<BO::types::City*> cities;
        std::vector<BO::types::StopArea*> stop_areas;
        std::vector<BO::types::StopPoint*> stop_points;
        std::vector<BO::types::VehicleJourney*> vehicle_journeys;
        std::vector<BO::types::Route*> routes;
        std::vector<BO::types::StopTime*> stops;
        std::vector<BO::types::Connection*> connections;


        ~Data(){
            BOOST_FOREACH(BO::types::Network* network, networks){
                delete network;
            }
            BOOST_FOREACH(BO::types::ModeType* mode_type, mode_types){
                delete mode_type;
            }
            BOOST_FOREACH(BO::types::Line* line, lines){
                delete line;
            }
            BOOST_FOREACH(BO::types::Mode* mode, modes){
                delete mode;
            }
            BOOST_FOREACH(BO::types::City* city, cities){
                delete city;
            }
            BOOST_FOREACH(BO::types::StopArea* stop_area, stop_areas){
                delete stop_area;
            }
            BOOST_FOREACH(BO::types::StopPoint* stop_point, stop_points){
                delete stop_point;
            }
            BOOST_FOREACH(BO::types::VehicleJourney* vehicle_journey, vehicle_journeys){
                delete vehicle_journey;
            }
            BOOST_FOREACH(BO::types::Route* route, routes){
                delete route;
            }
            BOOST_FOREACH(BO::types::StopTime* stop, stops){
                delete stop;
            }
            BOOST_FOREACH(BO::types::Connection* connection, connections){
                delete connection;
            }


        }

    };
}
