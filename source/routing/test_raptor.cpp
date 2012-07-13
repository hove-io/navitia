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

        data.build_proximity_list();
    }

    int depart = 147;
    int arrivee = 246;
    std::cout << "Recherche de chemin entre " << data.pt_data.stop_areas.at(depart).name << " et " << data.pt_data.stop_areas.at(arrivee).name << std::endl;



//    BOOST_FOREACH(navitia::type::VehicleJourney vj, data.pt_data.vehicle_journeys) {
//        unsigned int precstid = data.pt_data.stop_times.size() + 1;
//        BOOST_FOREACH(unsigned int stid, vj.stop_time_list){
//            if(precstid != (data.pt_data.stop_times.size() + 1)) {
//                if(data.pt_data.stop_times.at(precstid).arrival_time > data.pt_data.stop_times.at(stid).arrival_time ||
//                   data.pt_data.stop_times.at(precstid).departure_time > data.pt_data.stop_times.at(stid).departure_time )
//                    std::cout <<"Bug de données" << std::endl;
//            }
//            precstid =  stid;
//        }
//    }

    routing::raptor::RAPTOR raptor(data);
//    {
//        Timer t("Calcul raptor");
//        routing::Path result = raptor.makeItineraire(raptor.compute(depart, arrivee, 72000, 7));
//        std::cout << result;
//    }


//    {
//        Timer t("Calcul raptor");/*
//        routing::Path result = raptor.makeItineraire(raptor.compute(depart, arrivee, 72000, 7));
//        std::cout << result;*/

//        routing::raptor::map_int_pint_t bests;
//        bests[depart] = navitia::routing::raptor::type_retour(-1, navitia::routing::DateTime(7, 72000));
//        std::cout << raptor.compute(bests, arrivee);
//    }

    {
        Timer t("RAPTOR");
        std::cout << raptor.compute(data.pt_data.stop_areas.at(13587).coord, 500, data.pt_data.stop_areas.at(2460).coord, 500, 28800, 7);
    }
}
