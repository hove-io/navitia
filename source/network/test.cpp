#include "network.h"
#include "routing/time_dependent.h"
#include "boost/property_map/property_map.hpp"
#include <boost/graph/graphviz.hpp>
#include <boost/graph/astar_search.hpp>
#include <sys/time.h>
#include <sys/resource.h>
#include <valgrind/callgrind.h>
#include "utils/timer.h"

using namespace network;

struct label_name {
    NW g;
    navitia::type::Data &data;
    map_tc_t map_tc;

    label_name(NW &g, navitia::type::Data &data, map_tc_t map_tc): g(g), data(data), map_tc(map_tc){}

    void operator()(std::ostream& oss,vertex_t v) {
        oss << "[label=\"";
        switch(get_n_type(v, data)) {
        case SA : oss << "SA " << get_idx(v, data, map_tc); break;
        case SP : oss << "SP " << get_idx(v, data, map_tc); break;
        case RP : oss << "RP " << get_idx(v, data, map_tc); break;
        case TA : oss << "TA " << get_idx(v, data, map_tc) << " " << get_time(v, data, g, map_tc); break;
        case TD : oss << "TD " << get_idx(v, data, map_tc) << " "<< get_time(v, data, g, map_tc); break;
        case TC : oss << "TC " << get_idx(v, data, map_tc) << " "<< get_time(v, data, g, map_tc); break;
        }
        oss << " " << v << " " << get_saidx(v, data, g, map_tc) << "\"]";
    }

};






