#include "raptor.h"
#include "type/data.h"
#include "utils/timer.h"
#include "boost/date_time.hpp"
using namespace navitia;

int main(int, char **) {
    type::Data data;
    {
        Timer t("Chargement des données");
        data.load_lz4("/home/vlara/navitia/jeu/IdF/IdF.nav");
    }

    int depart = 13587;
    int arrivee = 973;
    std::cout << "Recherche de chemin entre " << data.pt_data.stop_areas.at(depart).name << " et " << data.pt_data.stop_areas.at(arrivee).name << std::endl;

    routing::raptor::RAPTOR raptor(data);
    {
        Timer t("Calcul raptor");
        routing::Path result = raptor.compute(depart, arrivee, 28800, data.pt_data.validity_patterns.front().slide(boost::gregorian::date(2012,02,16)));
        std::cout << result;
    }


}
