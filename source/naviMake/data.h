#pragma once
#include "types.h"
#include <boost/foreach.hpp>
namespace navimake{

    class Data{
    public:
        std::vector<navimake::types::Network*> networks;
        std::vector<navimake::types::ModeType*> mode_types;
        std::vector<navimake::types::Line*> lines;
        std::vector<navimake::types::Mode*> modes;
        std::vector<navimake::types::City*> cities;
        std::vector<navimake::types::StopArea*> stop_areas;
        std::vector<navimake::types::StopPoint*> stop_points;
        std::vector<navimake::types::VehicleJourney*> vehicle_journeys;
        std::vector<navimake::types::Route*> routes;
        std::vector<navimake::types::StopTime*> stops;
        std::vector<navimake::types::Connection*> connections;


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
            BOOST_FOREACH(navimake::types::StopTime* stop, stops){
                delete stop;
            }
            BOOST_FOREACH(navimake::types::Connection* connection, connections){
                delete connection;
            }


        }

    };
}
