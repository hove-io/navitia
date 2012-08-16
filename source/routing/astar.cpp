#include "astar.h"
#include <boost/graph/astar_search.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

namespace navitia { namespace routing { namespace astar {



Astar::Astar(const type::PT_Data & data) : data(data),
    a_graph(data.stop_areas.size() + data.stop_points.size() + data.route_points.size()),
    min_time(boost::num_vertices(a_graph)),
    stop_point_offset(data.stop_areas.size()),
    route_point_offset(stop_point_offset + data.stop_points.size())
{

}


void Astar::build_graph() {
    auto weightmap = boost::get(boost::edge_weight, a_graph);
    // 1. On relie les stoppoints aux stop_areas
    BOOST_FOREACH(type::StopPoint stop_point, data.stop_points){
        as_edge_t ae;
        bool new_edge;

        boost::tie(ae, new_edge) = boost::add_edge(stop_point.stop_area_idx, stop_point.idx + stop_point_offset, a_graph);
        if(new_edge)
            weightmap[ae] = 0;
        boost::tie(ae, new_edge) = boost::add_edge(stop_point.idx + stop_point_offset, stop_point.stop_area_idx, a_graph);
        if(new_edge)
            weightmap[ae] = 0;
    }

    // 2. On relie les route_points aux stop points
    BOOST_FOREACH(type::RoutePoint route_point, data.route_points) {
        as_edge_t ae;
        bool new_edge;

        boost::tie(ae, new_edge) = boost::add_edge(route_point.stop_point_idx, route_point.idx + route_point_offset, a_graph);
        if(new_edge)
            weightmap[ae] = 0;
        boost::tie(ae, new_edge) = boost::add_edge(route_point.idx + route_point_offset, route_point.stop_point_idx, a_graph);
        if(new_edge)
            weightmap[ae] = 0;
    }

    // 3.On met sur les arcs, entre les route points de stop areas différents, le temps minimal
    BOOST_FOREACH(auto vj, data.vehicle_journeys) {
        int prec_stid = -1;
        unsigned int route_point_depart = route_point_offset + data.route_points.size() + 1,
                     route_point_arrivee;
        BOOST_FOREACH(auto stid, vj.stop_time_list) {
            route_point_arrivee = data.stop_times.at(stid).route_point_idx + route_point_offset;
            if(prec_stid != -1) {
                if(route_point_depart > (route_point_offset + data.route_points.size()) || route_point_depart < route_point_offset) {
                    std::cout << "Bug" << std::endl;
                    exit(1);
                }
                as_edge_t ae;
                bool new_edge;

                boost::tie(ae, new_edge) = boost::add_edge(route_point_arrivee, route_point_depart, a_graph);
                BOOST_ASSERT((data.stop_times.at(stid).arrival_time - data.stop_times.at(prec_stid).departure_time)>=0);
                unsigned int duration = data.stop_times.at(stid).arrival_time - data.stop_times.at(prec_stid).departure_time;
                if(new_edge){
                    weightmap[ae] = duration;
                } else {
                    weightmap[ae] = std::min(duration, weightmap[ae]);
                }
            }
            route_point_depart = route_point_arrivee;
            prec_stid = stid;
        }
    }

    BOOST_FOREACH(auto vertex, boost::vertices(a_graph)) {
        BOOST_FOREACH(auto edge, boost::out_edges(vertex, a_graph)) {
           /* std::cout << "Weigth : " << */weightmap[edge];
        }
        if(boost::out_edges(vertex, a_graph).first == boost::out_edges(vertex, a_graph).second)
            std::cout << "Probleme ?" << std::endl;
    }
}

void Astar::build_heuristic(vertex_t destination){
  /*  if(destination > stop_area_offset)
        destination -= stop_area_offset;
    else
        std::cout << "C'est pas normal là !" << std::endl;*/
    // on travaille sur le graphe inverse : pour la destination on veut savoir le temps minimal pour l'atteindre depuis tous les nœuds
    boost::dijkstra_shortest_paths(this->a_graph, destination,
                                   boost::distance_map(&min_time[0])
            );

}



}}}
