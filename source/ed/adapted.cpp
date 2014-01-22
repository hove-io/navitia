#include "adapted.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include "utils/logger.h"
#include <boost/format.hpp>

namespace nt = navitia::type;
namespace bg = boost::gregorian;
namespace pt = boost::posix_time;

namespace ed{

nt::ValidityPattern* find_validity_pattern(nt::PT_Data& data, nt::ValidityPattern* validity_pattern){
    auto find_vp_predicate = [&](nt::ValidityPattern* vp1) { return validity_pattern->days == vp1->days;};
    auto it = std::find_if(data.validity_patterns.begin(),
                        data.validity_patterns.end(), find_vp_predicate);
    if(it != data.validity_patterns.end()) {
        delete validity_pattern;
        return *(it);
    } else {
         data.validity_patterns.push_back(validity_pattern);
         return validity_pattern;
    }
}

nt::ValidityPattern* get_validity_pattern(nt::ValidityPattern* validity_pattern,
                          const nt::AtPerturbation& pert,
                          nt::PT_Data& data, uint32_t time){
    nt::ValidityPattern* vp = new nt::ValidityPattern(*validity_pattern);

    for(size_t i=0; i < vp->days.size(); ++i){
        bg::date current_date = vp->beginning_date + bg::days(i);
        if(!vp->check(i) || current_date < pert.application_period.begin().date()
                || current_date > pert.application_period.end().date()){
            continue;
        }
        pt::ptime begin(current_date, pt::seconds(time));
        //si le départ du vj est dans la plage d'appliation du message, on le supprime
        if(pert.is_applicable(pt::time_period(begin, pt::seconds(1)))){
            vp->remove(current_date);
        }
    }
    return find_validity_pattern(data, vp);
}

nt::ValidityPattern* get_validity_pattern(nt::ValidityPattern* validity_pattern,
                          nt::PT_Data& data, boost::gregorian::date day, bool add){
    nt::ValidityPattern* vp = new nt::ValidityPattern(*validity_pattern);
    if(add){
        vp->add(day);
    }else{
        vp->remove(day);
    }
    return find_validity_pattern(data, vp);
}

void update_stop_times(nt::VehicleJourney* vehicle_journey, const nt::AtPerturbation& pert, nt::PT_Data& data){

    for(nt::StopTime* st : vehicle_journey->stop_time_list){
        st->departure_adapted_validity_pattern = get_validity_pattern(st->departure_adapted_validity_pattern,
                                                                      pert,
                                                                      data,
                                                                      st->departure_time
                                                                      );
        st->arrival_adapted_validity_pattern = get_validity_pattern(st->arrival_adapted_validity_pattern,
                                                                    pert,
                                                                    data,
                                                                    st->arrival_time
                                                                    );
    }
}

void delete_vj(nt::VehicleJourney* vehicle_journey,
        const nt::AtPerturbation& pert, nt::PT_Data& data){
   vehicle_journey->adapted_validity_pattern = get_validity_pattern(vehicle_journey->adapted_validity_pattern,
                                                                     pert,
                                                                     data,
                                                                     vehicle_journey->stop_time_list.front()->departure_time
                                                                     );

    update_stop_times(vehicle_journey, pert, data);
}

pt::time_period build_stop_period(const nt::StopTime& stop,
        const bg::date& date){
    pt::ptime departure, arrival;
    if(/*stop.departure_time < 0 ||*/ !stop.pick_up_allowed()){
        departure = pt::ptime(date, pt::seconds(stop.arrival_time + 1));
    }else{
        departure = pt::ptime(date, pt::seconds(stop.departure_time));
    }

    if(/*stop.arrival_time < 0 ||*/ !stop.drop_off_allowed()){
        arrival = pt::ptime(date, pt::seconds(stop.departure_time - 1));
    }else if(stop.arrival_time == stop.departure_time){
        //si l'heure d'arrivée et égal à l'heure de départ (typiquement donnée urbaine) on soustrait une seconde pour avoir une période non nulle
        arrival = pt::ptime(date, pt::seconds(stop.arrival_time - 1));
    }else{
        arrival = pt::ptime(date, pt::seconds(stop.arrival_time));
    }

    return pt::time_period(arrival, departure);
}

std::vector<nt::StopTime*> get_stop_from_impact(
        const navitia::type::AtPerturbation& perturbation,
        bg::date current_date,
        std::vector<nt::StopTime*> stoplist){
    std::vector<nt::StopTime*> result;
    if(perturbation.object_type == navitia::type::Type_e::StopPoint){
        for(auto stop : stoplist){
            if((stop->journey_pattern_point->stop_point->uri == perturbation.object_uri)
                    && (perturbation.is_applicable(build_stop_period(*stop, current_date)))){
                result.push_back(stop);
            }
        }
    }
    if(perturbation.object_type == navitia::type::Type_e::StopArea){
        for(auto stop : stoplist){
            if((stop->journey_pattern_point->stop_point->stop_area->uri == perturbation.object_uri)
                    && (perturbation.is_applicable(build_stop_period(*stop, current_date)))){
                result.push_back(stop);
            }
        }
    }
    return result;
}

std::string make_adapted_uri(const nt::VehicleJourney* vj, nt::PT_Data&){
    return vj->uri + ":adapted:"
        + boost::lexical_cast<std::string>(
                vj->adapted_vehicle_journey_list.size());
}

//duplique un VJ et tout ce qui lui est lié pour construire un VJ adapté
nt::VehicleJourney* create_adapted_vj(
        nt::VehicleJourney* current_vj, nt::VehicleJourney* theorical_vj,
        nt::PT_Data& data){
    //on duplique le VJ
    nt::VehicleJourney* vj_adapted = new nt::VehicleJourney(*current_vj);
    vj_adapted->uri = make_adapted_uri(theorical_vj, data);
    data.vehicle_journeys.push_back(vj_adapted);
    data.vehicle_journeys_map[vj_adapted->uri] = vj_adapted;
    //le nouveau VJ garde bien une référence vers le théorique, et non pas sur le VJ adapté dont il est issu.
    vj_adapted->theoric_vehicle_journey = theorical_vj;
    theorical_vj->adapted_vehicle_journey_list.push_back(vj_adapted);

    //si on pointe vers le meme validity pattern pour l'adapté et le théorique, on duplique
    if(theorical_vj->adapted_validity_pattern == theorical_vj->validity_pattern){
        theorical_vj->adapted_validity_pattern = new nt::ValidityPattern(*theorical_vj->validity_pattern);
        data.validity_patterns.push_back(theorical_vj->adapted_validity_pattern);
    }

    vj_adapted->is_adapted = true;
    vj_adapted->validity_pattern = new nt::ValidityPattern(current_vj->validity_pattern->beginning_date);
    data.validity_patterns.push_back(vj_adapted->validity_pattern);

    vj_adapted->adapted_validity_pattern = new nt::ValidityPattern(vj_adapted->validity_pattern->beginning_date);
    data.validity_patterns.push_back(vj_adapted->adapted_validity_pattern);

    //On duplique le journey pattern
    nt::JourneyPattern* jp = new nt::JourneyPattern(*vj_adapted->journey_pattern);
    data.journey_patterns.push_back(jp);
    vj_adapted->journey_pattern = jp;
    auto it = std::find(jp->vehicle_journey_list.begin(), jp->vehicle_journey_list.end(), current_vj);
    if(it != jp->vehicle_journey_list.end()){
        jp->vehicle_journey_list.erase(it);
    }
    jp->vehicle_journey_list.push_back(vj_adapted);
    jp->uri = vj_adapted->journey_pattern->uri + ":adapted:"+boost::lexical_cast<std::string>(data.journey_patterns.size());
    //@TODO changer l'uri

    //on duplique les journey pattern point
    jp->journey_pattern_point_list = std::vector<nt::JourneyPatternPoint*>();
    for(auto jpp : current_vj->journey_pattern->journey_pattern_point_list){
        auto new_jpp = new nt::JourneyPatternPoint(*jpp);
        data.journey_pattern_points.push_back(new_jpp);
        jp->journey_pattern_point_list.push_back(new_jpp);
        new_jpp->journey_pattern = jp;
        //@TODO changer l'uri
        new_jpp->uri = jpp->uri + ":adapted:"+boost::lexical_cast<std::string>(data.journey_pattern_points.size());
    }

    //On duplique les StopTime
    vj_adapted->stop_time_list = std::vector<nt::StopTime*>();
    for(unsigned int i=0; i<current_vj->stop_time_list.size(); ++i){
        auto* stop = current_vj->stop_time_list[i];
        nt::StopTime* new_stop = new nt::StopTime(*stop);
        new_stop->vehicle_journey = vj_adapted;
        new_stop->journey_pattern_point = jp->journey_pattern_point_list[i];

        new_stop->arrival_validity_pattern = vj_adapted->validity_pattern;
        new_stop->departure_validity_pattern = vj_adapted->validity_pattern;
        new_stop->arrival_adapted_validity_pattern = vj_adapted->adapted_validity_pattern;
        new_stop->departure_adapted_validity_pattern = vj_adapted->adapted_validity_pattern;

        vj_adapted->stop_time_list.push_back(new_stop);
        data.stop_times.push_back(new_stop);
    }

    return vj_adapted;
}

std::pair<bool, nt::VehicleJourney*> find_reference_vj(
        nt::VehicleJourney* vehicle_journey, int day_index){
    bool found = true;
    nt::VehicleJourney* reference_vj = vehicle_journey;

    if(vehicle_journey->validity_pattern->check(day_index)
            && !vehicle_journey->adapted_validity_pattern->check(day_index)){
        nt::VehicleJourney* tmp_vj = NULL;
        //le VJ théorique ne circule pas: on recherche le vj adapté qui circule ce jour:
        for(auto* vj: vehicle_journey->adapted_vehicle_journey_list){
            if(vj->adapted_validity_pattern->check(day_index)){
                tmp_vj = vj;
                break;
            }
        }
        //si on n'a pas trouvé de VJ adapté circulant, c'est qu'il a été supprimé
        if(tmp_vj == NULL){
            found = false;
        }else{
            //on a trouvé un VJ adapté qui cirule à cette date, c'est lui qui va servir de base
            reference_vj = tmp_vj;
        }
    }
    return std::make_pair(found, reference_vj);
}

std::vector<nt::StopTime*> duplicate_vj(nt::VehicleJourney* vehicle_journey,
        const nt::AtPerturbation& perturbation, nt::PT_Data& data){
    std::vector<nt::StopTime*> stop_to_delete;
    //map contenant le mapping entre un vj de référence et le vj adapté qui lui est lié
    //ceci permet de ne pas recréer un vj adapté à chaque fois que l'on change de vj de référence
    std::map<nt::VehicleJourney*, nt::VehicleJourney*> prec_vjs;
    //on teste le validitypattern adapté car si le VJ est déjà supprimé le traitement n'est pas nécessaire
    for(size_t i=0; i < vehicle_journey->validity_pattern->days.size(); ++i){
        nt::VehicleJourney* vj_adapted = NULL;
        if(!vehicle_journey->validity_pattern->check(i)){
            //si le vj théorique ne circule pas ne circule pas, on a rien  à faire
            continue;
        }
        //construction de la période de circulation du VJ entre [current_date minuit] et [current_date 23H59] dans le cas d'une
        //circulation normal, ou, l'heure d'arrivé dans le cas d'un train passe minuit (période plus de 24H)
        pt::time_period current_period(pt::ptime(vehicle_journey->validity_pattern->beginning_date + bg::days(i), pt::seconds(0)),
                std::max(pt::seconds(86400), pt::seconds(vehicle_journey->stop_time_list.back()->arrival_time)));

        if(!current_period.intersects(perturbation.application_period)){
            //on est en dehors la plage d'application du message
            continue;
        }

        bool found = false;
        nt::VehicleJourney* current_vj = NULL;
        boost::tie(found, current_vj) = find_reference_vj(vehicle_journey, i);
        if(!found){
            //si on n'a pas trouvé de VJ adapté circulant, c'est qu'il a été supprimé => on a rien à faire
            continue;
        }

        // on utilise la stop_time_list du vj de référence: ce n'est pas forcément le vj théorique!
        std::vector<nt::StopTime*> impacted_stop = get_stop_from_impact(perturbation, current_period.begin().date(), current_vj->stop_time_list);
        if(impacted_stop.empty()){
            continue;
        }
        //si on à deja utilisé ce vj de référence donc on reprend le vj adapté qui lui correspond
        if(prec_vjs.find(current_vj) != prec_vjs.end()){
            vj_adapted = prec_vjs[current_vj];
        }else{
            //si on à jamais utilisé ce vj, on construit un nouveau vj adapté
            vj_adapted = create_adapted_vj(current_vj, vehicle_journey, data);
        }

        for(auto* stoptmp : impacted_stop){
            auto it = std::find_if(vj_adapted->stop_time_list.begin(), vj_adapted->stop_time_list.end(),
                    [&stoptmp](const nt::StopTime* s1){
                        return (s1->arrival_time == stoptmp->arrival_time && s1->departure_time == stoptmp->departure_time
                        && s1->journey_pattern_point->stop_point == stoptmp->journey_pattern_point->stop_point);
                    });
            if(it != vj_adapted->stop_time_list.end()){
                //on ajoute le pointeur du stop à supprimer
                stop_to_delete.push_back(*it);
                //on recherche puis supprime le jpp associé au stop
                auto& jpp_list = vj_adapted->journey_pattern->journey_pattern_point_list;
                auto jpp_it = std::find(jpp_list.begin(), jpp_list.end(), (*it)->journey_pattern_point);
                if(jpp_it != jpp_list.end()){
                    jpp_list.erase(jpp_it);
                }
                //on supprime le stop de la liste;
                //à partir d'ici l'itérateur it n'est plus valide!!!
                vj_adapted->stop_time_list.erase(it);
            }
        }
        vj_adapted->adapted_validity_pattern->add(current_period.begin().date());
        for(navitia::type::StopTime* st : vj_adapted->stop_time_list){
            st->arrival_adapted_validity_pattern = get_validity_pattern(st->arrival_adapted_validity_pattern, data, current_period.begin().date(), true);
            st->departure_adapted_validity_pattern = get_validity_pattern(st->departure_adapted_validity_pattern, data, current_period.begin().date(), true);
        }
        current_vj->adapted_validity_pattern->remove(current_period.begin().date());
        for(navitia::type::StopTime* st : current_vj->stop_time_list){
            st->arrival_adapted_validity_pattern = get_validity_pattern(st->arrival_adapted_validity_pattern, data, current_period.begin().date(), false);
            st->departure_adapted_validity_pattern = get_validity_pattern(st->departure_adapted_validity_pattern, data, current_period.begin().date(), false);
        }
        prec_vjs[current_vj] = vj_adapted;
    }

    return stop_to_delete;
}

void AtAdaptedLoader::init_map(const nt::PT_Data& data){
    for(auto* vj : data.vehicle_journeys){
        vj_map[vj->uri] = vj;
        nt::Line* tmp_line = vj->journey_pattern->route->line;
        if(tmp_line != NULL){
            line_vj_map[tmp_line->uri].push_back(vj);
            if(tmp_line->network != NULL){
                network_vj_map[tmp_line->network->uri].push_back(vj);
            }
        }
        for(auto* stop : vj->stop_time_list){
            assert(stop->journey_pattern_point->stop_point != NULL);
            stop_point_vj_map[stop->journey_pattern_point->stop_point->uri].push_back(vj);
        }
    }

    for(auto* sp : data.stop_points){
        assert(sp->stop_area != NULL);
        stop_area_to_stop_point_map[sp->stop_area->uri].push_back(sp);
    }
}

std::vector<nt::VehicleJourney*> AtAdaptedLoader::reconcile_impact_with_vj(
        const navitia::type::AtPerturbation& perturbation, const nt::PT_Data&){
    std::vector<nt::VehicleJourney*> result;

    if(perturbation.object_type == nt::Type_e::VehicleJourney){
        if(vj_map.find(perturbation.object_uri) != vj_map.end()){
            //cas simple; le message pointe sur un seul vj
            result.push_back(vj_map[perturbation.object_uri]);
        }
    }
    if(perturbation.object_type == nt::Type_e::Line){
        if(line_vj_map.find(perturbation.object_uri) != line_vj_map.end()){
            //on à deja le mapping line => vjs
            return line_vj_map[perturbation.object_uri];
        }
    }
    if(perturbation.object_type == nt::Type_e::Network){
        if(network_vj_map.find(perturbation.object_uri) != network_vj_map.end()){
            return network_vj_map[perturbation.object_uri];
        }
    }
//
    return result;
}


void AtAdaptedLoader::apply_deletion_on_vj(nt::VehicleJourney* vehicle_journey,
        const std::set<nt::AtPerturbation>& perturbations, nt::PT_Data& data){
    for(nt::AtPerturbation pert : perturbations){
        if(vehicle_journey->stop_time_list.size() > 0){
            delete_vj(vehicle_journey, pert, data);
        }
    }
}

void AtAdaptedLoader::apply_update_on_vj(nt::VehicleJourney* vehicle_journey,
        const std::set<nt::AtPerturbation>& perturbations,
        nt::PT_Data& data){
    for(nt::AtPerturbation pert : perturbations){
        if(vehicle_journey->stop_time_list.size() > 0){
            auto tmp = duplicate_vj(vehicle_journey, pert, data);
            stop_to_delete.insert(stop_to_delete.end(), tmp.begin(), tmp.end());
        }
    }
    //une fois tous les messages traités pour un vj, on renumérote les horaires des vj adapté
    for(nt::VehicleJourney* vj : vehicle_journey->adapted_vehicle_journey_list){
        for(unsigned int i=0; i < vj->stop_time_list.size(); i++){
            vj->stop_time_list[i]->journey_pattern_point->order = i;
        }
    }
}

std::vector<nt::VehicleJourney*> AtAdaptedLoader::get_vj_from_stoppoint(
        const std::string& stoppoint_uri){
    return stop_point_vj_map[stoppoint_uri];
}

std::vector<nt::VehicleJourney*> AtAdaptedLoader::get_vj_from_stop_area(
        const std::string& stop_area_uri){
    std::vector<nt::VehicleJourney*> result;
    for(nt::StopPoint* sp : stop_area_to_stop_point_map[stop_area_uri]){
        auto tmp = get_vj_from_stoppoint(sp->uri);
        result.insert(result.end(), tmp.begin(), tmp.end());
    }
    return result;
}

std::vector<nt::VehicleJourney*> AtAdaptedLoader::get_vj_from_impact(
        const navitia::type::AtPerturbation& perturbation){
    std::vector<nt::VehicleJourney*> result;
    if(perturbation.object_type == navitia::type::Type_e::StopPoint){
        auto tmp = get_vj_from_stoppoint(perturbation.object_uri);
        result.insert(result.end(), tmp.begin(), tmp.end());
    }
    if(perturbation.object_type == navitia::type::Type_e::StopArea){
        auto tmp = get_vj_from_stop_area(perturbation.object_uri);
        result.insert(result.end(), tmp.begin(), tmp.end());
    }
    return result;
}


void AtAdaptedLoader::dispatch_perturbations(
        const std::vector<navitia::type::AtPerturbation>& perturbations,
        const nt::PT_Data& data){
    auto logger = log4cplus::Logger::getInstance("adapted");
    for(const auto& pert : perturbations){
        LOG4CPLUS_DEBUG(logger, boost::format("perturbation sur: %s") % pert.object_uri);
        //on recupére la liste des VJ associé(s) a l'uri du message
        if(pert.object_type == nt::Type_e::VehicleJourney
                || pert.object_type == nt::Type_e::Route
                || pert.object_type == nt::Type_e::Line
                || pert.object_type == nt::Type_e::Network){
            std::vector<nt::VehicleJourney*> vj_list = reconcile_impact_with_vj(pert, data);
            LOG4CPLUS_DEBUG(logger, boost::format("nb vj: %d") % vj_list.size());
            //on parcourt la liste des VJ associée au message
            //et on associe le message au vehiclejourney
            for(auto* vj  : vj_list){
                update_vj_map[vj].insert(pert);
            }

        }else if(pert.object_type == nt::Type_e::JourneyPatternPoint
                || pert.object_type == nt::Type_e::StopPoint
                || pert.object_type == nt::Type_e::StopArea){
            std::vector<nt::VehicleJourney*> vj_list = get_vj_from_impact(pert);
            for(auto* vj : vj_list){
                duplicate_vj_map[vj].insert(pert);
            }
        }
    }
}

void AtAdaptedLoader::clean(nt::PT_Data& data){
    //on supprime les journey pattern point associé au stop time, puis les stop time
    clean_journey_pattern_point(data);
    clean_stop_time(data);
}

void AtAdaptedLoader::clean_stop_time(nt::PT_Data& data){
    std::sort(stop_to_delete.begin(), stop_to_delete.end());
    std::unique(stop_to_delete.begin(), stop_to_delete.end());

    std::sort(data.stop_times.begin(), data.stop_times.end());

    size_t original_size = data.stop_times.size();
    size_t count = data.stop_times.size();


    int stop_index = data.stop_times.size() -1;
    int to_delete_index = stop_to_delete.size() -1;

    //on itére une seule fois sur les stops; on met à la fin ceux à supprimer, puis on redimensionne le tableau
    while(stop_index >= 0 && to_delete_index >= 0){
        if(data.stop_times[stop_index] < stop_to_delete[to_delete_index]){
            --to_delete_index;
            continue;
        }
        if(data.stop_times[stop_index] == stop_to_delete[to_delete_index]){
            delete data.stop_times[stop_index];
            data.stop_times[stop_index] = data.stop_times[count - 1];
            --count;
        }

        --stop_index;
    }
    data.stop_times.resize(count);
    assert(data.stop_times.size() == (original_size - stop_to_delete.size()));
    std::cout << "stop_to_delete: " << stop_to_delete.size() << std::endl;
}

void AtAdaptedLoader::clean_journey_pattern_point(nt::PT_Data& data){
    std::sort(stop_to_delete.begin(), stop_to_delete.end(),
            [](const nt::StopTime* s1, const nt::StopTime* s2){
            return s1->journey_pattern_point < s2->journey_pattern_point;});

    std::sort(data.journey_pattern_points.begin(), data.journey_pattern_points.end());

    size_t original_size = data.journey_pattern_points.size();
    size_t count = original_size;


    int jpp_index = original_size - 1;
    int to_delete_index = stop_to_delete.size() - 1;

    //on itére une seule fois sur les jpps; on met à la fin ceux à supprimer, puis on redimensionne le tableau
    while(jpp_index >= 0 && to_delete_index >= 0){
        if(data.journey_pattern_points[jpp_index] < stop_to_delete[to_delete_index]->journey_pattern_point){
            --to_delete_index;
            continue;
        }
        if(data.journey_pattern_points[jpp_index] == stop_to_delete[to_delete_index]->journey_pattern_point){
            auto jpp = data.journey_pattern_points[jpp_index]->journey_pattern;
            auto it = std::find(jpp->journey_pattern_point_list.begin(),
                    jpp->journey_pattern_point_list.end(),
                    data.journey_pattern_points[jpp_index]);
            if(it != jpp->journey_pattern_point_list.end()){
                jpp->journey_pattern_point_list.erase(it);
            }

            delete data.journey_pattern_points[jpp_index];
            data.journey_pattern_points[jpp_index] = data.journey_pattern_points[count - 1];
            --count;
        }

        --jpp_index;
    }
    data.journey_pattern_points.resize(count);
    assert(data.journey_pattern_points.size() == (original_size - stop_to_delete.size()));
}

void AtAdaptedLoader::apply(
        const std::vector<navitia::type::AtPerturbation>& perturbations,
        nt::PT_Data& data){
    if(perturbations.size() < 1){
        return;
    }
    init_map(data);
    unsigned int vj_count = data.vehicle_journeys.size();
    dispatch_perturbations(perturbations, data);

    std::cout << "update_vj_map: " << update_vj_map.size() << std::endl;
    std::cout << "duplicate_vj_map: " << duplicate_vj_map.size() << std::endl;

    std::vector<nt::StopTime*> stop_to_delete;
    for(auto pair : update_vj_map){
        apply_deletion_on_vj(pair.first, pair.second, data);
    }
    for(auto pair : duplicate_vj_map){
        apply_update_on_vj(pair.first, pair.second, data);
    }
    this->clean(data);
    std::cout << (data.vehicle_journeys.size() - vj_count) << " vj ajoutés lors de l'application des données adaptées" << std::endl;
}

}//namespace
