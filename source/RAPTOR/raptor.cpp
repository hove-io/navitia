#include "raptor.h"

namespace raptor {

std::pair<unsigned int, bool> earliest_trip(unsigned int route, unsigned int stop_area, map_retour_t &retour, unsigned int count, navitia::type::Data &data){

    if(retour[count - 1].count(stop_area) == 0)
        return std::pair<unsigned int, bool>(-1, false);
    int time = retour[count -1][stop_area].temps;


    if(count > 1)
        time += 2*60;

    unsigned int day = retour[count -1][stop_area].jour;

    bool pam = false;
    if(time > 86400) {
        time = time %86400;
        ++day;
        pam = true;
    }

    int rp_id = get_rp_id(route, stop_area, data);
    if(rp_id == -1)
        return std::pair<unsigned int, bool>(-1, false);



    for(std::vector<unsigned int>::iterator it = std::lower_bound(data.pt_data.route_points.at(rp_id).vehicle_journey_list.begin(),
                                                                  data.pt_data.route_points.at(rp_id).vehicle_journey_list.end(),
                                                                  time, compare_rp(data.pt_data.route_points.at(rp_id), data));
        it != data.pt_data.route_points.at(rp_id).vehicle_journey_list.end(); ++it) {
        if(data.pt_data.validity_patterns.at(data.pt_data.vehicle_journeys.at(*it).validity_pattern_idx).check(day)) {
            return std::pair<unsigned int, bool>(*it, pam);
        }
    }


    ++day;

    for(std::vector<unsigned int>::iterator it = std::lower_bound(data.pt_data.route_points.at(rp_id).vehicle_journey_list.begin(),
                                                                  data.pt_data.route_points.at(rp_id).vehicle_journey_list.end(),
                                                                  0, compare_rp(data.pt_data.route_points.at(rp_id), data));
        it != data.pt_data.route_points.at(rp_id).vehicle_journey_list.end(); ++it) {
        if(data.pt_data.validity_patterns.at(data.pt_data.vehicle_journeys.at(*it).validity_pattern_idx).check(day)) {
            return std::pair<unsigned int, bool>(*it, true);
        }
    }



    return std::pair<unsigned int, bool>(-1, false);

}


int get_rp_id(navitia::type::Route &route, unsigned int stop_area, navitia::type::Data &data) {
    for(unsigned int i = 0; i < route.route_point_list.size();++i)
        if(data.pt_data.stop_points.at(data.pt_data.route_points.at(route.route_point_list[i]).stop_point_idx).stop_area_idx== stop_area)
            return route.route_point_list[i];
    return -1;
}

int get_rp_id(unsigned int route, unsigned int stop_area, navitia::type::Data &data) {
    return get_rp_id(data.pt_data.routes.at(route), stop_area, data);
}

int get_rp_order(navitia::type::Route &route, unsigned int stop_area, navitia::type::Data &data) {
    for(unsigned int i = 0; i < route.route_point_list.size();++i)
        if(data.pt_data.stop_points.at(data.pt_data.route_points.at(route.route_point_list[i]).stop_point_idx).stop_area_idx== stop_area)
            return i;
    return -1;
}

int get_rp_order(unsigned int route, unsigned int stop_area, navitia::type::Data &data) {
    return get_rp_order(data.pt_data.routes.at(route), stop_area, data);
}

int get_sa_rp(unsigned int order, int route, navitia::type::Data & data) {

    return data.pt_data.stop_points.at(data.pt_data.route_points.at(data.pt_data.routes.at(route).route_point_list.at(order)).stop_point_idx).stop_area_idx;
}

map_int_int_t make_queue(std::vector<unsigned int> stops, navitia::type::Data &data) {
    map_int_int_t retour;
    map_int_int_t::iterator it;
    BOOST_FOREACH(unsigned int said, stops) {
        BOOST_FOREACH(unsigned int spid, data.pt_data.stop_areas.at(said).stop_point_list) {
            BOOST_FOREACH(unsigned int rp, data.pt_data.stop_points.at(spid).route_point_list) {
                it = retour.find(data.pt_data.route_points.at(rp).route_idx);
                if(it == retour.end()) {
                    retour.insert(it, pair_int(data.pt_data.route_points.at(rp).route_idx, said));
                } else if(data.pt_data.route_points.at(rp).order < get_rp_order(data.pt_data.route_points.at(rp).route_idx, retour[data.pt_data.route_points.at(rp).route_idx], data)){
                    retour[data.pt_data.route_points.at(rp).route_idx] = said;
                }
            }
        }
    }

    return retour;
}

pair_retour_t RAPTOR(unsigned int depart, int arrivee, int debut, int date, navitia::type::Data &data) {
    int working_date = date;
    std::cout << "Debut date : " << working_date << std::endl;
    map_retour_t retour;
    map_best_t best;
    int et_temp ;
    BOOST_FOREACH(navitia::type::StopArea sa, data.pt_data.stop_areas)
            best[sa.idx] = type_best(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
    std::vector<unsigned int> marked_stop;
    int t, prev_temps, stid, said, temps_depart;
    unsigned int route, p;
    bool pam;

    marked_stop.push_back(depart);

    retour[0][depart] = type_retour(-1, debut, working_date);

    std::cout << "Depart temps : " << retour[0][depart].temps << std::endl;

    map_int_int_t Q;
    unsigned int count = 1;
    while(((Q.size() > 0) & (count < 10)) || count == 1) {

        Q = raptor::make_queue(marked_stop, data);
        marked_stop.clear();

        BOOST_FOREACH(map_int_int_t::value_type vq, Q) {
            route = vq.first;
            p = vq.second;
            t = -1;
            temps_depart = retour[count - 1][p].temps;
            working_date = retour[count - 1][p].jour;
            prev_temps = temps_depart;
            stid = -1;
            for(unsigned int i = get_rp_order(route, p, data); i < data.pt_data.routes.at(route).route_point_list.size(); ++i) {

                said = get_sa_rp(i, route, data);

                if(t != -1) {
                    stid = data.pt_data.vehicle_journeys.at(t).stop_time_list.at(i);
                    temps_depart = data.pt_data.stop_times.at(stid).departure_time%86400;
                    //Passe minuit
                    if(prev_temps > (data.pt_data.stop_times.at(stid).arrival_time%86400))
                        ++working_date;

                    //On stocke, et on marque pour explorer par la suite
                    if( type_best(working_date,data.pt_data.stop_times.at(stid).arrival_time%86400) < std::min(best[said], best[arrivee])) {
                        retour[count][said]  = type_retour(stid, data.pt_data.stop_times.at(stid).arrival_time, working_date);
                        best[said] = type_best(working_date,data.pt_data.stop_times.at(stid).arrival_time);
                        if(std::find(marked_stop.begin(), marked_stop.end(), said) == marked_stop.end())
                            marked_stop.push_back(said);
                    }
                }
                //Si on peut arriver plus tôt à l'arrêt en passant par une autre route
                if(retour[count-1].count(said) > 0)  {
                    if(retour[count-1][said].jour < working_date ||
                            ((retour[count-1][said].jour == working_date) && (retour[count-1][said].temps <= temps_depart))){
                        std::tie(et_temp, pam) = earliest_trip(route, said, retour, count, data);
                        if(et_temp >=0) {
                            t = et_temp;
                            working_date = retour[count -1][said].jour;
                            stid = data.pt_data.vehicle_journeys.at(t).stop_time_list.at(i);
                            if(pam)
                                ++working_date;
                        }
                    }
                }
                if(stid != -1)
                    prev_temps = data.pt_data.stop_times.at(stid).arrival_time % 86400;
            }
        }

        ++count;
    }

    return pair_retour_t(retour, best);
}

void afficher_chemin(unsigned int depart, int arrivee, pair_retour_t &pair_retour, unsigned int corr, navitia::type::Data &data) {

    map_retour_t retour = pair_retour.first;

    navitia::type::StopTime st = data.pt_data.stop_times.at(retour.at(corr).at(arrivee).stid), prev_st;
    navitia::type::StopArea sa = data.pt_data.stop_areas.at(data.pt_data.stop_points.at(data.pt_data.route_points.at(st.route_point_idx).stop_point_idx).stop_area_idx);
    navitia::type::Route route = data.pt_data.routes.at(data.pt_data.route_points.at(st.route_point_idx).route_idx);
    int                   date = retour.at(corr).at(arrivee).jour;

    std::vector<std::string> afficher;
    bool fin = false;
    unsigned int rp = get_rp_order(route, sa.idx, data);

    while((sa.idx != depart) && !fin && (retour.at(corr).count(sa.idx) > 0) && (rp > 0)) {

        afficher.push_back(boost::lexical_cast<std::string>(corr)+"J'arrive à " + sa.name + "(" +boost::lexical_cast<std::string>(sa.idx)+ ") avec la ligne "
                           +data.pt_data.lines.at(route.line_idx).name+" à "+boost::lexical_cast<std::string>(st.arrival_time)+ " le  "
                           + boost::lexical_cast<std::string>(retour.at(corr).at(sa.idx).jour)+ " à  "+ boost::lexical_cast<std::string>(retour.at(corr).at(sa.idx).temps)
                           + " vj : " + boost::lexical_cast<std::string>(data.pt_data.stop_times.at(retour.at(corr).at(sa.idx).stid).vehicle_journey_idx)
                           + " route : "+ boost::lexical_cast<std::string>(data.pt_data.route_points.at(data.pt_data.stop_times.at(retour.at(corr).at(sa.idx).stid).route_point_idx).route_idx));

        prev_st = st;
        rp      = get_rp_order(route, sa.idx, data) - 1;
        st      = data.pt_data.stop_times.at(data.pt_data.vehicle_journeys.at(st.vehicle_journey_idx).stop_time_list.at(rp));
        sa      = data.pt_data.stop_areas.at(data.pt_data.stop_points.at(data.pt_data.route_points.at(st.route_point_idx).stop_point_idx).stop_area_idx);
        route   = data.pt_data.routes.at(data.pt_data.route_points.at(st.route_point_idx).route_idx);

        if((data.pt_data.stop_times.at(prev_st.idx).departure_time%86400) < (data.pt_data.stop_times.at(st.idx).arrival_time%86400))
            date --;

        if((retour.at(corr - 1).count(sa.idx) > 0) && (corr > 1)) {
            if( ((date == retour.at(corr - 1).at(sa.idx).jour) && ((data.pt_data.stop_times.at(retour.at(corr - 1).at(sa.idx).stid).arrival_time%86400) < (st.arrival_time%86400)))
                    || (date < retour.at(corr - 1).at(sa.idx).jour)) {

                -- corr;
                afficher.push_back("Je pars de " + sa.name + "("+boost::lexical_cast<std::string>(sa.idx)+") avec la ligne "+data.pt_data.lines.at(route.line_idx).name
                                   +" à "+boost::lexical_cast<std::string>(st.departure_time)
                                   + " le  "+ boost::lexical_cast<std::string>(retour.at(corr).at(sa.idx).jour)+ " vj : " + boost::lexical_cast<std::string>(data.pt_data.stop_times.at(retour.at(corr).at(sa.idx).stid).vehicle_journey_idx)
                                   + " route : "+ boost::lexical_cast<std::string>(data.pt_data.route_points.at(data.pt_data.stop_times.at(retour.at(corr).at(sa.idx).stid).route_point_idx).route_idx));
                afficher.push_back("");
                st    = data.pt_data.stop_times.at(retour.at(corr).at(sa.idx).stid);
                route = data.pt_data.routes.at(data.pt_data.route_points.at(st.route_point_idx).route_idx);
                rp    = get_rp_order(route, sa.idx, data);
            }
        }

    }
    std::cout << "Corr : " << corr << std::endl;
    afficher.push_back("Je pars de " + sa.name + " avec la ligne "+data.pt_data.lines.at(route.line_idx).name+" à "+boost::lexical_cast<std::string>(st.departure_time));

    std::reverse(afficher.begin(), afficher.end());
    BOOST_FOREACH(std::string str, afficher) {
        std::cout << str << std::endl;
    }

}

itineraire_t make_itineraire(unsigned int depart, int arrivee, pair_retour_t &pair_retour, unsigned int corr, navitia::type::Data &data) {
    map_retour_t retour = pair_retour.first;
    itineraire_t to_return;
    navitia::type::StopTime st = data.pt_data.stop_times.at(retour.at(corr).at(arrivee).stid), prev_st;
    navitia::type::StopArea sa = data.pt_data.stop_areas.at(data.pt_data.stop_points.at(data.pt_data.route_points.at(st.route_point_idx).stop_point_idx).stop_area_idx);
    navitia::type::Route route = data.pt_data.routes.at(data.pt_data.route_points.at(st.route_point_idx).route_idx);
    int                   date = retour.at(corr).at(arrivee).jour;

    bool fin = false;
    unsigned int rp = get_rp_order(route, sa.idx, data);

    to_return.push_back(type_retour(st.idx, st.arrival_time, date));
    while((sa.idx != depart) && !fin && (retour.at(corr).count(sa.idx) > 0) && (rp > 0)) {
        prev_st = st;
        rp      = get_rp_order(route, sa.idx, data) - 1;
        st      = data.pt_data.stop_times.at(data.pt_data.vehicle_journeys.at(st.vehicle_journey_idx).stop_time_list.at(rp));
        sa      = data.pt_data.stop_areas.at(data.pt_data.stop_points.at(data.pt_data.route_points.at(st.route_point_idx).stop_point_idx).stop_area_idx);
        route   = data.pt_data.routes.at(data.pt_data.route_points.at(st.route_point_idx).route_idx);

        if((data.pt_data.stop_times.at(prev_st.idx).departure_time%86400) < (data.pt_data.stop_times.at(st.idx).arrival_time%86400))
            date --;

        if((retour.at(corr - 1).count(sa.idx) > 0) && (corr > 1)) {
            if( ((date == retour.at(corr - 1).at(sa.idx).jour) && ((data.pt_data.stop_times.at(retour.at(corr - 1).at(sa.idx).stid).arrival_time%86400) < (st.arrival_time%86400)))
                    || (date < retour.at(corr - 1).at(sa.idx).jour)) {
                to_return.push_back(type_retour(st.idx, st.arrival_time, date));
                -- corr;
                to_return.push_back(retour.at(corr).at(sa.idx));
                st    = data.pt_data.stop_times.at(retour.at(corr).at(sa.idx).stid);
                route = data.pt_data.routes.at(data.pt_data.route_points.at(st.route_point_idx).route_idx);
                rp    = get_rp_order(route, sa.idx, data);
            }
        }

    }
    to_return.push_back(type_retour(st.idx, st.departure_time, date));
    std::reverse(to_return.begin(), to_return.end());

    return to_return;
}


void afficher_itineraire(unsigned int depart, int arrivee, pair_retour_t &pair_retour, unsigned int corr, navitia::type::Data &data) {
    itineraire_t itineraire = make_itineraire(depart, arrivee, pair_retour, corr, data);


    int i = 0;
    BOOST_FOREACH(raptor::type_retour retour, itineraire) {
        if((i%2) == 0) {
            std::cout << "Je pars de " << data.pt_data.stop_areas.at(data.pt_data.stop_points.at(data.pt_data.route_points.at(data.pt_data.stop_times.at(retour.stid).route_point_idx).stop_point_idx).stop_area_idx).name
                      <<" avec la ligne " << data.pt_data.lines.at(data.pt_data.routes.at(data.pt_data.route_points.at(data.pt_data.stop_times.at(retour.stid).route_point_idx).route_idx).line_idx).name
                     << " à " << boost::lexical_cast<std::string>(data.pt_data.stop_times.at(retour.stid).departure_time)<<  " le  " << boost::lexical_cast<std::string>(retour.jour) << std::endl;
        } else {
            std::cout << "J'arrive à " << data.pt_data.stop_areas.at(data.pt_data.stop_points.at(data.pt_data.route_points.at(data.pt_data.stop_times.at(retour.stid).route_point_idx).stop_point_idx).stop_area_idx).name
                      << " avec la ligne " << data.pt_data.lines.at(data.pt_data.routes.at(data.pt_data.route_points.at(data.pt_data.stop_times.at(retour.stid).route_point_idx).route_idx).line_idx).name
                      << " à " << boost::lexical_cast<std::string>(data.pt_data.stop_times.at(retour.stid).arrival_time) << " le  " << boost::lexical_cast<std::string>(retour.jour) << std::endl;
        }

        ++i;
    }
}

}

