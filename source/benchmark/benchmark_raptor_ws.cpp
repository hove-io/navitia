#include "type/type.h"
#include "type/type.pb.h"
#include "baseworker.h"
#include "RAPTOR/raptor.h"
#include "boost/graph/dijkstra_shortest_paths.hpp"
#include "gateway_mod/interface.h"
using namespace webservice;
using namespace boost;

struct Data2{
    navitia::type::Data data;
    int nb_threads;

    Data2() :nb_threads(1) {
        Configuration * conf = Configuration::get();
        std::cout << "Je suis l'executable " << conf->get_string("application") <<std::endl;
        std::cout << "Je réside dans le path " << conf->get_string("path") <<std::endl;

        data.load_lz4("/home/vlara/navitia/jeu/IdF/IdF.nav");
        std::cout << "Chargement des données effectué" << std::endl;
        std::cout << "Num stop areas : " << data.pt_data.stop_areas.size() << std::endl;
    }



};
class Worker : public BaseWorker<Data2> {
private:
    int i;

    ResponseData test1(RequestData& request, Data2 &d) {
        ResponseData rd;

        boost::posix_time::ptime start, end;
        start = boost::posix_time::microsec_clock::local_time();
        raptor::pair_retour_t pair_retour = raptor::RAPTOR(atoi(get<std::string>(request.parsed_params["depart"].value).c_str()),
                                                           atoi(get<std::string>(request.parsed_params["arrivee"].value).c_str()),
                                                           atoi(get<std::string>(request.parsed_params["heure"].value).c_str()),
                                                           d.data.pt_data.validity_patterns.at(0).slide(boost::gregorian::from_undelimited_string(get<std::string>(request.parsed_params["date"].value).c_str())),
                                                           d.data);
        end = boost::posix_time::microsec_clock::local_time();

        raptor::map_retour_t retour = pair_retour.first;
        int num_correspondances = 0;
        BOOST_FOREACH(raptor::map_retour_t::value_type r1, retour) {
            if(r1.first > 0) {
                if(r1.second.count(atoi(get<std::string>(request.parsed_params["arrivee"].value).c_str())) != 0) {
                    num_correspondances = r1.first;
                }
            }
        }

        int temps = 0;
        if(retour[0][atoi(get<std::string>(request.parsed_params["depart"].value).c_str())].jour < retour[num_correspondances][atoi(get<std::string>(request.parsed_params["arrivee"].value).c_str())].jour) {
            temps = (retour[num_correspondances][atoi(get<std::string>(request.parsed_params["arrivee"].value).c_str())].jour - retour[0][atoi(get<std::string>(request.parsed_params["depart"].value).c_str())].jour) * 86400;
            temps += 86400 - atoi(get<std::string>(request.parsed_params["heure"].value).c_str());
            temps += retour[num_correspondances][atoi(get<std::string>(request.parsed_params["arrivee"].value).c_str())].temps;
        } else {
            temps = retour[num_correspondances][atoi(get<std::string>(request.parsed_params["arrivee"].value).c_str())].temps - atoi(get<std::string>(request.parsed_params["heure"].value).c_str());
        }

        rd.response << get<std::string>(request.parsed_params["depart"].value).c_str() << ","
                    << d.data.pt_data.stop_areas.at(atoi(get<std::string>(request.parsed_params["depart"].value).c_str())).external_code << ","
                    << d.data.pt_data.stop_areas.at(atoi(get<std::string>(request.parsed_params["depart"].value).c_str())).coord.x << ","
                    << d.data.pt_data.stop_areas.at(atoi(get<std::string>(request.parsed_params["depart"].value).c_str())).coord.y << ","
                    << atoi(get<std::string>(request.parsed_params["arrivee"].value).c_str()) << ","
                    << d.data.pt_data.stop_areas.at(atoi(get<std::string>(request.parsed_params["arrivee"].value).c_str())).external_code << ","
                    << d.data.pt_data.stop_areas.at(atoi(get<std::string>(request.parsed_params["arrivee"].value).c_str())).coord.x << ","
                    << d.data.pt_data.stop_areas.at(atoi(get<std::string>(request.parsed_params["arrivee"].value).c_str())).coord.y << ","
                    << retour[0][atoi(get<std::string>(request.parsed_params["depart"].value).c_str())].jour << ","
                    << retour[num_correspondances][atoi(get<std::string>(request.parsed_params["arrivee"].value).c_str())].jour << ","
                    << get<std::string>(request.parsed_params["heure"].value).c_str() << ","
                    << retour[num_correspondances][atoi(get<std::string>(request.parsed_params["arrivee"].value).c_str())].temps << ","
                    << num_correspondances << ","
                    << temps
                    << "," << (end-start).total_milliseconds() << std::endl;
        rd.content_type = "text/csv";
        rd.status_code = 200;
        std::cout << "fin" << get<std::string>(request.parsed_params["depart"].value).c_str() << " " << get<std::string>(request.parsed_params["arrivee"].value).c_str() << std::endl;
        return rd;
    }

public:
    Worker(Data2 &) : i(0) {
        register_api("test1",bind(&Worker::test1, this, _1, _2), "Api qui tous les itineraires d'un stop area");
        add_default_api();
    }
};

/// Macro qui va construire soit un exectuable FastCGI, soit une DLL ISAPI
MAKE_WEBSERVICE(Data2, Worker)
