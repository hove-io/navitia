#include "time_dependent.h"
#include <boost/foreach.hpp>
namespace navitia { namespace routing {

TimeDependent::TimeDependent(const type::PT_Data & data) : data(data),
       // On crée un nœud par route point, deux par stopArea, deux par stopPoint
       graph(data.route_points.size() + 2 * data.stop_areas.size() + 2* data.stop_points.size()),
       stop_area_offset_dep(data.route_points.size()),
       stop_area_offset_arr(stop_area_offset_dep + data.stop_areas.size()),
       stop_point_offset_dep(stop_area_offset_arr + data.stop_areas.size()),
       stop_point_offset_arr(stop_point_offset_dep + data.stop_points.size())
   {}

void  TimeDependent::build_graph(){
    // 1. On rajoute un arc pour chaque segment de route
    BOOST_FOREACH(type::Route route, data.routes){
        for(size_t i = 1; i < route.route_point_list.size(); ++i){
            boost::add_edge(route.route_point_list[i-1], route.route_point_list[i], graph);
        }
    }

    BOOST_FOREACH(type::StopPoint stop_point, data.stop_points){
        // 2. On ajoute les correspondances entre routepoints d'un même stopPoint
        BOOST_FOREACH(type::idx_t rp_idx1, stop_point.route_point_list){
            BOOST_FOREACH(type::idx_t rp_idx2, stop_point.route_point_list){
                if(rp_idx1 != rp_idx2){
                    // 120 secondes pour une correspondance intra-stoppoint
                    boost::add_edge(rp_idx1, rp_idx2, Edge(120), graph);
                }
            }
        }

        // 3. On relie les stoppoints aux stop_areas
        boost::add_edge(stop_point.stop_area_idx + stop_area_offset_dep, stop_point.idx + stop_point_offset_dep, Edge(60), graph);
        boost::add_edge(stop_point.idx + stop_point_offset_arr, stop_point.stop_area_idx + stop_area_offset_arr, Edge(60), graph);
    }


    // 3. On rajoute un arc entre route_points et stop_points de durée constante
    BOOST_FOREACH(type::RoutePoint route_point, data.route_points){
        // Deux minutes de battement à l'arrivée
        boost::add_edge(route_point.stop_point_idx + stop_point_offset_dep, route_point.idx, Edge(120), graph);
        // 0 à l'arrivée
        boost::add_edge(route_point.idx, route_point.stop_point_idx + stop_point_offset_arr, Edge(0), graph);
    }

    // 4. On rajoute les horaires
    BOOST_FOREACH(type::VehicleJourney vj, data.vehicle_journeys){
        for(size_t i = 1; i < vj.stop_time_list.size(); ++i){
            type::StopTime stop1 = data.stop_times[vj.stop_time_list[vj.stop_time_list[i-1]]];
            type::StopTime stop2 = data.stop_times[vj.stop_time_list[vj.stop_time_list[i]]];
            ValidityPatternTime vpt1(vj.validity_pattern_idx, stop1.departure_time);
            ValidityPatternTime vpt2(vj.validity_pattern_idx, stop2.departure_time);
            bool b;
            edge_t e;
            boost::tie(e, b) = boost::edge(stop1.route_point_idx, stop2.route_point_idx, graph);
            BOOST_ASSERT(b);
            graph[e].add(vpt1, vpt2);
        }
    }
}
}}
