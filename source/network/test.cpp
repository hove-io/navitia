#include "network.h"
#include "boost/property_map/property_map.hpp"
#include <boost/graph/graphviz.hpp>

using namespace network;

struct label_name {
    NW g;
    navitia::type::Data &data;

    label_name(NW &g, navitia::type::Data &data): g(g), data(data){}

    void operator()(std::ostream& oss,vertex_t v) {
            oss << "[label=\"";
            switch(get_n_type(v, data)) {
            case SA : oss << "SA " << get_idx(v, data); break;
            case SP : oss << "SP " << get_idx(v, data); break;
            case RP : oss << "RP " << get_idx(v, data); break;
            case TA : oss << "TA " << get_idx(v, data) << " " << get_time(v, data, g); break;
            case TD : oss << "TD " << get_idx(v, data) << " "<< get_time(v, data, g); break;
            case TC : oss << "TC " << get_idx(v, data) << " "<< get_time(v, data, g); break;
            }
            oss << "\"]";
        }



};






int main(int , char** argv) {
    navitia::type::Data data;
    std::cout << "Debut chargement des données ... " << std::flush;
    data.load_flz("/home/vlara/navitia/jeu/IdF/IdF.nav");
    std::cout << " Fin chargement des données" << std::endl;


    std::cout << "Création et chargement du graph ..." << std::flush;
    NW g(data.pt_data.stop_areas.size() + data.pt_data.stop_points.size() + data.pt_data.route_points.size() + data.pt_data.stop_times.size() * 2);
    charger_graph(data, g);
    std::cout << " Fin de création et chargement du graph" << std::endl;


    std::cout << "Nombre de TC : " << (boost::num_vertices(g) - (data.pt_data.stop_areas.size() + data.pt_data.stop_points.size() + data.pt_data.route_points.size() + data.pt_data.stop_times.size() * 2)) << std::endl;
    vertex_t v1, v2;
    v1 = atoi(argv[1]);
    v2 = atoi(argv[2]);
    std::vector<vertex_t> predecessors(boost::num_vertices(g));
    std::vector<etiquette> distances(boost::num_vertices(g));
    etiquette etdebut;
    etdebut.temps = 0;
    etdebut.date_arrivee = data.pt_data.validity_patterns.at(0).slide(boost::gregorian::from_undelimited_string(argv[4]));
    etdebut.heure_arrivee = atoi(argv[3]);


    std::cout  << "Recherche du chemin entre : " << data.pt_data.stop_areas.at(v1).name << " et " << data.pt_data.stop_areas.at(v2).name << std::endl;

    boost::typed_identity_property_map<edge_t> identity;



    try {
        boost::dijkstra_shortest_paths(g, v1,
                                       boost::predecessor_map(&predecessors[0])
                                       .weight_map(identity)
                                       .distance_map(&distances[0])
                                       .distance_combine(combine(data, g, v1, v2, etdebut.date_arrivee))
                                       .distance_zero(etdebut)
                                       .distance_inf(etiquette::max())
                                       .distance_compare(edge_less2())
                                       .visitor(dijkstra_goal_visitor(v2, data, g))
                                       );
    } catch(found_goal fg) { v2 = fg.v; }


    if(predecessors[v2] == v2) {
        std::cout << "Pas de chemin trouve" << std::endl;
    } else {
        std::cout << "Chemin trouve" << std::endl;
        int i = 0;
        for(vertex_t v = v2; (v!=v1) & (i < 10); v = predecessors[v]) {
            ++i;
            if(get_n_type(v, data) == TA)
                std::cout << data.pt_data.stop_areas.at(data.pt_data.stop_points.at(data.pt_data.route_points.at(data.pt_data.stop_times.at(get_idx(v, data)).route_point_idx).stop_point_idx).stop_area_idx).name << std::endl;
        }
    }


//    label_name ln(g, data);
//    std::ofstream f("test.z");
//    write_graphviz(f, g, ln);



    return 0;

}
