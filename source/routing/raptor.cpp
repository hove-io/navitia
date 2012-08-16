#include "raptor.h"
namespace navitia { namespace routing { namespace raptor {

communRAPTOR::communRAPTOR(navitia::type::Data &data) : data(data)
{
    //Construction de la liste des marche à pied
    BOOST_FOREACH(navitia::type::Connection connection, data.pt_data.connections) {
        foot_path[connection.departure_stop_point_idx].push_back(connection.idx);
    }
}

std::pair<unsigned int, bool> communRAPTOR::earliest_trip(unsigned int route, unsigned int order, map_int_pint_t &best, unsigned int count){
    const unsigned int stop_area = data.pt_data.stop_points.at(data.pt_data.route_points.at(data.pt_data.routes.at(route).route_point_list.at(order)).stop_point_idx).stop_area_idx;
    if(best.count(stop_area) == 0)
        return std::pair<unsigned int, bool>(-1, false);

    if(count > 1) {
        if((best[stop_area].dt + 2*60).date > best[stop_area].dt.date)
            return std::make_pair(earliest_trip(route, order, best[stop_area].dt + 2*60).first, true);
        else {
            return earliest_trip(route, order, best[stop_area].dt + 2*60);
        }
    } else
        return earliest_trip(route, order, best[stop_area].dt);
}


std::pair<unsigned int, bool> communRAPTOR::earliest_trip(unsigned int route, unsigned int order, map_retour_t &retour, unsigned int count){
    const unsigned int stop_area = data.pt_data.stop_points.at(data.pt_data.route_points.at(data.pt_data.routes.at(route).route_point_list.at(order)).stop_point_idx).stop_area_idx;
    if(retour[count - 1].count(stop_area) == 0)
        return std::pair<unsigned int, bool>(-1, false);

    int temps_correspondance = 0;
    if(count > 1 && retour[count -1][stop_area].stid != navitia::type::invalid_idx) {
        if(data.pt_data.route_points.at(data.pt_data.stop_times.at(retour[count -1][stop_area].stid).route_point_idx).route_idx != route) {
            temps_correspondance = 2 * 60;
        }
    }

    if(count > 1) {
        if((retour[count-1][stop_area].dt + temps_correspondance).date > retour[count-1][stop_area].dt.date)
            return std::make_pair(earliest_trip(route, order, retour[count-1][stop_area].dt + temps_correspondance).first, true);
        else
            return earliest_trip(route, order, retour[count-1][stop_area].dt + temps_correspondance);
    } else
        return earliest_trip(route, order, retour[count-1][stop_area].dt);
}


std::pair<unsigned int, bool> communRAPTOR::earliest_trip(unsigned int route, unsigned int order, DateTime dt){

    bool pam = false;
    if(dt.hour > 86400) {
        dt.hour = dt.hour %86400;
        ++dt.date;
        pam = true;
    }
    int rp_id = data.pt_data.routes.at(route).route_point_list.at(order);

    if(rp_id == -1) {
        std::cout << "Erreur ! " << std::endl;
        exit(1);
        return std::pair<unsigned int, bool>(-1, false);
    }



    for(auto it = std::lower_bound(data.pt_data.route_points[rp_id].vehicle_journey_list.begin(),
                                  data.pt_data.route_points[rp_id].vehicle_journey_list.end(),
                                  dt.hour, compare_rp(order, data));
        it != data.pt_data.route_points[rp_id].vehicle_journey_list.end(); ++it) {
        navitia::type::ValidityPattern vp = data.pt_data.validity_patterns[data.pt_data.vehicle_journeys[*it].validity_pattern_idx];

        if(vp.check(dt.date)) {
            return std::pair<unsigned int, bool>(*it, pam);
        }
    }
    ++dt.date;


    for(auto  it = std::lower_bound(data.pt_data.route_points[rp_id].vehicle_journey_list.begin(),
                                    data.pt_data.route_points[rp_id].vehicle_journey_list.end(),
                                    0, compare_rp(order, data));
        it != data.pt_data.route_points[rp_id].vehicle_journey_list.end(); ++it) {
        navitia::type::ValidityPattern vp = data.pt_data.validity_patterns[data.pt_data.vehicle_journeys[*it].validity_pattern_idx];
        if(vp.check(dt.date)) {
            return std::pair<unsigned int, bool>(*it, true);
        }
    }
    return std::pair<unsigned int, bool>(-1, false);
}


std::pair<unsigned int, bool> communRAPTOR::tardiest_trip(unsigned int route, unsigned int stop_area, map_int_pint_t &best, unsigned int count){
    if(best.count(stop_area) == 0)
        return std::pair<unsigned int, bool>(-1, false);

    if(count > 1) {
        if((best[stop_area].dt - 2*60).date < best[stop_area].dt.date)
            return std::make_pair(tardiest_trip(route, stop_area, best[stop_area].dt - 2*60).first, true);
        else {
            return tardiest_trip(route, stop_area, best[stop_area].dt - 2*60);
        }
    } else
        return tardiest_trip(route, stop_area, best[stop_area].dt);
}


std::pair<unsigned int, bool> communRAPTOR::tardiest_trip(unsigned int route, unsigned int stop_area, map_retour_t &retour, unsigned int count){
    if(retour[count - 1].count(stop_area) == 0)
        return std::pair<unsigned int, bool>(-1, false);

    if(count > 1) {
        if((retour[count-1][stop_area].dt - 2*60).date < retour[count-1][stop_area].dt.date)
            return std::make_pair(tardiest_trip(route, stop_area, retour[count-1][stop_area].dt - 2*60).first, true);
        else {
            return tardiest_trip(route, stop_area, retour[count-1][stop_area].dt - 2*60);
        }
    } else
        return tardiest_trip(route, stop_area, retour[count-1][stop_area].dt);
}


std::pair<unsigned int, bool> communRAPTOR::tardiest_trip(unsigned int route, unsigned int stop_area, DateTime dt){

    bool pam = false;
    if(dt.hour > 86400) {
        dt.normalize();
        pam = true;    BOOST_FOREACH(navitia::type::Route route, data.pt_data.routes) {
            if(route.line_idx == 9807) {
                BOOST_FOREACH(unsigned int rpidx, route.route_point_list) {
    //                if(data.pt_data.stop_points.at(data.pt_data.route_points.at(rpidx).stop_point_idx).stop_area_idx == 6344){
                        BOOST_FOREACH(unsigned int vjidx, data.pt_data.route_points.at(rpidx).vehicle_journey_list) {
                            std::cout <<data.pt_data.stop_points.at(data.pt_data.route_points.at(rpidx).stop_point_idx).stop_area_idx
                                     << " " << data.pt_data.stop_times.at(data.pt_data.vehicle_journeys.at(vjidx).stop_time_list.at(data.pt_data.route_points.at(rpidx).order)).departure_time
                                     << " " << data.pt_data.stop_times.at(data.pt_data.vehicle_journeys.at(vjidx).stop_time_list.at(data.pt_data.route_points.at(rpidx).order)).arrival_time  << std::endl;
                        }
    //                }
                }
            }
        }

    }

    int rp_id = get_rp_id(route, stop_area);
    if(rp_id == -1)
        return std::pair<unsigned int, bool>(-1, false);

    auto itdebug = data.pt_data.route_points[rp_id].vehicle_journey_list_arrival.rend();
    for(auto it = std::lower_bound(data.pt_data.route_points[rp_id].vehicle_journey_list_arrival.rbegin(),
                                   data.pt_data.route_points[rp_id].vehicle_journey_list_arrival.rend(),
                                   dt.hour, compare_rp_reverse(data.pt_data.route_points.at(rp_id).order, data));
        it != data.pt_data.route_points[rp_id].vehicle_journey_list_arrival.rend(); ++it) {
        if(itdebug == data.pt_data.route_points[rp_id].vehicle_journey_list_arrival.rend())
            itdebug = it;
        navitia::type::ValidityPattern vp = data.pt_data.validity_patterns[data.pt_data.vehicle_journeys[*it].validity_pattern_idx];
        if(vp.check(dt.date)) {/*
            if(data.pt_data.stop_times.at(data.pt_data.vehicle_journeys[*it].stop_time_list.at(data.pt_data.route_points.at(rp_id).order)).arrival_time %86400 <= dt.hour)*/
            return std::pair<unsigned int, bool>(*it, pam);
            //            else {
            //                std::cout << "Route point : " << rp_id << " " << data.pt_data.stop_times.at(data.pt_data.vehicle_journeys[*itdebug].stop_time_list.at(data.pt_data.route_points.at(rp_id).order)).arrival_time
            //                          << " " << *itdebug << " " << data.pt_data.stop_times.at(data.pt_data.vehicle_journeys[*it].stop_time_list.at(data.pt_data.route_points.at(rp_id).order)).arrival_time << " " << *it
            //                          << " " << dt.hour <<  std::endl;
            //                exit(1);
            //            }

        }
    }
    --dt.date;

    if(dt.date < 0)
        return std::pair<unsigned int, bool>(-1, false);

    for(auto  it = std::lower_bound(data.pt_data.route_points[rp_id].vehicle_journey_list_arrival.rbegin(),
                                    data.pt_data.route_points[rp_id].vehicle_journey_list_arrival.rend(),
                                    86400, compare_rp_reverse(data.pt_data.route_points[rp_id].order, data));
        it != data.pt_data.route_points[rp_id].vehicle_journey_list_arrival.rend(); ++it) {
        navitia::type::ValidityPattern vp = data.pt_data.validity_patterns[data.pt_data.vehicle_journeys[*it].validity_pattern_idx];
        if(vp.check(dt.date)) {
            return std::pair<unsigned int, bool>(*it, true);
        }
    }
    return std::pair<unsigned int, bool>(-1, false);
}




int communRAPTOR::get_rp_id(unsigned int route, unsigned int stop_area) {
    return get_rp_id(data.pt_data.routes[route], stop_area);
}

int communRAPTOR::get_rp_id(const navitia::type::Route &route, unsigned int stop_area) {
    for(unsigned int i = 0; i < route.route_point_list.size();++i) {
        if(data.pt_data.stop_points[data.pt_data.route_points[route.route_point_list[i]].stop_point_idx].stop_area_idx == stop_area) {
            return route.route_point_list[i];
        }
    }
    return -1;
}

int communRAPTOR::get_rp_order(unsigned int route, unsigned int stop_area) {
    return get_rp_order(data.pt_data.routes[route], stop_area);
}

int communRAPTOR::get_rp_order(const navitia::type::Route &route, unsigned int stop_area) {
    for(unsigned int i = 0; i < route.route_point_list.size();++i)
        if(data.pt_data.stop_points.at(data.pt_data.route_points.at(route.route_point_list.at(i)).stop_point_idx).stop_area_idx == stop_area) {
            return i;
        }
    return -1;
}

int communRAPTOR::get_sa_rp(unsigned int order, int route) {
    return data.pt_data.stop_points[data.pt_data.route_points[data.pt_data.routes[route].route_point_list[order]].stop_point_idx].stop_area_idx;
}






map_int_int_t communRAPTOR::make_queue(std::vector<unsigned int> stops) {
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


map_int_pairint_t communRAPTOR::make_queue2(std::vector<unsigned int> stops) {
    map_int_pairint_t retour;
    map_int_int_t::iterator it;
    BOOST_FOREACH(unsigned int said, stops) {
        BOOST_FOREACH(unsigned int spid, data.pt_data.stop_areas[said].stop_point_list) {
            BOOST_FOREACH(unsigned int rp, data.pt_data.stop_points[spid].route_point_list) {
                if(retour.count(data.pt_data.route_points[rp].route_idx) == 0 ||
                        data.pt_data.route_points[rp].order < retour[data.pt_data.route_points[rp].route_idx].second)
                    retour[data.pt_data.route_points[rp].route_idx] = pair_int(said, data.pt_data.route_points[rp].order);
            }
        }
    }

    return retour;
}




Path monoRAPTOR::compute_raptor(map_int_pint_t departs, map_int_pint_t destinations) {
    map_retour_t retour;
    map_int_pint_t best;
    std::vector<unsigned int> marked_stop;


    BOOST_FOREACH(auto item, departs) {
        retour[0][item.first] = item.second;
        best[item.first] = item.second;
        marked_stop.push_back(item.first);
    }

    best_dest b_dest;

    BOOST_FOREACH(auto item, destinations) {
        b_dest.ajouter_destination(item.first, item.second);
    }

    unsigned int count = 1;


    boucleRAPTOR(marked_stop, retour, best, b_dest, count);

    if(b_dest.best_now != type_retour()) {
        unsigned int destination_idx = b_dest.best_now_said;
        return makeBestPath(retour, best, departs, destination_idx, count);
    }
    Path result;
    return result;
}


std::vector<Path> monoRAPTOR::compute_all(map_int_pint_t departs, map_int_pint_t destinations) {
    map_retour_t retour;
    map_int_pint_t best;
    std::vector<unsigned int> marked_stop;


    BOOST_FOREACH(auto item, departs) {
        retour[0][item.first] = item.second;
        best[item.first] = item.second;
        marked_stop.push_back(item.first);
    }

    best_dest b_dest;

    BOOST_FOREACH(auto item, destinations) {
        b_dest.ajouter_destination(item.first, item.second);
    }

    unsigned int count = 1;


    {
        Timer t("boucle raptor ");
        boucleRAPTOR(marked_stop, retour, best, b_dest, count);
    }



    if(b_dest.best_now != type_retour()) {
        return makePathes(retour, best, departs, b_dest, count);
    }
    std::vector<Path> result;
    return result;
}

void RAPTOR::boucleRAPTOR(std::vector<unsigned int> &marked_stop, map_retour_t &retour, map_int_pint_t &best, best_dest &b_dest, unsigned int & count) {
    map_int_pairint_t Q;

    int et_temp ;

    int t, stid, said;
    unsigned int route, p;
    bool pam;

    std::vector<unsigned int> marked_stop_copy;
    marked_stop_copy = marked_stop;
    BOOST_FOREACH(auto stop_area, marked_stop_copy) {
        BOOST_FOREACH(auto stop_p, data.pt_data.stop_areas.at(stop_area).stop_point_list) {
            auto it_fp = foot_path.find(stop_p);
            if(it_fp != foot_path.end()) {
                BOOST_FOREACH(auto connection_idx, (*it_fp).second) {

                    unsigned int saiddest = data.pt_data.stop_points.at(data.pt_data.connections[connection_idx].destination_stop_point_idx).stop_area_idx;
                    if(best.find(saiddest) == best.end()) {
                        best[saiddest] = type_retour(navitia::type::invalid_idx, stop_area, DateTime(retour[0][stop_area].dt + data.pt_data.connections[connection_idx].duration), connection);
                        retour[0][saiddest] = type_retour(navitia::type::invalid_idx, stop_area, DateTime(retour[0][stop_area].dt + data.pt_data.connections[connection_idx].duration), connection);
                    } else {
                        if(best[saiddest].dt > DateTime(retour[0][stop_area].dt + data.pt_data.connections[connection_idx].duration)) {
                            best[saiddest] = type_retour(navitia::type::invalid_idx, stop_area, DateTime(retour[0][stop_area].dt + data.pt_data.connections[connection_idx].duration), connection);
                            retour[0][saiddest] = type_retour(navitia::type::invalid_idx, stop_area, DateTime(retour[0][stop_area].dt + data.pt_data.connections[connection_idx].duration), connection);
                        }
                    }
                    b_dest.ajouter_best(saiddest, type_retour(navitia::type::invalid_idx, stop_area, DateTime(retour[0][stop_area].dt + data.pt_data.connections[connection_idx].duration), connection));

                    marked_stop.push_back(saiddest);

                }
            }
        }
    }

    while(((Q.size() > 0) /*& (count < 10)*/) || count == 1) {

        Q = make_queue2(marked_stop);
        marked_stop.clear();

        BOOST_FOREACH(map_int_pairint_t::value_type vq, Q) {
            route = vq.first;
            p = vq.second.first;
            t = -1;
            int temps_depart = retour[count - 1][p].dt.hour;
            int working_date = retour[count - 1][p].dt.date;
            int prev_temps = temps_depart;
            stid = -1;
            int embarquement = p;




            for(unsigned int i = vq.second.second; i < data.pt_data.routes[route].route_point_list.size(); ++i) {
                said = get_sa_rp(i, route);
                if(t != -1) {


                    stid = data.pt_data.vehicle_journeys[t].stop_time_list[i];
                    temps_depart = data.pt_data.stop_times[stid].departure_time%86400;

                    //Passe minuit
                    if(prev_temps > (data.pt_data.stop_times[stid].arrival_time%86400))
                        ++working_date;


                    //On stocke, et on marque pour explorer par la suite


                    if(DateTime(working_date,data.pt_data.stop_times[stid].arrival_time%86400) < std::min(best[said].dt, b_dest.best_now.dt)) {

                        retour[count][said]  = type_retour(stid, embarquement, DateTime(working_date, data.pt_data.stop_times[stid].arrival_time%86400));
                        best[said] = type_retour(stid, DateTime(working_date, data.pt_data.stop_times[stid].arrival_time%86400));
                        b_dest.ajouter_best(said, type_retour(stid, DateTime(working_date, data.pt_data.stop_times[stid].arrival_time%86400)));

                        if(std::find(marked_stop.begin(), marked_stop.end(), said) == marked_stop.end()) {
                            marked_stop.push_back(said);
                        }
                    }
                }

                //Si on peut arriver plus tôt à l'arrêt en passant par une autre route
                if(retour[count-1].count(said) > 0)  {
                    if(retour[count-1][said].dt <= DateTime(working_date, temps_depart)){

                        std::tie(et_temp, pam) = earliest_trip(route, i, retour, count);
                        if((et_temp >=0)/* && (et_temp != t)*/) {

                            t = et_temp;
                            working_date = retour[count-1][said].dt.date;
                            stid = data.pt_data.vehicle_journeys[t].stop_time_list[i];

                            embarquement = said;


                            if(pam || (data.pt_data.stop_times.at(stid).arrival_time > 86400 && (temps_depart%86400)!=0))
                                ++working_date;
                        }
                    }
                }
                if(stid != -1)
                    prev_temps = data.pt_data.stop_times[stid].arrival_time % 86400;
            }
        }


        std::vector<unsigned int> marked_stop_copy;
        marked_stop_copy = marked_stop;
        BOOST_FOREACH(auto stop_area, marked_stop_copy) {
            BOOST_FOREACH(auto stop_p, data.pt_data.stop_areas.at(stop_area).stop_point_list) {
                auto it_fp = foot_path.find(stop_p);
                if(it_fp != foot_path.end()) {
                    BOOST_FOREACH(auto connection_idx, (*it_fp).second) {

                        unsigned int saiddest = data.pt_data.stop_points.at(data.pt_data.connections[connection_idx].destination_stop_point_idx).stop_area_idx;

                        if(best.count(saiddest) > 0)
                            best[saiddest] = std::min(best[saiddest], type_retour(navitia::type::invalid_idx, stop_area, DateTime(retour[count][stop_area].dt + data.pt_data.connections[connection_idx].duration), connection));
                        else
                            best[saiddest] = type_retour(navitia::type::invalid_idx, stop_area, DateTime(retour[count][stop_area].dt + data.pt_data.connections[connection_idx].duration), connection);
                        if(retour[count].count(saiddest) > 0)
                            retour[count][saiddest] = std::min(retour[count][saiddest], type_retour(navitia::type::invalid_idx, stop_area, DateTime(retour[count][stop_area].dt + data.pt_data.connections[connection_idx].duration), connection));
                        else
                            retour[count][saiddest] = type_retour(navitia::type::invalid_idx, stop_area, DateTime(retour[count][stop_area].dt + data.pt_data.connections[connection_idx].duration), connection);
                        b_dest.ajouter_best(saiddest, type_retour(navitia::type::invalid_idx, stop_area, DateTime(retour[count][stop_area].dt + data.pt_data.connections[connection_idx].duration), connection));
                        marked_stop.push_back(saiddest);

                    }
                }
            }
        }



        ++count;
    }



}


void reverseRAPTOR::boucleRAPTOR(std::vector<unsigned int> &marked_stop, map_retour_t &retour, map_int_pint_t &best, best_dest &b_dest, unsigned int & count) {
    map_int_int_t Q;

    int et_temp ;

    int t, stid, said;
    unsigned int route, p;
    bool pam;
    b_dest.reverse();


    //    std::cout << "retour 0 1 " << retour.at(0).at(1).dt << std::endl;
    while(((Q.size() > 0) /*& (count < 10)*/) || count == 1) {

        Q = make_queue(marked_stop);
        marked_stop.clear();


        BOOST_FOREACH(map_int_int_t::value_type vq, Q) {
            route = vq.first;
            p = vq.second;
            t = -1;
            int temps_depart = retour[count - 1][p].dt.hour;
            int working_date = retour[count - 1][p].dt.date;
            int prev_temps = temps_depart;
            stid = -1;
            int embarquement = p;
            for(int i = get_rp_order(route, p); i >= 0; --i) {
                said = get_sa_rp(i, route);


                if(t != -1) {

                    stid = data.pt_data.vehicle_journeys[t].stop_time_list[i];
                    temps_depart = data.pt_data.stop_times[stid].departure_time%86400;


                    //Passe minuit
                    if(prev_temps < (data.pt_data.stop_times[stid].arrival_time%86400))
                        --working_date;
                    //On stocke, et on marque pour explorer par la suite
                    type_retour max_said;
                    if(best.count(said) > 0)
                        max_said = std::max(best[said], b_dest.best_now);
                    else
                        max_said = b_dest.best_now;
                    if(type_retour(-1, DateTime(working_date,data.pt_data.stop_times[stid].arrival_time%86400)) > max_said) {
                        retour[count][said]  = type_retour(stid, embarquement, DateTime(working_date, data.pt_data.stop_times[stid].departure_time%86400));
                        best[said] = type_retour(stid, DateTime(working_date, data.pt_data.stop_times[stid].departure_time%86400));
                        b_dest.ajouter_best_reverse(said, type_retour(stid, DateTime(working_date, data.pt_data.stop_times[stid].departure_time%86400)));
                        if(std::find(marked_stop.begin(), marked_stop.end(), said) == marked_stop.end()) {
                            marked_stop.push_back(said);
                        }
                    }
                }

                //Si on peut arriver plus tôt à l'arrêt en passant par une autre route
                if(retour[count-1].count(said) > 0)  {
                    if(retour[count-1][said].dt >= DateTime(working_date, temps_depart)){
                        std::tie(et_temp, pam) = tardiest_trip(route, said, retour, count);
                        if(et_temp >=0) {

                            t = et_temp;
                            working_date = retour[count -1][said].dt.date;
                            stid = data.pt_data.vehicle_journeys[t].stop_time_list[i];
                            embarquement = said;
                            if(pam /*|| data.pt_data.stop_times.at(stid).arrival_time > 86400*/)
                                --working_date;
                        }
                    }
                }
                if(stid != -1)
                    prev_temps = data.pt_data.stop_times[stid].departure_time % 86400;
            }
        }


        std::vector<unsigned int> marked_stop_copy;
        marked_stop_copy = marked_stop;
        BOOST_FOREACH(auto stop_area, marked_stop_copy) {

            BOOST_FOREACH(auto stop_p, data.pt_data.stop_areas.at(stop_area).stop_point_list) {
                auto it_fp = foot_path.find(stop_p);
                if(it_fp != foot_path.end()) {
                    BOOST_FOREACH(auto connection_idx, (*it_fp).second) {

                        unsigned int saiddest = data.pt_data.stop_points.at(data.pt_data.connections[connection_idx].destination_stop_point_idx).stop_area_idx;

                        if(best.count(saiddest) > 0)
                            best[saiddest] = std::max(best[saiddest], type_retour(navitia::type::invalid_idx, stop_area, DateTime(retour[count][stop_area].dt - data.pt_data.connections[connection_idx].duration), connection));
                        else
                            best[saiddest] = type_retour(navitia::type::invalid_idx, stop_area, DateTime(retour[count][stop_area].dt - data.pt_data.connections[connection_idx].duration), connection);
                        if(retour[count].count(saiddest) > 0)
                            retour[count][saiddest] = std::max(retour[count][saiddest], type_retour(navitia::type::invalid_idx, stop_area, DateTime(retour[count][stop_area].dt - data.pt_data.connections[connection_idx].duration), connection));
                        else
                            retour[count][saiddest] = type_retour(navitia::type::invalid_idx, stop_area, DateTime(retour[count][stop_area].dt - data.pt_data.connections[connection_idx].duration), connection);
                        b_dest.ajouter_best(saiddest, type_retour(navitia::type::invalid_idx, stop_area, DateTime(retour[count][stop_area].dt - data.pt_data.connections[connection_idx].duration), connection));
                        marked_stop.push_back(saiddest);
                    }
                }
            }
        }



        ++count;
    }

}


Path monoRAPTOR::makeBestPath(const map_retour_t &retour, const map_int_pint_t &best, map_int_pint_t departs, unsigned int destination_idx, unsigned int count) {
    unsigned int countb = 1;
    for(;countb<=count;++countb) {
        if(retour.at(countb).count(destination_idx) > 0) {
            if((retour.at(countb).at(destination_idx).stid == best.at(destination_idx).stid) && (retour.at(countb).at(destination_idx).dt.hour == best.at(destination_idx).dt.hour) && (retour.at(countb).at(destination_idx).dt.date == best.at(destination_idx).dt.date)) {
                break;
            }
        }
    }

    return makePath(retour, best, departs, destination_idx, countb);
}

std::vector<Path> monoRAPTOR::makePathes(const map_retour_t &retour, const map_int_pint_t &best, map_int_pint_t departs, best_dest &b_dest, unsigned int count) {
    std::vector<Path> result;

    for(unsigned int i=1;i<count;++i) {
        BOOST_FOREACH(auto dest, b_dest.map_date_time) {
            if(retour.count(i) > 0) {
                if(retour.at(i).count(dest.first) > 0) {
                    if(retour.at(i).at(dest.first) != type_retour()) {
                        result.push_back(makePath(retour, best, departs, dest.first, i));
                    }
                }
            }
        }
    }

    return result;
}

Path RAPTOR::makePath(const map_retour_t &retour, const map_int_pint_t &best, map_int_pint_t departs, unsigned int destination_idx, unsigned int countb) {
    Path result;
    unsigned int current_said = destination_idx;

    type_retour r = retour.at(countb).at(current_said);
    DateTime workingDate = r.dt;
    navitia::type::StopTime current_st, prec_st;
    int said_embarquement = -1;

    bool stop = false;
    BOOST_FOREACH(auto item, departs) {
        stop = stop || (item.first == (int)current_said);
    }

    result.nb_changes = countb - 1;
    while(!stop) {
        bool debut = false, footpath = false;

        if(retour.count(countb) > 0) {
            if(retour.at(countb).count(current_said) > 0) {
                if(retour.at(countb).at(current_said).type == vj) {
                    if(said_embarquement == -1) {
                        r = retour.at(countb).at(current_said);
                        said_embarquement = r.said_emarquement;
                        current_st = data.pt_data.stop_times.at(r.stid);
                        workingDate = r.dt;
                        workingDate.normalize();
                        debut = true;
                    }
                } else {
                    if(said_embarquement == -1) {
                        r = retour.at(countb).at(current_said);
                        workingDate = r.dt;
                        workingDate.normalize();
                        result.items.push_back(PathItem(current_said, workingDate, workingDate));
                        current_said = r.said_emarquement;
                        said_embarquement = -1;
                        footpath = true;
                    }
                }
            }
        }
        if(!footpath) {
            if(!debut) {
                prec_st = current_st;
                current_st = data.pt_data.stop_times.at(data.pt_data.vehicle_journeys.at(current_st.vehicle_journey_idx).stop_time_list.at(current_st.order-1));
                if(current_st.arrival_time%86400 > prec_st.arrival_time%86400 && prec_st.vehicle_journey_idx!=navitia::type::invalid_idx)
                    --workingDate.date;
                workingDate.hour = current_st.arrival_time;
                workingDate.normalize();
            }
            current_said = data.pt_data.stop_points.at(data.pt_data.route_points.at(current_st.route_point_idx).stop_point_idx).stop_area_idx;
            if(said_embarquement == (int)current_said) {
                --countb;
                said_embarquement = -1;
            }
            result.items.push_back(PathItem(current_said, DateTime(workingDate.date, current_st.arrival_time), DateTime(workingDate.date, current_st.departure_time),
                                            data.pt_data.routes.at(data.pt_data.route_points.at(current_st.route_point_idx).route_idx).line_idx));

        }

        BOOST_FOREACH(auto item, departs) {
            stop = stop || (item.first == (int)current_said);
        }
    }
    std::reverse(result.items.begin(), result.items.end());


    result.duration = result.items.back().arrival - result.items.front().departure;

    int count_visites = 0;
    BOOST_FOREACH(auto t, best) {
        if(t.second.stid != navitia::type::invalid_idx) {
            ++count_visites;
        }
    }
    result.percent_visited = 100*count_visites / data.pt_data.stop_areas.size();

    return result;
}


Path reverseRAPTOR::makePath(const map_retour_t &retour, const map_int_pint_t &best, map_int_pint_t departs, unsigned int destination_idx, unsigned int countb) {
    Path result;
    unsigned int current_said = destination_idx;

    type_retour r = retour.at(countb).at(current_said);
    DateTime workingDate = r.dt;
    navitia::type::StopTime current_st, prec_st;
    int said_embarquement = -1;

    bool stop = false;
    BOOST_FOREACH(auto item, departs) {
        stop = stop || (item.first == (int)current_said);
    }

    while(!stop) {
        bool debut = false, footpath = false;

        if(retour.count(countb) > 0) {
            if(retour.at(countb).count(current_said) > 0) {
                if(retour.at(countb).at(current_said).type == vj) {
                    if(said_embarquement == -1) {
                        r = retour.at(countb).at(current_said);
                        said_embarquement = r.said_emarquement;
                        current_st = data.pt_data.stop_times.at(r.stid);
                        workingDate = r.dt;
                        workingDate.hour = current_st.departure_time;
                        workingDate.normalize();
                        debut = true;
                    }
                } else {
                    if(said_embarquement == -1) {
                        r = retour.at(countb).at(current_said);
                        workingDate = r.dt;
                        workingDate.normalize();
                        result.items.push_back(PathItem(current_said, workingDate, workingDate));
                        current_said = r.said_emarquement;
                        said_embarquement = -1;
                        footpath = true;
                    }
                }
            }
        }
        if(!footpath) {
            if(!debut) {
                prec_st = current_st;
                current_st = data.pt_data.stop_times.at(data.pt_data.vehicle_journeys.at(current_st.vehicle_journey_idx).stop_time_list.at(current_st.order+1));
                if(current_st.arrival_time%86400 < prec_st.arrival_time%86400 && prec_st.vehicle_journey_idx!=navitia::type::invalid_idx)
                    --workingDate.date;
                workingDate.hour = current_st.arrival_time;
                workingDate.normalize();
            }
            current_said = data.pt_data.stop_points.at(data.pt_data.route_points.at(current_st.route_point_idx).stop_point_idx).stop_area_idx;
            if(said_embarquement == (int)current_said) {
                --countb;
                ++ result.nb_changes;
                said_embarquement = -1;
                //                result.items.push_back(PathItem(current_said, DateTime(current_st.arrival_time, workingDate.date), DateTime(current_st.departure_time, workingDate.date),
                //                                                data.pt_data.routes.at(data.pt_data.route_points.at(current_st.route_point_idx).route_idx).line_idx));
            } /*else {
                result.items.push_back(PathItem(current_said, workingDate, workingDate,
                                                data.pt_data.routes.at(data.pt_data.route_points.at(current_st.route_point_idx).route_idx).line_idx));
            }*/
            result.items.push_back(PathItem(current_said, DateTime(workingDate.date, current_st.arrival_time), DateTime(workingDate.date, current_st.departure_time),
                                            data.pt_data.routes.at(data.pt_data.route_points.at(current_st.route_point_idx).route_idx).line_idx));
            //            std::cout << result << std::endl;

        }

        BOOST_FOREACH(auto item, departs) {
            stop = stop || (item.first == (int)current_said);
        }
    }


    result.duration = result.items.back().arrival - result.items.front().departure;

    int count_visites = 0;
    BOOST_FOREACH(auto t, best) {
        if(t.second.stid != navitia::type::invalid_idx) {
            ++count_visites;
        }
    }
    result.percent_visited = 100*count_visites / data.pt_data.stop_areas.size();

    return result;
}


Path communRAPTOR::compute(idx_t departure_idx, idx_t destination_idx, int departure_hour, int departure_day) {
    map_int_pint_t departs, destinations;
    departs[departure_idx] = type_retour(-1, DateTime(departure_day, departure_hour));
    destinations[destination_idx] = type_retour();
    return compute_raptor(departs, destinations);
}

std::vector<Path> monoRAPTOR::compute_all(navitia::type::EntryPoint departure, navitia::type::EntryPoint destination, int departure_hour, int departure_day) {
    map_int_pint_t departs, destinations;

    auto it_departure = data.pt_data.stop_area_map.find(departure.external_code),
            it_destination = data.pt_data.stop_area_map.find(destination.external_code);
    std::cout << "dep ext : " << departure.external_code << " " << data.pt_data.stop_area_map.size() << " " << data.pt_data.stop_areas.size() << std::endl;
    std::cout << "dest ext : " << destination.external_code << std::endl;

    if(it_departure == data.pt_data.stop_area_map.end() || it_destination == data.pt_data.stop_area_map.end()) {
        std::vector<Path> r;
        return r;
    }

    std::cout << "Depart de " << data.pt_data.stop_areas.at((*it_departure).second).name << " " << (*it_departure).second << std::endl;
    std::cout << "Destination " << data.pt_data.stop_areas.at((*it_destination).second).name << " " << (*it_destination).second << std::endl;

    departs[(*it_departure).second] = type_retour(-1, DateTime(departure_day, departure_hour), 0, 0);
    destinations[(*it_destination).second] = type_retour();
    return compute_all(departs, destinations);
}

Path communRAPTOR::compute(const type::GeographicalCoord & departure, double radius, idx_t destination_idx, int departure_hour, int departure_day) {
    map_int_pint_t bests, destinations;
    {
        Timer t("Recherche des stations de départ");

        typedef std::vector< std::pair<idx_t, double> > retour;
        retour prox;
        try {
            prox = (retour) (data.street_network.find_nearest(departure, data.pt_data.stop_area_proximity_list, radius));
        } catch(NotFound) {return Path();}
        BOOST_FOREACH(auto item, prox) {
            int temps = departure_hour + (item.second / 80);
            int day;
            if(temps > 86400) {
                temps = temps % 86400;
                day = departure_day + 1;
            } else {
                day = departure_day;
            }
            bests[item.first] = type_retour(-1, DateTime(day, temps));
        }
    }
    destinations[destination_idx] = type_retour();

    return compute_raptor(bests, destinations);
}

void communRAPTOR::trouverGeo(const type::GeographicalCoord & departure, double radius_depart, const type::GeographicalCoord & destination, double radius_destination,
                              int departure_hour, int departure_day, map_int_pint_t &departs, map_int_pint_t &destinations) {
    typedef std::vector< std::pair<idx_t, double> > retour;

    retour prox;

    try {
        prox = (retour) (data.street_network.find_nearest(departure, data.pt_data.stop_area_proximity_list, radius_depart));
    } catch(NotFound) {std::cout << "Not found 1 " << std::endl; return ;}


    BOOST_FOREACH(auto item, prox) {
        int temps = departure_hour + (item.second / 80);
        int day;
        if(temps > 86400) {
            temps = temps % 86400;
            day = departure_day + 1;
        } else {
            day = departure_day;
        }
        departs[item.first] = type_retour(-1, DateTime(day, temps), 0, (item.second / 80));
    }


    prox.clear();
    try {
        prox = (retour) (data.street_network.find_nearest(destination, data.pt_data.stop_area_proximity_list, radius_destination));
    } catch(NotFound) {std::cout << "Not found 2 " << std::endl;return ;}
    BOOST_FOREACH(auto item, prox) {
        destinations[item.first] = type_retour((int)(item.second/80));
    }
}

Path communRAPTOR::compute(const type::GeographicalCoord & departure, double radius_depart, const type::GeographicalCoord & destination, double radius_destination
                           , int departure_hour, int departure_day) {
    map_int_pint_t departs, destinations;

    trouverGeo(departure, radius_depart, destination, radius_destination, departure_hour, departure_day, departs, destinations);

    std::cout << "Nb stations departs : " << departs.size() << " destinations : " << destinations.size() << std::endl;

    Path result = compute_raptor(departs, destinations);
    return result;
}

std::vector<Path> monoRAPTOR::compute_all(const type::GeographicalCoord & departure, double radius_depart, const type::GeographicalCoord & destination, double radius_destination
                                          , int departure_hour, int departure_day) {
    map_int_pint_t departs, destinations;

    trouverGeo(departure, radius_depart, destination, radius_destination, departure_hour, departure_day, departs, destinations);

    std::cout << "Nb stations departs : " << departs.size() << " destinations : " << destinations.size() << std::endl;

    return compute_all(departs, destinations);
}


}}}
