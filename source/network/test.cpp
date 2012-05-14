#include "network.h"

using namespace network;

int main(int , char** argv) {
    navitia::type::Data data;
    std::cout << "Debut chargement des données ... " << std::flush;
    data.load_flz("/home/vlara/navitia/jeu/IdF/IdF.nav");

    std::cout << " Fin chargement des données" << std::endl;



//    navitia::type::Route r1, r2;
//    navitia::type::RoutePoint rp1, rp2;



//    rp1.idx = 0;
//    rp2.idx = 1;
//    rp1.stop_point_idx = 1;
//    rp2.stop_point_idx = 2;

//    r1.route_point_list.push_back(rp1.idx);
//    r1.route_point_list.push_back(rp2.idx);


//    r2.route_point_list.push_back(rp2.idx);
//    r2.route_point_list.push_back(rp1.idx);

//    data.pt_data.routes.push_back(r1);
//    data.pt_data.routes.push_back(r2);

//    data.pt_data.route_points.push_back(rp1);
//    data.pt_data.route_points.push_back(rp2);

//    map_ar_t map_ar;
//    calculer_ar(data, map_ar);

//    std::cout << "Nombre d'aller retour trouvés : " << map_ar.size() << std::endl;

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
    } catch(found_goal fg) { v2 = fg.v;}
    if(predecessors[v2] == v2) {
        std::cout << " Fail" << std::endl;
    } else {
        for(vertex_t v = v2; v != v1; v = predecessors[v]) {
            std::cout << data.pt_data.stop_areas.at(get_saidx(v, data)).name << " : " << data.pt_data.stop_times.at(get_idx(v, data)).departure_time;
            std::cout << "Ligne : "  << data.pt_data.lines.at(data.pt_data.routes.at(data.pt_data.route_points.at(data.pt_data.stop_times.at(get_idx(v,data)).route_point_idx).route_idx).line_idx).name<< "Route : " << data.pt_data.stop_times.at(get_idx(v,data)).vehicle_journey_idx<<std::endl;
        }
    }

    BOOST_FOREACH(idx_t idx, data.pt_data.routes.at(334317).route_point_list) {
        std::cout << data.pt_data.stop_areas.at(data.pt_data.stop_points.at(data.pt_data.route_points.at(idx).stop_point_idx).stop_area_idx).name << std::endl;
    }

    return 0;
}
