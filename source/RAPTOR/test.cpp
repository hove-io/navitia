#include <valgrind/callgrind.h>
#include "raptor.h"
#include "type/type.h"

int main(int , char** /*argv*/) {
    navitia::type::Data data;
    std::cout << "Chargemement des données ... " << std::flush;

    data.load_lz4("/home/vlara/navitia/jeu/IdF/IdF.nav");

    std::cout << "Fin chargement" << std::endl;
    std::cout << "Nb stop areas : " << data.pt_data.stop_areas.size() << std::endl
              << "Nb stop points : " << data.pt_data.stop_points.size() << std::endl;

    unsigned int depart = 13587, arrivee = 16331;

    BOOST_FOREACH(navitia::type::Route route, data.pt_data.routes) {


        BOOST_FOREACH(unsigned int vj, route.vehicle_journey_list) {

            for(unsigned int i=0; i < route.route_point_list.size(); ++i) {

                if(route.route_point_list.at(i) != data.pt_data.stop_times.at(data.pt_data.vehicle_journeys.at(vj).stop_time_list.at(i)).route_point_idx) {
                    std::cout << " non ";

                }

            }

        }
    }



        boost::posix_time::ptime start, end;


        CALLGRIND_START_INSTRUMENTATION;
        start = boost::posix_time::microsec_clock::local_time();
        raptor::pair_retour_t pair_retour = raptor::RAPTOR(depart, arrivee, 28800, data.pt_data.validity_patterns.at(0).slide(boost::gregorian::from_undelimited_string("20120219")), data);
        end = boost::posix_time::microsec_clock::local_time();
        CALLGRIND_STOP_INSTRUMENTATION;
        CALLGRIND_DUMP_STATS;


        std::cout << "temps : "  << (end - start).total_milliseconds() << std::endl;


        std::cout << "Depart : " << data.pt_data.stop_areas.at(depart).name << " Arrivée : " << data.pt_data.stop_areas.at(arrivee).name << std::endl << std::endl;
        raptor::map_retour_t retour = pair_retour.first;

        BOOST_FOREACH(raptor::map_retour_t::value_type r1, retour) {
            if(r1.first > 0) {
                if(r1.second.count(arrivee) == 0)
                    std::cout << "Aucun trajet avec " << (r1.first - 1)<< " correspondances " << std::endl;
                else {
                    std::cout << "Un trajet avec " << (r1.first - 1) << "correspondances existe, il arrive à : " <<r1.second[arrivee].temps;
                    std::cout << " route : " << data.pt_data.route_points.at(data.pt_data.stop_times.at(r1.second[arrivee].stid).route_point_idx).route_idx<< std::endl;
                    raptor::afficher_itineraire(depart, arrivee, pair_retour, r1.first, data);
                    std::cout << std::endl << std::endl;
                }
            }

        }
        raptor::McRAPTOR(depart, arrivee, 28800, data.pt_data.validity_patterns.at(0).slide(boost::gregorian::from_undelimited_string("20120219")), data, 973);



    return 0;
}
