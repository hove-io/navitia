#include "network.h"

using namespace network;

int main(int , char**) {
    navitia::type::Data data;
    std::cout << "Debut chargement des données ... " << std::flush;
    data.load_flz("/home/vlara/navitia/jeu/IdF/IdF.nav");

    std::cout << " Fin chargement des données" << std::endl;


    std::cout << "Création et chargement du graph ..." << std::flush;
    NW g(data.pt_data.stop_areas.size() + data.pt_data.stop_points.size() + data.pt_data.route_points.size() + data.pt_data.stop_times.size() * 2);
    charger_graph(data, g);
    std::cout << " Fin de création et chargement du graph" << std::endl;



    return 0;
}
