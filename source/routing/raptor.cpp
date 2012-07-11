#include "raptor.h"
namespace navitia { namespace routing { namespace raptor{

RAPTOR::RAPTOR(const navitia::type::Data &data) : data(data)
{
}

std::pair<unsigned int, bool> RAPTOR::earliest_trip(unsigned int route, unsigned int stop_area, map_retour_t &retour, unsigned int count){
    if(retour[count - 1].count(stop_area) == 0)
        return std::pair<unsigned int, bool>(-1, false);
    int time = retour[count -1][stop_area].dt.hour;


    if(count > 1)
        time += 2*60;

    unsigned int day = retour[count -1][stop_area].dt.date;

    return earliest_trip(route, stop_area, time, day);
}



std::pair<unsigned int, bool> RAPTOR::earliest_trip(unsigned int route, unsigned int stop_area, int time, int day){

    bool pam = false;
    if(time > 86400) {
        time = time %86400;
        ++day;
        pam = true;
    }

    int rp_id = get_rp_id(route, stop_area);
    if(rp_id == -1)
        return std::pair<unsigned int, bool>(-1, false);

    for(auto it = std::lower_bound(data.pt_data.route_points[rp_id].vehicle_journey_list.begin(),
                                   data.pt_data.route_points[rp_id].vehicle_journey_list.end(),
                                   time, compare_rp(data.pt_data.route_points.at(rp_id), data));
        it != data.pt_data.route_points[rp_id].vehicle_journey_list.end(); ++it) {
        navitia::type::ValidityPattern vp = data.pt_data.validity_patterns[data.pt_data.vehicle_journeys[*it].validity_pattern_idx];

        if(vp.check(day)) {
            return std::pair<unsigned int, bool>(*it, pam);
        }
    }
    ++day;

    for(auto  it = std::lower_bound(data.pt_data.route_points[rp_id].vehicle_journey_list.begin(),
                                    data.pt_data.route_points[rp_id].vehicle_journey_list.end(),
                                    0, compare_rp(data.pt_data.route_points[rp_id], data));
        it != data.pt_data.route_points[rp_id].vehicle_journey_list.end(); ++it) {
        navitia::type::ValidityPattern vp = data.pt_data.validity_patterns[data.pt_data.vehicle_journeys[*it].validity_pattern_idx];
        if(vp.check(day)) {
            return std::pair<unsigned int, bool>(*it, true);
        }
    }
    return std::pair<unsigned int, bool>(-1, false);
}




int RAPTOR::get_rp_id(unsigned int route, unsigned int stop_area) {
    return get_rp_id(data.pt_data.routes[route], stop_area);
}

int RAPTOR::get_rp_id(const navitia::type::Route &route, unsigned int stop_area) {
    for(unsigned int i = 0; i < route.route_point_list.size();++i)
        if(data.pt_data.stop_points[data.pt_data.route_points[route.route_point_list[i]].stop_point_idx].stop_area_idx == stop_area)
            return route.route_point_list[i];
    return -1;
}

int RAPTOR::get_rp_order(unsigned int route, unsigned int stop_area) {
    return get_rp_order(data.pt_data.routes[route], stop_area);
}

int RAPTOR::get_rp_order(const navitia::type::Route &route, unsigned int stop_area) {
    for(unsigned int i = 0; i < route.route_point_list.size();++i)
        if(data.pt_data.stop_points[data.pt_data.route_points[route.route_point_list[i]].stop_point_idx].stop_area_idx== stop_area)
            return i;
    return -1;
}

int RAPTOR::get_sa_rp(unsigned int order, int route) {
    return data.pt_data.stop_points[data.pt_data.route_points[data.pt_data.routes[route].route_point_list[order]].stop_point_idx].stop_area_idx;
}

bool RAPTOR::Best_Bag::ajouter_label(label lbl) {
    bool test = false;
    if(lbl[heure] > 86400) {
        ++lbl[jour];
        lbl[heure] = lbl[heure]%86400;
    }
    if(lbl[temps_trajet] < best[temps_trajet]) {
        best[temps_trajet] = lbl[temps_trajet];
        test = true;
    }
    if(lbl[via] > best[via]) {
        best[via] = lbl[via];
        test = true;
    }
    if(lbl[jour] < best[jour]) {
        best[jour] = lbl[jour];
        best[heure] = lbl[heure];
        test = true;
    } else if ((lbl[jour] == best[jour]) && (lbl[heure] < best[heure])) {
        best[heure] = lbl[heure];
        test = true;
    }

    return test;
}



bool RAPTOR::Bag::ajouter_label(label lbl) {
    bool test = false;

    if(lbl[heure] > 86400) {
        ++lbl[jour];
        lbl[heure] = lbl[heure]%86400;
    }

    if(!lbl.dominated_by(bp.best) && !lbl.dominated_by(bpt.best)) {
        if(lbl[temps_trajet] < best[temps_trajet]) {
            best[temps_trajet] = lbl[temps_trajet];
            test = true;
        }

        if(lbl[jour] < best[jour]) {
            best[jour] = lbl[jour];
            best[heure] = lbl[heure];
            test = true;
        } else if ((lbl[jour] == best[jour]) && (lbl[heure] < best[heure])) {
            best[heure] = lbl[heure];
            best[jour] = lbl[jour];
            test = true;
        }


        if(lbl[via] < best[via]) {
            best[via] = lbl[via] ;
            test = true;
        }

        if(test) {
            labels.push_back(lbl);
        }

        bp.ajouter_label(lbl);
    }
    return test;
}


struct dominates {
    label best;
    dominates(label best) : best(best) {}
    bool operator()(const label &lbl) {

        return best.criteres.at(temps_trajet) < lbl.criteres.at(temps_trajet)
                && ((best.criteres.at(jour) < lbl.criteres.at(jour)) ||((best.criteres.at(jour) == lbl.criteres.at(jour)) && (best.criteres.at(heure) < lbl.criteres.at(heure))))
                && (best.criteres.at(via) < lbl.criteres.at(via));
    }
};

bool RAPTOR::Bag::merge(Bag_route &bagr) {
    bool test = false;


    BOOST_FOREACH(label lbl, bagr.labels) {
        test = test || ajouter_label(lbl);
    }



    if(test) {
        auto iter = labels.begin();
        auto iter_end = labels.end();

        auto new_end = std::remove_if(iter, iter_end, dominates(best));
        labels = list_label_t(labels.begin(), new_end);
    }

    return test;
}



bool RAPTOR::Bag_route::ajouter_label(label lbl) {
    bool test = false;
    if(lbl[heure] > 86400) {
        ++lbl[jour];
        lbl[heure] = lbl[heure]%86400;
    }

    if(lbl[temps_trajet] < best[temps_trajet]) {
        best[temps_trajet] = lbl[temps_trajet];
        test = true;
    }

    if(lbl[jour] < best[jour]) {
        best[jour] = lbl[jour];
        best[heure] = lbl[heure];
        test = true;
    } else if ((lbl[jour] <= best[jour]) && (lbl[heure] < best[heure])) {
        best[heure] = lbl[heure];
        best[jour] = lbl[jour];
        test = true;
    }

    if(lbl[via] < best[via]) {
        best[via] = lbl[via];
        test = true;
    }

    if(test) {
        labels.push_back(lbl);
    }
    return test;
}

void RAPTOR::Bag_route::update(unsigned int order, unsigned int said_via) {
    best = label();

    for(auto iter = labels.begin(); iter != labels.end(); ++iter) {
        if((*iter).stid != -1) {
            unsigned int vj = raptor->data.pt_data.stop_times.at((*iter).stid).vehicle_journey_idx;
            (*iter).stid = raptor->data.pt_data.vehicle_journeys.at(vj).stop_time_list.at(order);

            unsigned int temps_depart = raptor->data.pt_data.stop_times.at(raptor->data.pt_data.vehicle_journeys.at(vj).stop_time_list.at(order -1)).arrival_time % 86400 ;
            unsigned int temps_arrivee = raptor->data.pt_data.stop_times.at((*iter).stid).arrival_time % 86400;


            (*iter).criteres[heure] = temps_arrivee;
            if(temps_arrivee >= temps_depart)
                (*iter).criteres[temps_trajet] += temps_arrivee - temps_depart;
            else
                (*iter).criteres[temps_trajet] += 86400 - temps_depart + temps_arrivee ;

            if(temps_depart  > temps_arrivee)
                (*iter).criteres[jour] +=1;


            if(raptor->data.pt_data.stop_points.at(raptor->data.pt_data.route_points.at(raptor->data.pt_data.stop_times.at((*iter).stid).route_point_idx).stop_point_idx).stop_area_idx == said_via) {
                (*iter).criteres[via] = 0;
                best[via] = 0;
            }

            if((*iter).criteres[jour] < best[jour]) {

                best[jour] = (*iter).criteres[jour];
                best[heure] = (*iter).criteres[heure] ;
            } else if(((*iter).criteres[jour] == best[jour]) && (((*iter).criteres[heure]) < best[heure])) {

                best[heure] = (*iter).criteres[heure] ;
                best[jour] = (*iter).criteres[jour];
            }
            if((*iter).criteres[jour] == 18 && (*iter).criteres[heure] < 28800) {
                std::cout << "probleme " <<  (*iter).stid << " " << route << " " << order << std::endl;
                exit(1);
            }

            if((*iter).criteres[temps_trajet] < best[temps_trajet])
                best[temps_trajet] = (*iter).criteres[temps_trajet];
        }


    }

}


bool RAPTOR::Bag_route::merge(Bag &bag, unsigned int order) {
    bool test = false;
    unsigned int said = raptor->data.pt_data.stop_points.at(raptor->data.pt_data.route_points.at(raptor->data.pt_data.routes.at(route).route_point_list.at(order)).stop_point_idx).stop_area_idx;
    int etemp;
    bool pam;

    BOOST_FOREACH(label lbl, bag.labels) {
        std::tie(etemp, pam) = raptor->earliest_trip(route, said, lbl[heure], lbl[jour]);
        if(etemp != -1) {
            label lbl_temp(temps_trajet, lbl[temps_trajet]);
            lbl_temp.ajouter_critere(heure, raptor->data.pt_data.stop_times.at(raptor->data.pt_data.vehicle_journeys.at(etemp).stop_time_list.at(order)).arrival_time);
            if(pam)
                lbl_temp.ajouter_critere(jour, lbl[jour]+1);
            else
                lbl_temp.ajouter_critere(jour, lbl[jour]);
            lbl_temp.stid = raptor->data.pt_data.vehicle_journeys.at(etemp).stop_time_list.at(order);
            test = test || ajouter_label(lbl_temp);
        }


    }

    auto iter = labels.begin();
    auto iter_end = labels.end();
    auto new_end = std::remove_if(iter, iter_end, dominates(best));
    labels = list_label_t(labels.begin(), new_end);

    return test;
}




map_int_int_t RAPTOR::make_queue(std::vector<unsigned int> stops) {
    map_int_int_t retour;
    map_int_int_t::iterator it;
    BOOST_FOREACH(unsigned int said, stops) {
        BOOST_FOREACH(unsigned int spid, data.pt_data.stop_areas[said].stop_point_list) {
            BOOST_FOREACH(unsigned int rp, data.pt_data.stop_points[spid].route_point_list) {
                if(retour.count(data.pt_data.route_points[rp].route_idx) == 0 ||
                        data.pt_data.route_points[rp].order < get_rp_order(data.pt_data.route_points[rp].route_idx, retour[data.pt_data.route_points[rp].route_idx]))
                    retour[data.pt_data.route_points[rp].route_idx] = said;
            }
        }
    }

    return retour;
}

Path RAPTOR::compute(idx_t departure_idx, idx_t destination_idx, int departure_hour, int departure_day) {
    Path result;

    int working_date = departure_day;
    map_retour_t retour;
    map_int_pint_t best;
    int et_temp ;
    BOOST_FOREACH(navitia::type::StopArea sa, data.pt_data.stop_areas)
            best[sa.idx] = type_retour();
    std::vector<unsigned int> marked_stop;
    int t, prev_temps, stid, said, temps_depart;
    unsigned int route, p;
    bool pam;

    marked_stop.push_back(departure_idx);

    retour[0][departure_idx] = type_retour(-1, DateTime(working_date, departure_hour));


    map_int_int_t Q;
    unsigned int count = 1;

    while(((Q.size() > 0) /*& (count < 10)*/) || count == 1) {

        Q = make_queue(marked_stop);
        marked_stop.clear();

        BOOST_FOREACH(map_int_int_t::value_type vq, Q) {
            route = vq.first;
            p = vq.second;
            t = -1;
            temps_depart = retour[count - 1][p].dt.hour;
            working_date = retour[count - 1][p].dt.date;
            prev_temps = temps_depart;
            stid = -1;
            for(unsigned int i = get_rp_order(route, p); i < data.pt_data.routes[route].route_point_list.size(); ++i) {
                said = get_sa_rp(i, route);

                if(t != -1) {
                    stid = data.pt_data.vehicle_journeys[t].stop_time_list[i];
                    temps_depart = data.pt_data.stop_times[stid].departure_time%86400;
                    //Passe minuit
                    if(prev_temps > (data.pt_data.stop_times[stid].arrival_time%86400))
                        ++working_date;
                    //On stocke, et on marque pour explorer par la suite
                    if(type_retour(-1, DateTime(working_date,data.pt_data.stop_times[stid].arrival_time%86400)) < std::min(best[said], best[destination_idx])) {
                        retour[count][said]  = type_retour(stid, DateTime(working_date, data.pt_data.stop_times[stid].arrival_time%86400));
                        best[said] = type_retour(stid, DateTime(working_date, data.pt_data.stop_times[stid].arrival_time%86400));

                        if(std::find(marked_stop.begin(), marked_stop.end(), said) == marked_stop.end())
                            marked_stop.push_back(said);
                    }
                }
                //Si on peut arriver plus tôt à l'arrêt en passant par une autre route
                if(retour[count-1].count(said) > 0)  {
                    if(retour[count-1][said].dt <= DateTime(working_date, temps_depart)){
                        std::tie(et_temp, pam) = earliest_trip(route, said, retour, count);
                        if(et_temp >=0) {
                            t = et_temp;
                            working_date = retour[count -1][said].dt.date;
                            stid = data.pt_data.vehicle_journeys[t].stop_time_list[i];
                            if(pam || data.pt_data.stop_times.at(stid).arrival_time > 86400)
                                ++working_date;

                        }

                    }
                }
                if(stid != -1)
                    prev_temps = data.pt_data.stop_times[stid].arrival_time % 86400;
            }
        }
        ++count;
    }






    if(best[destination_idx] != type_retour()) {
        unsigned int current_said = destination_idx;

        unsigned int countb = 0;
        for(;countb<=count;++countb) {
            if(retour[countb][destination_idx].stid == best[destination_idx].stid) {
                break;
            }
        }
        navitia::type::StopTime st = data.pt_data.stop_times.at(retour[countb][destination_idx].stid);
        type_retour precretour = retour[countb][destination_idx];
        int countdebug = 0;
        int heure = data.pt_data.stop_times.at(precretour.stid).departure_time;
        int date = precretour.dt.date;
        result.items.push_back(PathItem(current_said,
                                        heure, date, data.pt_data.routes.at(data.pt_data.route_points.at(data.pt_data.stop_times.at(precretour.stid).route_point_idx).route_idx).line_idx ));
        while(current_said != departure_idx && countdebug < 50) {
            ++ countdebug;

            if(retour[(countb-1)][current_said].dt < retour[countb][current_said].dt ||
                    best[current_said].dt < retour[countb][current_said].dt) {
                int heure = data.pt_data.stop_times.at(precretour.stid).departure_time;
                int date = precretour.dt.date;
                std::string line = data.pt_data.lines.at(data.pt_data.routes.at(data.pt_data.route_points.at(data.pt_data.stop_times.at(precretour.stid).route_point_idx).route_idx).line_idx).name;
                result.items.push_back(PathItem(current_said, heure, date,
                                                data.pt_data.routes.at(data.pt_data.route_points.at(data.pt_data.stop_times.at(precretour.stid).route_point_idx).route_idx).line_idx));

                countb = 0;
                for(;countb<=count;++countb) {
                    if(retour[countb][current_said].dt == best[current_said].dt) {
                        break;
                    }
                }
                ++ result.nb_changes;


            }
            result.items.push_back(PathItem(current_said,
                                            retour[countb][current_said].dt.hour, retour[countb][current_said].dt.date,
                                            data.pt_data.routes.at(data.pt_data.route_points.at(data.pt_data.stop_times.at(retour[countb][current_said].stid).route_point_idx).route_idx).line_idx));

            precretour = retour[countb][current_said];
            st = data.pt_data.stop_times.at(retour[countb][current_said].stid);
            current_said = data.pt_data.stop_points.at(data.pt_data.route_points.at(data.pt_data.stop_times.at(data.pt_data.vehicle_journeys.at(st.vehicle_journey_idx).stop_time_list.at(st.order -1)).route_point_idx).stop_point_idx).stop_area_idx;

        }
        std::reverse(result.items.begin(), result.items.end());



        result.duration = result.items.back().time - result.items.front().time;
        if(result.items.back().day != result.items.front().day)
            result.duration += (result.items.back().day - result.items.front().day) * 86400;

        int count_visites = 0;
        BOOST_FOREACH(auto t, best) {
            if(t.second.stid != -1) {
                ++count_visites;
            }
        }
        result.percent_visited = 100*count_visites / data.pt_data.stop_areas.size();
    }

    return result;
}


/// Implémentation de RAPTOR pour du multicritères
void RAPTOR::McRAPTOR(unsigned int depart, int arrivee, int debut, int date, unsigned int said_via) {
    bags_t bags(data.pt_data.stop_areas.size(), arrivee, this);

    std::vector<unsigned int> marked_stop;
    unsigned int route, p;


    bags[std::make_pair(0, depart)].ajouter_label(label(heure, debut).ajouter_critere(jour, date).ajouter_critere(temps_trajet, 0));
    marked_stop.push_back(depart);
    //Pour tous les k
    map_int_int_t Q;
    unsigned int count = 1;

    unsigned int count_marked_stop = 0;
    while(((Q.size() > 0) && (count < 5)) || count == 1) {
        //On intialise la queue
        Q = this->make_queue(marked_stop);
        marked_stop.clear();
        BOOST_FOREACH(map_int_int_t::value_type vq, Q) {
            route = vq.first;
            p = vq.second;
            //On initialise un bad vide Br
            Bag_route Br(this, route);
            //On traverse chaque route r en  partant de p
            for(unsigned int i = get_rp_order(route, p); i < data.pt_data.routes[route].route_point_list.size(); ++i) {
                unsigned int said = get_sa_rp(i, route);
                Br.update(i, said_via);


                if(bags[std::make_pair(count, said)].merge(Br)) {
                    if(bags[std::make_pair(count, said)].best.criteres[via] == 0 && said == 16331)
                        std::cout << "via " << count << " " << said << std::endl;
                    marked_stop.push_back(said);
                    ++count_marked_stop;
                }

                if(bags.exists(std::make_pair(count-1, said))) {
                    Br.merge(bags[std::make_pair(count-1, said)], i);
                    if(Br.best.criteres[via] == 0)
                        std::cout << "via " << std::endl;
                }
            }
        }
        std::cout  << " count : " << count << " " << Q.size() << std::endl;
        ++count;
    }

    std::cout << "Marked stop " << count_marked_stop << std::endl;

    for(unsigned int i=0; i < count; ++i) {
        if(bags.bags.find(std::make_pair(i, arrivee)) != bags.bags.end()) {
            std::cout << "Cible atteinte " << i << std::endl;
            BOOST_FOREACH(auto label, bags[std::make_pair(i, arrivee)].labels) {
                std::cout << " i : " << i << " arrivee : " << label[heure] << " jour : " << label[jour] << " temps trajet : " << label[temps_trajet] << " via : " << label[via] << std::endl;
            }

        } else {
            std::cout << "Cible non atteinte " << i << std::endl;
        }
    }
}




}}}
