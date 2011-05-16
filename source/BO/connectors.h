#pragma once

#include <string>
#include "data.h"

namespace BO{ namespace connectors{

struct BadFormated : public std::exception{};

class CsvFusio {
    public:
        CsvFusio(const std::string & path);
        void fill(BO::Data& data);
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

        std::map<int, BO::types::Network*> network_map;
        std::map<int, BO::types::ModeType*> mode_type_map;
        std::map<int, BO::types::Mode*> mode_map;
        std::map<std::string, BO::types::City*> city_map;
        std::map<int, BO::types::StopArea*> stop_area_map;
        std::map<int, BO::types::StopPoint*> stop_point_map;
        std::map<int, BO::types::VehicleJourney*> vehicle_journey_map;
        std::map<int, BO::types::Route*> route_map;
        std::map<int, BO::types::Line*> line_map;

        void fill_networks(BO::Data& data);
        void fill_modes_type(BO::Data& data);
        void fill_modes(BO::Data& data);
        void fill_lines(BO::Data& data);
        void fill_cities(BO::Data& data);
        void fill_stop_areas(BO::Data& data);
        void fill_stop_points(BO::Data& data);
        void fill_vehicle_journeys(BO::Data& data);
        void fill_routes(BO::Data& data);
        void fill_stops(BO::Data& data);
        void fill_connections(BO::Data& data);


};


}}// BO::connectors


