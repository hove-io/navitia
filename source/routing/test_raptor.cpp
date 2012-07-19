#include "raptor.h"
#include "type/data.h"
#include "utils/timer.h"
#include "boost/date_time.hpp"
#include "naviMake/build_helper.h"
using namespace navitia;

int main(int, char **) {
//    type::Data data;
//    {
//        Timer t("Chargement des données");
//        data.load_lz4("/home/vlara/navitia/jeu/IdF/IdF.nav");

//        data.build_proximity_list();
//    }

//    int depart = 13587;
//    int arrivee = 2460;
//    std::cout << "Recherche de chemin entre " << data.pt_data.stop_areas.at(depart).name << " et " << data.pt_data.stop_areas.at(arrivee).name << std::endl;



////    BOOST_FOREACH(navitia::type::VehicleJourney vj, data.pt_data.vehicle_journeys) {
////        unsigned int precstid = data.pt_data.stop_times.size() + 1;
////        BOOST_FOREACH(unsigned int stid, vj.stop_time_list){
////            if(precstid != (data.pt_data.stop_times.size() + 1)) {
////                if(data.pt_data.stop_times.at(precstid).arrival_time > data.pt_data.stop_times.at(stid).arrival_time ||
////                   data.pt_data.stop_times.at(precstid).departure_time > data.pt_data.stop_times.at(stid).departure_time )
////                    std::cout <<"Bug de données" << std::endl;
////            }
////            precstid =  stid;
////        }
////    }

//    routing::raptor::RAPTOR raptor(data);
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
//    }/planner?departure_lat=48.867483087514856&departure_lon=2.3305474316803103&destination_lat=48.84850904718449&destination_lon=2.349430179055217&time=28800&date=20120211

//    {
//        Timer t("RAPTOR");
//        std::cout << data.pt_data.stop_areas.at(13587).coord << data.pt_data.stop_areas.at(2460).coord << std::endl;

//        std::cout << raptor.compute(navitia::type::GeographicalCoord(2.3305474316803103, 48.867483087514856), 500, navitia::type::GeographicalCoord(2.349430179055217, 48.84850904718449), 500, 28800, 7);
//    }

    navimake::builder b("20120614");
    b.vj("A")("stop1", 8000)("stop2", 8200);
    b.vj("B")("stop3", 9000)("stop4", 9200);
    b.connection("stop2", "stop3", 10*60);
    type::Data data;
    data.pt_data =  b.build();
    routing::raptor::RAPTOR raptor(data);

    type::PT_Data d = data.pt_data;

    auto res = raptor.compute(d.stop_areas[0].idx, d.stop_areas[3].idx, 7500, 0);
    std::cout << res;
}