int main(int , char** argv) {
    navitia::type::Data data;
    std::cout << "Debut chargement des données ... " << std::flush;
    {
        Timer t("Chargement des données");
        data.load_lz4("/home/vlara/navitia/jeu/poitiers/poitiers.nav");
    }
    std::cout << " Fin chargement des données" << std::endl;

    std::cout << "Nombre de stop areas : " << data.pt_data.stop_areas.size() << std::endl;
    std::cout << "Nombre de stop points : "<< data.pt_data.stop_points.size()<< std::endl;
    std::cout << "Nombre de route points : " << data.pt_data.route_points.size() << std::endl;
    std::cout << "Nombre d'horaires :" << data.pt_data.stop_times.size() << std::endl;

    std::cout << "Taille data : " << sizeof(data) << std::endl;

    std::cout << "Création et chargement du graph ..." << std::flush;
    NW g(data.pt_data.stop_areas.size() + data.pt_data.stop_points.size() + data.pt_data.route_points.size() + data.pt_data.stop_times.size() * 2);
    map_tc_t map_tc, map_td;

    boost::posix_time::ptime start, end;

    {
        Timer t("Chargement du graphe");
        charger_graph(data, g, map_tc, map_td);
        std::cout << " Fin de création et chargement du graph " << std::endl;
    }





    std::cout << "Nombre de noeuds : " << num_vertices(g) << std::endl;
    std::cout << "Nombre d'arêtes : " << num_edges(g) << std::endl;
    std::cout << "Taille EdgeDesc : " << sizeof(EdgeDesc) << std::endl;
    std::cout << "Taille etiquette : " << sizeof(etiquette) << std::endl;
    std::cout << "Taille g : " << sizeof(g) << std::endl;


    vertex_t v1, v2;

    std::cout << "v1 : " << atoi(argv[1]) << " v2 " <<  atoi(argv[2]) << std::endl;


    v2 = atoi(argv[2]);
    std::vector<vertex_t> predecessors(boost::num_vertices(g));
    std::vector<etiquette> distances(boost::num_vertices(g));
    etiquette etdebut;
    etdebut.temps = 0;
    etdebut.date_arrivee = data.pt_data.validity_patterns.at(0).slide(boost::gregorian::from_undelimited_string(argv[4]));
    etdebut.heure_arrivee = atoi(argv[3]);

    std::cout  << "Recherche du chemin entre : " << data.pt_data.stop_areas.at(atoi(argv[1])).name << " et " << data.pt_data.stop_areas.at(v2).name << " " << get_time(atoi(argv[1]), data, g, map_tc) <<  std::endl;

    boost::typed_identity_property_map<edge_t> identity;




    start = boost::posix_time::microsec_clock::local_time();
    v1 = trouver_premier_tc(atoi(argv[1]), atoi(argv[3]), data, map_td);
    std::cout << "tc : " << v1 << std::endl;
    end = boost::posix_time::microsec_clock::local_time();
    std::cout << "Recherche tc : "  << (end-start).total_milliseconds() << std::endl;
    CALLGRIND_START_INSTRUMENTATION;
    start = boost::posix_time::microsec_clock::local_time();
    try {
        boost::dijkstra_shortest_paths(g, v1,
                                       boost::predecessor_map(&predecessors[0])
                                       .weight_map(boost::get(boost::edge_bundle, g)/*identity*/)
                                       .distance_map(&distances[0])
                                       .distance_combine(combine_simple(data, g, map_tc, v1, v2, etdebut.date_arrivee))
                                       .distance_zero(etdebut)
                                       .distance_inf(etiquette::max())
                                       .distance_compare(edge_less2())
                                       .visitor(dijkstra_goal_visitor(v2, data, g, map_tc))
                                       );

    } catch(found_goal fg) { v2 = fg.v; }
//    end = boost::posix_time::microsec_clock::local_time();
//    CALLGRIND_STOP_INSTRUMENTATION;
//    CALLGRIND_DUMP_STATS;

    if(predecessors[v2] == v2) {
        std::cout << "Pas de chemin trouve" << std::endl;
    } else {
        std::cout << "J'arrive à " << get_time(v2, data, g, map_tc) << std::endl;
    }
//        std::cout << "Chemin trouve en " << (end-start).total_milliseconds() << std::endl;

//        for(vertex_t v = v2; (v!=v1); v = predecessors[v]) {
//            if(get_n_type(v, data) == TD || get_n_type(v, data) == TA) {
//                std::cout << get_n_type(v,data) << " " << get_time(v, data, g, map_tc) << " "
//                          << data.pt_data.stop_areas.at(data.pt_data.stop_points.at(data.pt_data.route_points.at(data.pt_data.stop_times.at(get_idx(v, data, map_tc)).route_point_idx).stop_point_idx).stop_area_idx).name << " ";
//                std::cout << data.pt_data.stop_points.at(data.pt_data.route_points.at(data.pt_data.stop_times.at(get_idx(v, data, map_tc)).route_point_idx).stop_point_idx).stop_area_idx;
//                std::cout << " " << data.pt_data.stop_times.at(get_idx(v, data, map_tc)).arrival_time;
//                std::cout << " " << data.pt_data.lines.at(data.pt_data.routes.at(data.pt_data.route_points.at(data.pt_data.stop_times.at(get_idx(v, data, map_tc)).route_point_idx).route_idx).line_idx).name;
//                std::cout << " t : " << distances[v].heure_arrivee << " d : " << distances[v].date_arrivee << " " << data.pt_data.route_points.at(data.pt_data.stop_times.at(get_idx(v, data, map_tc)).route_point_idx).route_idx
//                          << " vj : " << data.pt_data.stop_times.at(get_idx(v, data,map_tc)).vehicle_journey_idx << std::endl;
//            }
//        }
//    }

//    end = boost::posix_time::microsec_clock::local_time();
//    CALLGRIND_STOP_INSTRUMENTATION;
//    CALLGRIND_DUMP_STATS;

//    if(predecessors[v2] == v2) {
//        std::cout << "Pas de chemin trouve" << std::endl;
//    } else {
//        std::cout << "Chemin trouve en " << (end-start).total_milliseconds() << std::endl;

//        for(vertex_t v = v2; (v!=v1); v = predecessors[v]) {
//            if(get_n_type(v, data) == TD || get_n_type(v, data) == TA) {
//                std::cout << get_n_type(v,data) << " " << get_time(v, data, g, map_tc) << " "
//                          << data.pt_data.stop_areas.at(data.pt_data.stop_points.at(data.pt_data.route_points.at(data.pt_data.stop_times.at(get_idx(v, data, map_tc)).route_point_idx).stop_point_idx).stop_area_idx).name << " ";
//                std::cout << data.pt_data.stop_points.at(data.pt_data.route_points.at(data.pt_data.stop_times.at(get_idx(v, data, map_tc)).route_point_idx).stop_point_idx).stop_area_idx;
//                std::cout << " " << data.pt_data.stop_times.at(get_idx(v, data, map_tc)).arrival_time;
//                std::cout << " " << data.pt_data.lines.at(data.pt_data.routes.at(data.pt_data.route_points.at(data.pt_data.stop_times.at(get_idx(v, data, map_tc)).route_point_idx).route_idx).line_idx).name;
//                std::cout << " t : " << distances[v].heure_arrivee << " d : " << distances[v].date_arrivee << " " << data.pt_data.route_points.at(data.pt_data.stop_times.at(get_idx(v, data, map_tc)).route_point_idx).route_idx
//                          << " vj : " << data.pt_data.stop_times.at(get_idx(v, data,map_tc)).vehicle_journey_idx << std::endl;
//            }
//        }
//    }

//    etiquette etdebut;
//    etdebut.temps = 0;
//    etdebut.date_arrivee = data.pt_data.validity_patterns.at(0).slide(boost::gregorian::from_undelimited_string("20120215"));
//    etdebut.heure_arrivee = 28800;

//    v1 = 14796;
//    v2 = 2460;
//    v1 = trouver_premier_tc(v1, 28800, data, map_td);
//    try {
//        boost::dijkstra_shortest_paths(g, v1,
//                                       boost::predecessor_map(&predecessors[0])
//                                       .weight_map(boost::get(boost::edge_bundle, g))
//                                       .distance_map(&distances[0])
//                                       .distance_combine(combine_simple(data, g, map_tc, v1, v2, etdebut.date_arrivee))
//                                       .distance_zero(etdebut)
//                                       .distance_inf(etiquette::max())
//                                       .distance_compare(edge_less2())
//                                       .visitor(dijkstra_goal_visitor(v2, data, g, map_tc))
//                                       );

//    } catch(found_goal fg) { v2 = fg.v; }


//    v1 = 16450;
//    v2 = 15164;
//    v1 = trouver_premier_tc(v1, 28800, data, map_td);
//    try {
//        boost::dijkstra_shortest_paths(g, v1,
//                                       boost::predecessor_map(&predecessors[0])
//                                       .weight_map(boost::get(boost::edge_bundle, g))
//                                       .distance_map(&distances[0])
//                                       .distance_combine(combine_simple(data, g, map_tc, v1, v2, etdebut.date_arrivee))
//                                       .distance_zero(etdebut)
//                                       .distance_inf(etiquette::max())
//                                       .distance_compare(edge_less2())
//                                       .visitor(dijkstra_goal_visitor(v2, data, g, map_tc))
//                                       );

//    } catch(found_goal fg) { v2 = fg.v; }


//    v1 = 2306;
//    v2 = 17595;
//    v1 = trouver_premier_tc(v1, 28800, data, map_td);
//    try {
//        boost::dijkstra_shortest_paths(g, v1,
//                                       boost::predecessor_map(&predecessors[0])
//                                       .weight_map(boost::get(boost::edge_bundle, g))
//                                       .distance_map(&distances[0])
//                                       .distance_combine(combine_simple(data, g, map_tc, v1, v2, etdebut.date_arrivee))
//                                       .distance_zero(etdebut)
//                                       .distance_inf(etiquette::max())
//                                       .distance_compare(edge_less2())
//                                       .visitor(dijkstra_goal_visitor(v2, data, g, map_tc))
//                                       );

//    } catch(found_goal fg) { v2 = fg.v; }


//    v1 = 16587;
//    v2 = 4014;
//    v1 = trouver_premier_tc(v1, 28800, data, map_td);
//    try {
//        boost::dijkstra_shortest_paths(g, v1,
//                                       boost::predecessor_map(&predecessors[0])
//                                       .weight_map(boost::get(boost::edge_bundle, g))
//                                       .distance_map(&distances[0])
//                                       .distance_combine(combine_simple(data, g, map_tc, v1, v2, etdebut.date_arrivee))
//                                       .distance_zero(etdebut)
//                                       .distance_inf(etiquette::max())
//                                       .distance_compare(edge_less2())
//                                       .visitor(dijkstra_goal_visitor(v2, data, g, map_tc))
//                                       );

//    } catch(found_goal fg) { v2 = fg.v; }


//    v1 = 11484;
//    v2 = 5596;
//    v1 = trouver_premier_tc(v1, 28800, data, map_td);
//    try {
//        boost::dijkstra_shortest_paths(g, v1,
//                                       boost::predecessor_map(&predecessors[0])
//                                       .weight_map(boost::get(boost::edge_bundle, g))
//                                       .distance_map(&distances[0])
//                                       .distance_combine(combine_simple(data, g, map_tc, v1, v2, etdebut.date_arrivee))
//                                       .distance_zero(etdebut)
//                                       .distance_inf(etiquette::max())
//                                       .distance_compare(edge_less2())
//                                       .visitor(dijkstra_goal_visitor(v2, data, g, map_tc))
//                                       );

//    } catch(found_goal fg) { v2 = fg.v; }


//    v1 = 1771;
//    v2 = 9938;
//    v1 = trouver_premier_tc(v1, 28800, data, map_td);
//    try {
//        boost::dijkstra_shortest_paths(g, v1,
//                                       boost::predecessor_map(&predecessors[0])
//                                       .weight_map(boost::get(boost::edge_bundle, g))
//                                       .distance_map(&distances[0])
//                                       .distance_combine(combine_simple(data, g, map_tc, v1, v2, etdebut.date_arrivee))
//                                       .distance_zero(etdebut)
//                                       .distance_inf(etiquette::max())
//                                       .distance_compare(edge_less2())
//                                       .visitor(dijkstra_goal_visitor(v2, data, g, map_tc))
//                                       );

//    } catch(found_goal fg) { v2 = fg.v; }


//    v1 = 5057;
//    v2 = 3421;
//    v1 = trouver_premier_tc(v1, 28800, data, map_td);
//    try {
//        boost::dijkstra_shortest_paths(g, v1,
//                                       boost::predecessor_map(&predecessors[0])
//                                       .weight_map(boost::get(boost::edge_bundle, g))
//                                       .distance_map(&distances[0])
//                                       .distance_combine(combine_simple(data, g, map_tc, v1, v2, etdebut.date_arrivee))
//                                       .distance_zero(etdebut)
//                                       .distance_inf(etiquette::max())
//                                       .distance_compare(edge_less2())
//                                       .visitor(dijkstra_goal_visitor(v2, data, g, map_tc))
//                                       );

//    } catch(found_goal fg) { v2 = fg.v; }


//    v1 = 9931;
//    v2 = 18031;
//    v1 = trouver_premier_tc(v1, 28800, data, map_td);
//    try {
//        boost::dijkstra_shortest_paths(g, v1,
//                                       boost::predecessor_map(&predecessors[0])
//                                       .weight_map(boost::get(boost::edge_bundle, g))
//                                       .distance_map(&distances[0])
//                                       .distance_combine(combine_simple(data, g, map_tc, v1, v2, etdebut.date_arrivee))
//                                       .distance_zero(etdebut)
//                                       .distance_inf(etiquette::max())
//                                       .distance_compare(edge_less2())
//                                       .visitor(dijkstra_goal_visitor(v2, data, g, map_tc))
//                                       );

//    } catch(found_goal fg) { v2 = fg.v; }


//    v1 = 17389;
//    v2 = 18096;
//    v1 = trouver_premier_tc(v1, 28800, data, map_td);
//    try {
//        boost::dijkstra_shortest_paths(g, v1,
//                                       boost::predecessor_map(&predecessors[0])
//                                       .weight_map(boost::get(boost::edge_bundle, g))
//                                       .distance_map(&distances[0])
//                                       .distance_combine(combine_simple(data, g, map_tc, v1, v2, etdebut.date_arrivee))
//                                       .distance_zero(etdebut)
//                                       .distance_inf(etiquette::max())
//                                       .distance_compare(edge_less2())
//                                       .visitor(dijkstra_goal_visitor(v2, data, g, map_tc))
//                                       );

//    } catch(found_goal fg) { v2 = fg.v; }


//    v1 = 17523;
//    v2 = 17574;
//    v1 = trouver_premier_tc(v1, 28800, data, map_td);
//    try {
//        boost::dijkstra_shortest_paths(g, v1,
//                                       boost::predecessor_map(&predecessors[0])
//                                       .weight_map(boost::get(boost::edge_bundle, g))
//                                       .distance_map(&distances[0])
//                                       .distance_combine(combine_simple(data, g, map_tc, v1, v2, etdebut.date_arrivee))
//                                       .distance_zero(etdebut)
//                                       .distance_inf(etiquette::max())
//                                       .distance_compare(edge_less2())
//                                       .visitor(dijkstra_goal_visitor(v2, data, g, map_tc))
//                                       );

//    } catch(found_goal fg) { v2 = fg.v; }


//    v1 = 17627;
//    v2 = 17818;
//    v1 = trouver_premier_tc(v1, 28800, data, map_td);
//    try {
//        boost::dijkstra_shortest_paths(g, v1,
//                                       boost::predecessor_map(&predecessors[0])
//                                       .weight_map(boost::get(boost::edge_bundle, g))
//                                       .distance_map(&distances[0])
//                                       .distance_combine(combine_simple(data, g, map_tc, v1, v2, etdebut.date_arrivee))
//                                       .distance_zero(etdebut)
//                                       .distance_inf(etiquette::max())
//                                       .distance_compare(edge_less2())
//                                       .visitor(dijkstra_goal_visitor(v2, data, g, map_tc))
//                                       );

//    } catch(found_goal fg) { v2 = fg.v; }


//    v1 = 8814;
//    v2 = 14494;
//    v1 = trouver_premier_tc(v1, 28800, data, map_td);
//    try {
//        boost::dijkstra_shortest_paths(g, v1,
//                                       boost::predecessor_map(&predecessors[0])
//                                       .weight_map(boost::get(boost::edge_bundle, g))
//                                       .distance_map(&distances[0])
//                                       .distance_combine(combine_simple(data, g, map_tc, v1, v2, etdebut.date_arrivee))
//                                       .distance_zero(etdebut)
//                                       .distance_inf(etiquette::max())
//                                       .distance_compare(edge_less2())
//                                       .visitor(dijkstra_goal_visitor(v2, data, g, map_tc))
//                                       );

//    } catch(found_goal fg) { v2 = fg.v; }




//        label_name ln(g, data, map_tc);
//        std::ofstream f("test.z");
//        write_graphviz(f, g, ln);



    return 0;
}
