#include "raptor_api.h"
#include "routing.h"

namespace navitia { namespace routing { namespace raptor {


/**
 *  se charge de remplir l'objet protocolbuffer solution passé en paramètre
*/
void create_pb_froute(navitia::routing::Path & path, const nt::Data & data, pbnavitia::FeuilleRoute &solution) {
    solution.set_nbchanges(path.nb_changes);
    solution.set_duree(path.duration);
    int i = 0;
    pbnavitia::Etape * etape;


    BOOST_FOREACH(navitia::routing::PathItem item, path.items) {
        std::cout << "item : " << item.day << " " << item.time << " " << item.said << " " << item.line_idx << std::endl;
    }
    BOOST_FOREACH(navitia::routing::PathItem item, path.items) {
        if(i%2 == 0) {
            etape = solution.add_etapes();
            if(item.line_idx  < data.pt_data.lines.size())
                etape->mutable_mode()->set_ligne(data.pt_data.lines.at(item.line_idx).name);
            else
                etape->mutable_mode()->set_ligne("marche a pied");
            etape->mutable_depart()->mutable_lieu()->mutable_geo()->set_x(data.pt_data.stop_areas.at(item.said).coord.x);
            etape->mutable_depart()->mutable_lieu()->mutable_geo()->set_y(data.pt_data.stop_areas.at(item.said).coord.y);
            etape->mutable_depart()->mutable_lieu()->set_nom(data.pt_data.stop_areas.at(item.said).name);
            etape->mutable_depart()->mutable_date()->set_date(item.day);
            etape->mutable_depart()->mutable_date()->set_heure(item.time);
        } else {
            etape->mutable_arrivee()->mutable_lieu()->mutable_geo()->set_x(data.pt_data.stop_areas.at(item.said).coord.x);
            etape->mutable_arrivee()->mutable_lieu()->mutable_geo()->set_y(data.pt_data.stop_areas.at(item.said).coord.y);
            etape->mutable_arrivee()->mutable_lieu()->set_nom(data.pt_data.stop_areas.at(item.said).name);
            etape->mutable_arrivee()->mutable_date()->set_date(item.day);
            etape->mutable_arrivee()->mutable_date()->set_heure(item.time);
        }

        ++i;
    }

}

void create_pb_itineraire(navitia::routing::Path &path, const nt::Data & data, pbnavitia::ItineraireDetail &itineraire) {
    if(path.items.size() > 0) {
        pbnavitia::Trajet * trajet = itineraire.add_trajets();
        trajet->set_mode(0);
        trajet->set_ligne("");

        unsigned int precsaid = std::numeric_limits<unsigned int>::max();
        BOOST_FOREACH(navitia::routing::PathItem item, path.items) {
            if(item.said == precsaid) {
                trajet = itineraire.add_trajets();
                trajet->set_mode(0);
                trajet->set_ligne("");
            }
            pbnavitia::Geocode * geo = trajet->add_pas();
            geo->set_x(data.pt_data.stop_areas.at(item.said).coord.x);
            geo->set_y(data.pt_data.stop_areas.at(item.said).coord.y);
            precsaid = item.said;

        }
    }
}

pbnavitia::Response make_response(std::vector<navitia::routing::Path> pathes, const nt::Data & d) {
    pbnavitia::Response pb_response;
    pb_response.set_requested_api(pbnavitia::PLANNER);

    BOOST_FOREACH(auto path, pathes) {
        navitia::routing::Path itineraire = navitia::routing::makeItineraire(path);
        pbnavitia::Planning * planning = pb_response.mutable_planner()->add_planning();
        create_pb_froute(itineraire, d, *planning->mutable_feuilleroute());
        create_pb_itineraire(path, d, *planning->mutable_itineraire());
    }

    return pb_response;
}

}}}
