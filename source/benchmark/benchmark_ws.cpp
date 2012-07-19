#include "type/type.h"
#include "type/type.pb.h"
#include "baseworker.h"
#include "network/network.h"
#include "boost/graph/dijkstra_shortest_paths.hpp"
#include "gateway_mod/interface.h"
using namespace webservice;
using namespace network;
using namespace boost;

struct Data2{
    navitia::type::Data data;
    network::NW g;
    map_tc_t map_tc, map_td;
    int nb_threads;

    Data2() : nb_threads(1) {
        Configuration * conf = Configuration::get();
        std::cout << "Je suis l'executable " << conf->get_string("application") <<std::endl;
        std::cout << "Je réside dans le path " << conf->get_string("path") <<std::endl;
        data.load_lz4("/home/vlara/navitia/jeu/IdF/IdF.nav");
        std::cout << "Chargement des données effectué" << std::endl;
        std::cout << "Nb route points : " << data.pt_data.route_points.size() << std::endl;
        std::cout << "Num stop areas : " << data.pt_data.stop_areas.size() << std::endl;
        std::cout << "Num stop times : " << data.pt_data.stop_times.size() << std::endl;
        g = NW(data.pt_data.stop_areas.size() + data.pt_data.stop_points.size() + data.pt_data.route_points.size() + (data.pt_data.stop_times.size()*2));
        charger_graph(data, g, map_tc, map_td);
        std::cout << "num edges : " << num_edges(g) << std::endl;
        std::cout << "num vertices : " << num_vertices(g) << std::endl;

    }
};


class Worker : public BaseWorker<Data2> {
private:
    int i;


    std::vector<vertex_t> predecessors;
    std::vector<etiquette> distances;


    ResponseData test1(RequestData& request, Data2 &d) {
        ResponseData rd;


        vertex_t v1 = atoi(get<std::string>(request.parsed_params["depart"].value).c_str()),
                 v2 = atoi(get<std::string>(request.parsed_params["arrivee"].value).c_str()),
                 fin_parcours = num_vertices(d.g) + 1;

        posix_time::ptime start, end;

        etiquette etdebut;
        etdebut.temps = 0;
        etdebut.date_arrivee = d.data.pt_data.validity_patterns.at(0).slide(gregorian::from_undelimited_string(get<std::string>(request.parsed_params["date"].value).c_str()));
        etdebut.heure_arrivee = atoi(get<std::string>(request.parsed_params["heure"].value).c_str());



        std::cout << "Debut "<< v1 << " " << v2 << " " << trouver_premier_tc(v1, etdebut.heure_arrivee, d.data, d.map_td) <<  " ... " << std::flush;
        start = posix_time::microsec_clock::local_time();
        v1 = trouver_premier_tc(v1, etdebut.heure_arrivee, d.data, d.map_td);
        try {
            boost::dijkstra_shortest_paths(d.g, v1,
                                           boost::predecessor_map(&predecessors[0])
                                           .weight_map(boost::get(boost::edge_bundle, d.g))
                                           .distance_map(&distances[0])
                                           .distance_combine(combine_simple(d.data, d.g, d.map_tc, v1, v2, etdebut.date_arrivee))
                                           .distance_zero(etdebut)
                                           .distance_inf(etiquette::max())
                                           .distance_compare(edge_less2())
                                           .visitor(dijkstra_goal_visitor(v2, d.data, d.g, d.map_tc))
                                           );
        } catch(found_goal fg) {fin_parcours= fg.v; }
        end = posix_time::microsec_clock::local_time();

        std::cout << "fin 1 " << std::flush;
        int num_correspondances = 0;

        if(fin_parcours != num_vertices(d.g) + 1) {
            vertex_t pred = fin_parcours;
            for(vertex_t v = predecessors[fin_parcours]; v != v1; v = predecessors[v]) {
                if(get_n_type(v, d.data) == TC && get_n_type(pred, d.data) == TD)
                    ++num_correspondances;
            }
        }
        v1 = atoi(get<std::string>(request.parsed_params["depart"].value).c_str());
        rd.response << v1 << "," << d.data.pt_data.stop_areas.at(atoi(get<std::string>(request.parsed_params["depart"].value).c_str())).external_code << "," << d.data.pt_data.stop_areas.at(v1).coord.x << "," << d.data.pt_data.stop_areas.at(v1).coord.y << ","
                    << v2  << "," << d.data.pt_data.stop_areas.at(v2).external_code << ","<< d.data.pt_data.stop_areas.at(v2).coord.x << "," << d.data.pt_data.stop_areas.at(v2).coord.y << ","
                    << get<std::string>(request.parsed_params["date"].value).c_str() << "," << get<std::string>(request.parsed_params["heure"].value).c_str() << ","
                    << num_correspondances << "," << distances[fin_parcours].temps << "," << (end-start).total_milliseconds() << std::endl;
        rd.content_type = "text/csv";
        rd.status_code = 200;
        std::cout << "fin" << v1 << " " << v2 << std::endl;
        return rd;
    }

public:
    Worker(Data2 &d) : i(0),predecessors(num_vertices(d.g)), distances(num_vertices(d.g)) {
        register_api("test1",bind(&Worker::test1, this, _1, _2), "Api qui tous les itineraires d'un stop area");
        add_default_api();

    }
};

/// Macro qui va construire soit un exectuable FastCGI, soit une DLL ISAPI
MAKE_WEBSERVICE(Data2, Worker)
