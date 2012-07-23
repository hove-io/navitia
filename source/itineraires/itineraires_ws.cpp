#include "type/type.h"
#include "type/type.pb.h"
#include "baseworker.h"
#include "network/network.h"
#include "boost/graph/dijkstra_shortest_paths.hpp"
#include "itineraires.h"
#include "to_proto.h"
#include "gateway_mod/interface.h"
using namespace webservice;
using namespace network;
using namespace itineraires;
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
        data.load_lz4("/home/vlara/navitia/jeu/poitiers/poitiers.nav");
        std::cout << "Chargement des données effectué" << std::endl;
        std::cout << "Nb route points : " << data.pt_data.route_points.size() << std::endl;
        g = NW(data.pt_data.stop_areas.size() + data.pt_data.stop_points.size() + data.pt_data.route_points.size() + (data.pt_data.stop_times.size()*2));
        charger_graph(data, g, map_tc, map_td);

        std::cout << "Num stop areas : " << data.pt_data.stop_areas.size() << std::endl;
    }
};





class Worker : public BaseWorker<Data2> {
private:
    int i;
    ResponseData getitineraires(RequestData& request, Data2 &d) {
        ResponseData rd;

        std::vector<vertex_t> predecessors(num_vertices(d.g));
        std::vector<etiquette> distances(num_vertices(d.g));

        vertex_t v1 = atoi(get<std::string>(request.parsed_params["depart"].value).c_str());
        vertex_t v2 = atoi(get<std::string>(request.parsed_params["arrivee"].value).c_str());

        etiquette etdebut;
        etdebut.temps = 0;
        etdebut.date_arrivee = d.data.pt_data.validity_patterns.at(0).slide(gregorian::from_undelimited_string(get<std::string>(request.parsed_params["date"].value).c_str()));
        etdebut.heure_arrivee = 0;

        std::cout << "Debut du calcul d'itineraire ... " << std::flush;

        boost::typed_identity_property_map<edge_t> identity;

        dijkstra_shortest_paths(d.g, v1,
                                predecessor_map(&predecessors[0])
                                .weight_map(identity)
                                .distance_map(&distances[0])
                                .distance_combine(combine(d.data, d.g, d.map_tc, v1, v2, etdebut.date_arrivee))
                                .distance_zero(etdebut)
                                .distance_inf(etiquette::max())
                                .distance_compare(edge_less2())
                                );
        std::cout << " Fin " << std::flush;

        std::vector<itineraire> itineraires;

        map_parcours parcours_list = map_parcours();
        std::cout << " Debut parse itineraires ..." << std::flush;
        make_itineraires(v1, v2, itineraires, parcours_list, d.g, d.data, predecessors, d.map_tc);
        std::cout << "Fin parse "  <<  std::flush;

        std::cout << " Debut encodage ... " << std::flush;
        pbnavitia::ReponseDemonstrateur reponse_demonstrateur;
        to_proto(parcours_list, &reponse_demonstrateur, d.data);

        BOOST_FOREACH(itineraires::itineraire it, itineraires) {
            pbnavitia::Itineraire *i_proto = reponse_demonstrateur.add_itineraires_liste();
            to_proto(it, d.data, i_proto);
        }
        std::cout << " Fin encodage " << std::endl;


        rd.response << get<std::string>(request.parsed_params["jsoncallback"].value).c_str() << "(" << pb2json(&reponse_demonstrateur) <<  ")";
        rd.content_type = "application/json";
        rd.status_code = 200;
        return rd;
    }

    ResponseData testons(RequestData& /*request*/, Data2 &/*d*/) {
        ResponseData rd;
        rd.charset = "utf-8";
        rd.response << "blabla" << std::flush;
        rd.content_type = "text/plain";
        rd.status_code = 200;

        return rd;
    }



public:
    Worker(Data2 &) : i(0) {
        register_api("getitineraires",bind(&Worker::getitineraires, this, _1, _2), "Api qui tous les itineraires d'un stop area");
        register_api("testons",bind(&Worker::testons, this, _1, _2), "Api qui tous les itineraires d'un stop area");
        add_default_api();
    }
};

/// Macro qui va construire soit un exectuable FastCGI, soit une DLL ISAPI
MAKE_WEBSERVICE(Data2, Worker)
