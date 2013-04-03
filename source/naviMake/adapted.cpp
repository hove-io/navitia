#include "adapted.h"
#include <boost/date_time/posix_time/posix_time.hpp>

namespace nt = navitia::type;
namespace bg = boost::gregorian;
namespace pt = boost::posix_time;

namespace navimake{


void delete_vj(types::VehicleJourney* vehicle_journey, const nt::Message& message, Data& data){
    //TODO on duplique les validitypattern à tort et à travers la, va falloir les mutualiser
    types::ValidityPattern* vp = NULL;
    vp = new types::ValidityPattern(*vehicle_journey->adapted_validity_pattern);
    data.validity_patterns.push_back(vp);
    vehicle_journey->adapted_validity_pattern = vp;

    for(size_t i=0; i < vp->days.size(); ++i){
        bg::date current_date = vp->beginning_date + bg::days(i);
        if(!vp->check(i) || current_date < message.application_period.begin().date()
                || current_date > message.application_period.end().date()){
            continue;
        }
        pt::ptime begin(current_date, pt::seconds(vehicle_journey->stop_time_list.front()->departure_time));
        //si le départ du vj est dans la plage d'appliation du message, on le supprime
        if(message.is_applicable(pt::time_period(begin, pt::seconds(1)))){
            vp->remove(current_date);
        }

    }


}

std::vector<types::StopTime*> get_stop_from_impact(const navitia::type::Message& message, bg::date current_date, std::vector<types::StopTime*> stoplist){
    std::vector<types::StopTime*> result;
    pt::ptime currenttime;
    if(message.object_type == navitia::type::Type_e::StopPoint){
        for(auto stop : stoplist){
            currenttime = pt::ptime(current_date, pt::seconds(stop->departure_time));
            if((stop->tmp_stop_point->uri == message.object_uri) && (message.is_applicable(pt::time_period(currenttime, pt::seconds(1))))){
                result.push_back(stop);
            }
        }
    }
    if(message.object_type == navitia::type::Type_e::StopArea){
        for(auto stop : stoplist){
            currenttime = pt::ptime(current_date, pt::seconds(stop->departure_time));
            if((stop->tmp_stop_point->stop_area->uri == message.object_uri)&& (message.is_applicable(pt::time_period(currenttime, pt::seconds(1))))){
                result.push_back(stop);
            }
        }
    }
    return result;
}

std::string make_adapted_uri(const types::VehicleJourney* vj, const Data& data){
    //@TODO: impleménter une régle intélligente
    return vj->uri + ":adapted";
}

void duplicate_vj(types::VehicleJourney* vehicle_journey, const nt::Message& message, Data& data){
    types::VehicleJourney* vj_adapted = NULL;
    //on teste le validitypattern adapté car si le VJ est déjà supprimé le traitement n'est pas nécessaire
    //@TODO: faux car on ne pas appilquer 2 messages différent sur un même jour, mais comment détecter le vj supprimé
    for(size_t i=0; i < vehicle_journey->validity_pattern->days.size(); ++i){
        pt::time_period current_period(pt::ptime(vehicle_journey->validity_pattern->beginning_date + bg::days(i), pt::seconds(0)),
                std::max(pt::seconds(86400), pt::seconds(vehicle_journey->stop_time_list.back()->arrival_time)));
        //on est en dehors la plage d'application du message
        if(!current_period.intersects(message.application_period)){
            continue;
        }

        if(vehicle_journey->validity_pattern->check(i) &&
                !vehicle_journey->adapted_validity_pattern->check(i)){
            //le VJ théorique ne circule pas: on recherche le vj adapté qui circule ce jour:
            for(auto* vj: vehicle_journey->adapted_vehicle_journey_list){
                if(vj->adapted_validity_pattern->check(i)){
                    vj_adapted = vj;
                    break;
                }
            }
            //si on n'a pas trouver de VJ adapté circulant, c'est qu'il a été supprimé
            if(vj_adapted == NULL){
                continue;
            }
        }
        //@TODO: faudrait probalement utilisé l'impact_list qui est utilisé en modéle
        std::vector<types::StopTime*> impacted_stop = get_stop_from_impact(message, current_period.begin().date(), vehicle_journey->stop_time_list);
        if(impacted_stop.size() < 1){
            continue;
        }

        if((vj_adapted == NULL)){
            vj_adapted = new types::VehicleJourney(*vehicle_journey);
            vj_adapted->theoric_vehicle_journey = vehicle_journey;
            vehicle_journey->adapted_vehicle_journey_list.push_back(vj_adapted);
            vj_adapted->uri = make_adapted_uri(vehicle_journey, data);
            data.vehicle_journeys.push_back(vj_adapted);
            vj_adapted->is_adapted = true;
            vj_adapted->validity_pattern = new types::ValidityPattern(vehicle_journey->validity_pattern->beginning_date);
            data.validity_patterns.push_back(vj_adapted->validity_pattern);

            vj_adapted->adapted_validity_pattern = new types::ValidityPattern(vj_adapted->validity_pattern->beginning_date);
            data.validity_patterns.push_back(vj_adapted->adapted_validity_pattern);
        }


        for(auto* stoptmp : impacted_stop){
            auto it = std::find(vj_adapted->stop_time_list.begin(), vj_adapted->stop_time_list.end(), stoptmp);
            if(it != vj_adapted->stop_time_list.end()){
                vj_adapted->stop_time_list.erase(it);
            }
        }
        vj_adapted->adapted_validity_pattern->add(current_period.begin().date());
        vehicle_journey->adapted_validity_pattern->remove(current_period.begin().date());
    }
}

void AtAdaptedLoader::init_map(const Data& data){
    for(auto* vj : data.vehicle_journeys){
        vj_map[vj->uri] = vj;
        if(vj->tmp_line != NULL){
            line_vj_map[vj->tmp_line->uri].push_back(vj);
            if(vj->tmp_line->network != NULL){
                network_vj_map[vj->tmp_line->network->uri].push_back(vj);
            }
        }
    }
}

std::vector<types::VehicleJourney*> AtAdaptedLoader::reconcile_impact_with_vj(const navitia::type::Message& message, const Data& data){
    std::vector<types::VehicleJourney*> result;

    if(message.object_type == navitia::type::Type_e::VehicleJourney){
        if(vj_map.find(message.object_uri) != vj_map.end()){
            //cas simple; le message pointe sur un seul vj
            result.push_back(vj_map[message.object_uri]);
        }
    }
    if(message.object_type == navitia::type::Type_e::Line){
        if(line_vj_map.find(message.object_uri) != line_vj_map.end()){
            //on à deja le mapping line => vjs
            return line_vj_map[message.object_uri];
        }
    }
    if(message.object_type == navitia::type::Type_e::Network){
        if(network_vj_map.find(message.object_uri) != network_vj_map.end()){
            return network_vj_map[message.object_uri];
        }
    }
//
    return result;
}


void AtAdaptedLoader::apply_deletion_on_vj(types::VehicleJourney* vehicle_journey, const std::vector<navitia::type::Message>& messages, Data& data){
    for(nt::Message m : messages){
        if(vehicle_journey->stop_time_list.size() > 0){
            delete_vj(vehicle_journey, m, data);
        }
    }
}

void AtAdaptedLoader::apply_update_on_vj(types::VehicleJourney* vehicle_journey, const std::vector<navitia::type::Message>& messages, Data& data){
    for(nt::Message m : messages){
        if(vehicle_journey->stop_time_list.size() > 0){
            duplicate_vj(vehicle_journey, m, data);
        }
    }
}

std::vector<types::VehicleJourney*> AtAdaptedLoader::get_vj_from_stoppoint(std::string stoppoint_uri, const Data& data){
    std::vector<types::VehicleJourney*> result;
    for(navimake::types::JourneyPatternPoint* jpp : data.journey_pattern_points){
        if(jpp->stop_point->uri == stoppoint_uri){
            //le journeypattern posséde le StopPoint
            navimake::types::JourneyPattern* jp = jpp->journey_pattern;
            //on récupere tous les vj qui posséde ce journeypattern
            for(navimake::types::VehicleJourney* vj : data.vehicle_journeys){
                if(vj->journey_pattern == jp){
                    result.push_back(vj);
                }
            }
        }
    }
    return result;
}

std::vector<types::VehicleJourney*> AtAdaptedLoader::get_vj_from_impact(const navitia::type::Message& message, const Data& data){
    std::vector<types::VehicleJourney*> result;
    if(message.object_type == navitia::type::Type_e::StopPoint){
        auto tmp = get_vj_from_stoppoint(message.object_uri, data);
        std::vector<types::VehicleJourney*>::iterator it;
        result.insert(it, tmp.begin(), tmp.end());
    }
    if(message.object_type == navitia::type::Type_e::StopArea){
        for(navimake::types::StopPoint* stp : data.stop_points){
            if(message.object_uri == stp->stop_area->uri){
                auto tmp = get_vj_from_stoppoint(stp->uri, data);
                std::vector<types::VehicleJourney*>::iterator it;
                result.insert(it, tmp.begin(), tmp.end());
            }
        }
    }
    return result;
}


void AtAdaptedLoader::dispatch_message(const std::map<std::string, std::vector<navitia::type::Message>>& messages, const Data& data){
    for(const std::pair<std::string, std::vector<navitia::type::Message>> & message_list : messages){
        for(auto m : message_list.second){
            //on recupére la liste des VJ associé(s) a l'uri du message
            if(m.object_type == nt::Type_e::VehicleJourney || m.object_type == nt::Type_e::Route
                    || m.object_type == nt::Type_e::Line || m.object_type == nt::Type_e::Network){
                std::vector<navimake::types::VehicleJourney*> vj_list = reconcile_impact_with_vj(m, data);
                //on parcourt la liste des VJ associée au message
                //et on associe le message au vehiclejourney
                for(auto vj  : vj_list){
                    update_vj_map[vj].push_back(m);
                }

            }else{
                if(m.object_type == nt::Type_e::JourneyPatternPoint || m.object_type == nt::Type_e::StopPoint
                        || m.object_type == nt::Type_e::StopArea){
                    std::vector<navimake::types::VehicleJourney*> vj_list = get_vj_from_impact(m, data);
                    for(auto vj  : vj_list){
                        duplicate_vj_map[vj].push_back(m);
                    }
                }
            }
        }
    }
}

void AtAdaptedLoader::apply(const std::map<std::string, std::vector<navitia::type::Message>>& messages, Data& data){
    init_map(data);

    dispatch_message(messages, data);

    std::cout << "nombre de VJ adaptés: " << update_vj_map.size() << std::endl;
    for(auto pair : update_vj_map){
        apply_deletion_on_vj(pair.first, pair.second, data);
    }
    for(auto pair : duplicate_vj_map){
        apply_update_on_vj(pair.first, pair.second, data);
    }
}

}//namespace
