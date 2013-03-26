#include "adapted.h"
#include <boost/date_time/posix_time/posix_time.hpp>

namespace nt = navitia::type;
namespace bg = boost::gregorian;
namespace pt = boost::posix_time;

namespace navimake{


void delete_vj(types::VehicleJourney* vehicle_journey, const nt::Message& message, Data& data){
    //TODO on duplique les validitypattern à tort et à travers la, va falloir les mutualiser
    types::ValidityPattern* vp = NULL;
//    if(vehicle_journey->adapted_validity_pattern == NULL){//on duplique le validity pattern
//        vp = new types::ValidityPattern(*vehicle_journey->validity_pattern);
//    }else{
    vp = new types::ValidityPattern(*vehicle_journey->adapted_validity_pattern);
//    }
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

void AtAdaptedLoader::apply(const std::map<std::string, std::vector<navitia::type::Message>>& messages, Data& data){
    init_map(data);
    //on construit la liste des impacts pour chacun des vj impacté
    std::map<types::VehicleJourney*, std::vector<navitia::type::Message>> vj_messages_mapping;
    for(const std::pair<std::string, std::vector<navitia::type::Message>> & message_list : messages){
        for(auto message : message_list.second){
            for(auto* vj : reconcile_impact_with_vj(message, data)){
                vj_messages_mapping[vj].push_back(message);
            }
        }
    }
    std::cout << "nombre de VJ impactés: " << vj_messages_mapping.size() << std::endl;
    for(auto pair : vj_messages_mapping){
        apply_on_vj(pair.first, pair.second, data);
    }

}

void AtAdaptedLoader::apply_on_vj(types::VehicleJourney* vehicle_journey, const std::vector<navitia::type::Message>& messages, Data& data){
    for(nt::Message m : messages){
        if(m.object_type == nt::Type_e::VehicleJourney || m.object_type == nt::Type_e::Line
                || m.object_type == nt::Type_e::Route || m.object_type == nt::Type_e::Network){
            if(vehicle_journey->stop_time_list.size() > 0){
                delete_vj(vehicle_journey, m, data);

            }
        }
    }

}

}//namespace
