#pragma once

#include <string>
#include "data.h"

namespace navimake{ namespace connectors{

struct BadFormated : public std::exception{};

class CsvFusio {
    public:
        CsvFusio(const std::string & path, const std::string & start_date);
        ~CsvFusio();
        void fill(navimake::Data& data);
    private:
        std::string path;


        

        template <class T, typename T2>
        T* find(const std::map<T2, T*>& items_map, T2 id){
            typename std::map<T2, T*>::const_iterator items_it;
            items_it = items_map.find(id);
            if(items_it != items_map.end()){
                return items_it->second;
            }
            return NULL;
        }

        std::map<int, navimake::types::Network*> network_map;
        std::map<int, navimake::types::ModeType*> mode_type_map;
        std::map<int, navimake::types::Mode*> mode_map;
        std::map<std::string, navimake::types::City*> city_map;
        std::map<int, navimake::types::StopArea*> stop_area_map;
        std::map<int, navimake::types::StopPoint*> stop_point_map;
        std::map<int, navimake::types::VehicleJourney*> vehicle_journey_map;
        std::map<int, navimake::types::Route*> route_map;
        std::map<int, navimake::types::Line*> line_map;
        std::map<std::string, navimake::types::RoutePoint*> route_point_map;

        void fill_networks(navimake::Data& data);
        void fill_modes_type(navimake::Data& data);
        void fill_modes(navimake::Data& data);
        void fill_lines(navimake::Data& data);
        void fill_cities(navimake::Data& data);
        void fill_stop_areas(navimake::Data& data);
        void fill_stop_points(navimake::Data& data);
        void fill_vehicle_journeys(navimake::Data& data);
        void fill_routes(navimake::Data& data);
        void fill_stops(navimake::Data& data);
        void fill_connections(navimake::Data& data);
        void fill_route_points(navimake::Data& data);


};


}}// navimake::connectors


