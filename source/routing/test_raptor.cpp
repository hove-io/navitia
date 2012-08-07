#include "raptor.h"
#include "type/data.h"
#include "utils/timer.h"
#include "boost/date_time.hpp"
#include "naviMake/build_helper.h"

#include <valgrind/callgrind.h>

using namespace navitia;

int main(int, char **) {
//    type::Data data;
//    {
//        Timer t("Chargement des données");
//        data.load_lz4("/home/vlara/navitia/jeu/IdF/IdF.nav");

//        data.build_proximity_list();
//    }

//    int depart = 0;
//    int arrivee = 2;
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
////    }planner?format=json&departure_lat=48.83947126194525&departure_lon=2.3949204340949115&destination_lat=48.8757255182949&destination_lon=2.3257409141666066&time=0800&date=20120511
//    routing::raptor::RAPTOR raptor(data);
//    {
//        Timer t("Calcul raptor");
//        CALLGRIND_START_INSTRUMENTATION;
//        std::vector<routing::Path> result = raptor.compute_all(type::GeographicalCoord(2.3949204340949115, 48.83947126194525), 300, type::GeographicalCoord(2.325740914166606, 48.8757255182949), 300, 8*3600, 7);
//        CALLGRIND_STOP_INSTRUMENTATION;
//        CALLGRIND_DUMP_STATS;

//        BOOST_FOREACH(auto pouet, result) {
//        std::cout << pouet << std::endl << std::endl;

//        std::cout << makeItineraire(pouet);
//        }
//        //        routing::Path result = raptor.compute(11484, 5596, 28800, 7);
//    }

//    std::cout << "Je suis ici ! " << std::endl;

//    {
//        Timer t("Calcul raptor");/*
//        routing::Path result = raptor.makeItineraire(raptor.compute(depart, arrivee, 72000, 7));
//        std::cout << result;*/

//        routing::raptor::map_int_pint_t bests;
//        bests[depart] = navitia::routing::raptor::type_retour(-1, navitia::routing::DateTime(7, 72000));
//        std::cout << raptor.compute(bests, arrivee);
//    }

//    {
//        Timer t("RAPTOR");
//        std::cout << data.pt_data.stop_areas.at(13587).coord << data.pt_data.stop_areas.at(2460).coord << std::endl;

//        std::cout << raptor.compute(navitia::type::GeographicalCoord(2.3305474316803103, 48.867483087514856), 500, navitia::type::GeographicalCoord(2.349430179055217, 48.84850904718449), 500, 28800, 7);
//    }


    navimake::builder b("20120614");
    b.vj("A", "0")("stop1", 8000)("stop2", 8200);
    b.vj("B", "1001")("stop1", 9000)("stop2", 9200);
    type::Data data;
    data.pt_data =  b.build();
    routing::raptor::reverseRAPTOR raptor(data);

    type::PT_Data d = data.pt_data;

    std::cout << "Test : " << data.pt_data.validity_patterns.at(0).check(0) << std::endl;
    std::cout << "Test : " << data.pt_data.validity_patterns.at(0).check(1) << std::endl;
    std::cout << "Test : " << data.pt_data.validity_patterns.at(1).check(0) << std::endl;
    std::cout << "Test : " << data.pt_data.validity_patterns.at(1).check(1) << std::endl;
    std::cout << "Test : " << data.pt_data.validity_patterns.at(1).check(2) << std::endl;
    std::cout << "Test : " << data.pt_data.validity_patterns.at(1).check(3) << std::endl;

    auto res = raptor.compute(d.stop_areas[1].idx, d.stop_areas[0].idx, 9300, 0);
    std::cout << res << std::endl;
    res = raptor.compute(d.stop_areas[1].idx, d.stop_areas[0].idx, 9300, 1);
    std::cout << res << std::endl;
    res = raptor.compute(d.stop_areas[1].idx, d.stop_areas[0].idx, 9300, 2);
    std::cout << res << std::endl;
    res = raptor.compute(d.stop_areas[1].idx, d.stop_areas[0].idx, 9300, 3);
    std::cout << res << std::endl;


}
