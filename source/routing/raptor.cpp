#include "raptor.h"
namespace navitia { namespace routing { namespace raptor {

communRAPTOR::communRAPTOR(navitia::type::Data &data) : data(data), cp(data), routes(data.pt_data.routes.size()), stopTimes(data.pt_data.stop_times.size())
{
    //Construction de la liste des marche à pied
    BOOST_FOREACH(navitia::type::Connection connection, data.pt_data.connections) {
        foot_path[connection.departure_stop_point_idx].push_back(connection.idx);
    }

    navitia::type::idx_t i = 0;
    navitia::type::idx_t i_st = 0;
    BOOST_FOREACH(navitia::type::Route route, data.pt_data.routes) {
        routes[i].idx = route.idx;
        routes[i].nbStops = route.route_point_list.size();
        if(route.route_point_list.size() > 0) {
            routes[i].nbTrips = data.pt_data.route_points[route.route_point_list.front()].vehicle_journey_list.size();
            routes[i].firstStopTime = i_st;
            routes[i].first_orderVj.reserve(routes[i].nbTrips * sizeof(int));
            int vji = 0;
            BOOST_FOREACH(navitia::type::idx_t vjidx, data.pt_data.route_points[route.route_point_list.front()].vehicle_journey_list) {
                BOOST_FOREACH(navitia::type::idx_t stidx, data.pt_data.vehicle_journeys[vjidx].stop_time_list) {
                    stopTimes[i_st] = data.pt_data.stop_times[stidx];
                    ++i_st;
                }
                routes[i].first_orderVj[vji] = navitia::type::invalid_idx;
                ++vji;
            }
        }
        else {
            routes[i].nbTrips = 0;
            routes[i].firstStopTime = 0;
        }

        ++i;
    }


    //        int bad = 0, good = 0;

    //        for(unsigned int k = 0; k < routes.size(); ++k) {
    //            Route_t route = routes[k];
    //            for(int i=0; i < route.nbStops;++i) {
    //                //        int i = 0;
    //                for(int j=1; j < route.nbTrips;++j) {
    //                    navitia::type::StopTime st1 = stopTimes[get_stop_time_idx(k,j-1,i)], st2 = stopTimes[get_stop_time_idx(k,j,i)];
    //                    std::bitset<366> verif(0);
    //                    if((st1.departure_time ) > (st2.departure_time ) &&
    //                            ((data.pt_data.validity_patterns[data.pt_data.vehicle_journeys[st1.vehicle_journey_idx].validity_pattern_idx].days & data.pt_data.validity_patterns[data.pt_data.vehicle_journeys[st2.vehicle_journey_idx].validity_pattern_idx].days)
    //                             != verif)) {
    //                        std::cout << "order : " << i << " vj1 : " << st1.vehicle_journey_idx << " " << data.pt_data.vehicle_journeys[st1.vehicle_journey_idx].external_code
    //                                  << " vj2 : " << st2.vehicle_journey_idx << " " << data.pt_data.vehicle_journeys[st2.vehicle_journey_idx].external_code << std::endl;
    //                        exit(1);
    //                        ++bad;
    //                    } else {
    //                        ++good;
    //                    }
    //                }
    //            }
    //        }

    //    std::cout << "Bad : " << bad << " good : " << good << std::endl;


}

std::pair<navitia::type::idx_t, bool> communRAPTOR::earliest_trip(unsigned int route, unsigned int order, map_int_pint_t &best, unsigned int count){
    const unsigned int stop_area = data.pt_data.stop_points.at(data.pt_data.route_points.at(data.pt_data.routes.at(route).route_point_list.at(order)).stop_point_idx).stop_area_idx;

    if(count > 1) {
        if((best[stop_area].dt + 2*60).date > best[stop_area].dt.date)
            return std::pair<navitia::type::idx_t, bool>(earliest_trip(route, order, best[stop_area].dt + 2*60).first, true);
        else {
            return earliest_trip(route, order, best[stop_area].dt + 2*60);
        }
    } else
        return earliest_trip(route, order, best[stop_area].dt);
}


std::pair<navitia::type::idx_t, bool> communRAPTOR::earliest_trip(unsigned int route, unsigned int order, const type_retour &retour, unsigned int count){
    int temps_correspondance = 0;
    if(count > 1 && retour.stid != navitia::type::invalid_idx) {
        if(data.pt_data.route_points[data.pt_data.stop_times[retour.stid].route_point_idx].route_idx != route) {
            temps_correspondance = 2 * 60;
        }
    }
    DateTime dtTemp = retour.dt + temps_correspondance;

    if(count > 1) {
        if(dtTemp.date > retour.dt.date)
            return std::pair<navitia::type::idx_t, bool>(earliest_trip(route, order, dtTemp).first, true);
        else
            return earliest_trip(route, order, dtTemp);
    } else
        return earliest_trip(route, order, retour.dt);
}


std::pair<navitia::type::idx_t, bool>  communRAPTOR::earliest_trip(unsigned int route, unsigned int order, DateTime dt) {
    auto & vjlist = data.pt_data.route_points[data.pt_data.routes[route].route_point_list[order]].vehicle_journey_list;
    cp.order = order;
    std::vector<navitia::type::idx_t>::iterator lowerb = std::lower_bound(vjlist.begin(), vjlist.end(), dt.hour, cp);
    for(std::vector<navitia::type::idx_t>::iterator it = lowerb;  it != vjlist.end(); ++it) {

        if(data.pt_data.validity_patterns[data.pt_data.vehicle_journeys[*it].validity_pattern_idx].check(dt.date)) {
            return std::pair<navitia::type::idx_t, bool>(*it, false);
        }
    }
    ++dt.date;


    for(auto  it = vjlist.begin(); it != lowerb; ++it) {

        if(data.pt_data.validity_patterns[data.pt_data.vehicle_journeys[*it].validity_pattern_idx].check(dt.date)) {
            return std::pair<navitia::type::idx_t, bool>(*it, true);
        }
    }
    return std::pair<navitia::type::idx_t, bool>(navitia::type::invalid_idx, false);
}

//std::pair<int, bool>  communRAPTOR::earliest_trip(unsigned int route, unsigned int order, DateTime dt, unsigned int order_vj) {
//    Route_t & route_ = routes[route];
//    navitia::type::idx_t next_day = navitia::type::invalid_idx;
//    const int upper = (order_vj == navitia::type::invalid_idx) ? route_.nbTrips : (order_vj+1);


//    for(int i = 0; i < upper; ++i)  {
//        const navitia::type::StopTime & stopTime = stopTimes[get_stop_time_idx(route, i, order)];
//        navitia::type::ValidityPattern & vp = data.pt_data.validity_patterns[data.pt_data.vehicle_journeys[stopTime.vehicle_journey_idx].validity_pattern_idx];
//        if(next_day == navitia::type::invalid_idx) {
//            if(vp.check(dt.date +1)) {
//                next_day = i;
//            }
//        }
//        if(stopTime.departure_time%86400 >= dt.hour && vp.check(dt.date)) {
//            return std::pair<navitia::type::idx_t, bool>(i, false);
//        }
//    }

//    return std::pair<navitia::type::idx_t, bool>(next_day, true);
//}


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

//int communRAPTOR::get_temps_depart(navitia::type::idx_t t, int order) {
//    if(t == navitia::type::invalid_idx)
//        return std::numeric_limits<int>::max();
//    else
//        return data.pt_data.stop_times[data.pt_data.vehicle_journeys[t].stop_time_list[order]].departure_time;
//}






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


queue_t communRAPTOR::make_queue2(std::vector<unsigned int> stops) {
    queue_t retour;

    BOOST_FOREACH(unsigned int said, stops) {
        BOOST_FOREACH(unsigned int spid, data.pt_data.stop_areas[said].stop_point_list) {
            BOOST_FOREACH(unsigned int rp, data.pt_data.stop_points[spid].route_point_list) {
                if(retour.find(data.pt_data.route_points[rp].route_idx) == retour.end() ||
                        data.pt_data.route_points[rp].order < retour[data.pt_data.route_points[rp].route_idx])
                    retour[data.pt_data.route_points[rp].route_idx] = data.pt_data.route_points[rp].order;
            }
        }
    }

    return retour;
}



Path monoRAPTOR::compute_raptor(map_int_pint_t departs, map_int_pint_t destinations) {
    map_retour_t retour;
    map_int_pint_t best;
    //    best.reserve(sizeof(type_retour) * data.pt_data.stop_areas.size());
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

    //    BOOST_FOREACH(auto item1, retour) {
    //        std::cout << "Count : " << item1.first << std::endl;

    //        BOOST_FOREACH(auto item2, item1.second) {
    //            std::cout << "Stop area : " << item2.first << " : "  << item2.second.dt.date << " " << item2.second.dt.hour << " " << item2.second.stid << std::endl;
    //        }
    //    }


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

void RAPTOR::marcheapied(std::vector<unsigned int> & marked_stop, map_retour_t &retour, map_int_pint_t &best, best_dest &b_dest, unsigned int count) {
    std::vector<unsigned int> marked_stop_copy;
    marked_stop_copy = marked_stop;
    BOOST_FOREACH(auto stop_area, marked_stop_copy) {
        BOOST_FOREACH(auto stop_p, data.pt_data.stop_areas.at(stop_area).stop_point_list) {
            BOOST_FOREACH(auto connection_idx, foot_path[stop_p]) {
                const unsigned int saiddest = data.pt_data.stop_points.at(data.pt_data.connections[connection_idx].destination_stop_point_idx).stop_area_idx;
                const type_retour & retour_temp = retour[count][stop_area];
                const DateTime dtTemp = retour_temp.dt + data.pt_data.connections[connection_idx].duration;
                if(best[saiddest].dt > dtTemp) {
                    const type_retour nRetour = type_retour(navitia::type::invalid_idx, stop_area, dtTemp, connection);
                    best[saiddest] = nRetour;
                    retour[count][saiddest] = nRetour;
                    b_dest.ajouter_best(saiddest, nRetour);
                    marked_stop.push_back(saiddest);
                }
            }
        }
    }
}


void RAPTOR::boucleRAPTOR(std::vector<unsigned int> &marked_stop, map_retour_t &retour, map_int_pint_t &best, best_dest &b_dest, unsigned int & count) {
    queue_t Q;


    unsigned int said, route, t;
    marcheapied(marked_stop, retour, best, b_dest, 0);

    int nbboucle = 0;
    while(Q.size() > 0 || count == 1) {
        Q = make_queue2(marked_stop);
        marked_stop.clear();

        BOOST_FOREACH(queue_t::value_type vq, Q) {
            route = vq.first;
            t = -1;
            DateTime workingDt;
            int embarquement = -1;
            navitia::type::VehicleJourney &vj = data.pt_data.vehicle_journeys.front();

            for(unsigned int i = vq.second; i < data.pt_data.routes[route].route_point_list.size(); ++i) {
                ++nbboucle;
                said = get_sa_rp(i, route);
                if(t != navitia::type::invalid_idx) {
                    const navitia::type::StopTime & st = data.pt_data.stop_times[vj.stop_time_list[i]];
                    workingDt.update(st.arrival_time%86400);
                    //On stocke, et on marque pour explorer par la suite
                    if(workingDt < std::min(best[said].dt, b_dest.best_now.dt)) {
                        const type_retour retour_temp = type_retour(st.idx, embarquement, workingDt);
                        retour[count][said] = retour_temp;
                        best[said]          = retour_temp;
                        b_dest.ajouter_best(said, retour_temp);
                        marked_stop.push_back(said);
                    }
                }

                //Si on peut arriver plus tôt à l'arrêt en passant par une autre rout
                const type_retour & retour_temp = retour[count-1][said];
                if((retour_temp.dt.date != std::numeric_limits<int>::max()) &&
                        ((retour_temp.dt.date < workingDt.date)  ||
                         ((retour_temp.dt.date == workingDt.date) && (retour_temp.dt.hour <= get_temps_depart(t,i))))) {
                    navitia::type::idx_t et_temp;
                    bool pam;
                    std::tie(et_temp, pam) = earliest_trip(route, i, retour_temp, count);
                    if(et_temp != navitia::type::invalid_idx) {
                        t = et_temp;
                        vj = data.pt_data.vehicle_journeys[t];
                        workingDt = retour_temp.dt;
                        embarquement = said;
                        if(pam) {
                            ++workingDt.date;
                            workingDt.hour = data.pt_data.stop_times[vj.stop_time_list[i]].departure_time % 86400;
                        }
                    }
                }
            }
        }
        marcheapied(marked_stop, retour, best, b_dest, count);

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
