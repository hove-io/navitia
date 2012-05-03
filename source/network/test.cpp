#include "network.h"

using namespace network;

int main(int , char** argv) {
    navitia::type::Data data;
    std::cout << "Debut chargement des données ... " << std::flush;
    data.load_flz("/home/vlara/navitia/jeu/IdF/IdF.nav");

    std::cout << " Fin chargement des données" << std::endl;


    std::cout << "Création et chargement du graph ..." << std::flush;
    NW g(data.pt_data.stop_areas.size() + data.pt_data.stop_points.size() + data.pt_data.route_points.size() + data.pt_data.stop_times.size() * 2);
    charger_graph(data, g);
    std::cout << " Fin de création et chargement du graph" << std::endl;

    vertex_t v1, v2;
    v1 = atoi(argv[1]);
    v2 = atoi(argv[2]);
    std::vector<vertex_t> predecessors(boost::num_vertices(g));
    std::vector<etiquette> distances(boost::num_vertices(g));
    etiquette etdebut;
    etdebut.temps = 0;
    etdebut.date_arrivee = data.pt_data.validity_patterns.at(0).slide(boost::gregorian::from_undelimited_string(argv[4]));
    etdebut.heure_arrivee = atoi(argv[3]);




    try {
        boost::dijkstra_shortest_paths(g, v1,
                                       boost::predecessor_map(&predecessors[0])
                                       .weight_map(boost::get(boost::edge_bundle, g))
                                       .distance_map(&distances[0])
                                       .distance_combine(combine2(data, g, v1, v2, etdebut.date_arrivee))
                                       .distance_zero(etdebut)
                                       .distance_inf(etiquette::max())
                                       .distance_compare(edge_less())
                                       .visitor(dijkstra_goal_visitor(v2, data))
                                       );
    } catch(found_goal fg) { v2 = fg.v; std::cout << "FOUND " << fg.v <<std::endl;}
    return 0;
}
