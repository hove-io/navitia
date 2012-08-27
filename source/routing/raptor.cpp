#include "raptor.h"
namespace navitia { namespace routing { namespace raptor {

communRAPTOR::communRAPTOR(navitia::type::Data &data) : data(data), cp(data), sp_routeorder(data.pt_data.stop_points.size())
{
    //Construction de la liste des marche à pied à partir des connections renseignées
    Timer t;
    foot_path.resize(data.pt_data.stop_points.size());
    BOOST_FOREACH(navitia::type::Connection connection, data.pt_data.connections) {
        foot_path[connection.departure_stop_point_idx].push_back(Connection_t(connection.departure_stop_point_idx, connection.destination_stop_point_idx, connection.idx, connection.duration));
    }

    //On rajoute des connexions entre les stops points d'un même stop area si elles n'existent pas
    BOOST_FOREACH(navitia::type::StopPoint sp, data.pt_data.stop_points) {

        navitia::type::StopArea sa = data.pt_data.stop_areas.at(sp.stop_area_idx);
        BOOST_FOREACH(navitia::type::idx_t spidx, sa.stop_point_list) {

            if(sp.idx != spidx) {
                bool find = false;
                BOOST_FOREACH(Connection_t connection, foot_path[sp.idx]) {
                    if(connection.destination_sp == spidx) {
                        find = true;
                        break;
                    }
                }
                if(!find) {
                    foot_path[sp.idx].push_back(Connection_t(sp.idx, spidx, 2*60));
                }

            }
        }

    }


    std::cout << "Chargement des foot paths : " << t.ms() << std::endl;
    t.reset();


    typedef std::unordered_map<navitia::type::idx_t, vector_idx> idx_vector_idx;
    BOOST_FOREACH(navitia::type::Route route, data.pt_data.routes) {

        if(route.route_point_list.size() > 0) {
            idx_vector_idx vp_vj;
            //Je regroupe entre elles les routes ayant le même validity pattern
            BOOST_FOREACH(navitia::type::idx_t vjidx, route.vehicle_journey_list) {
                if(vp_vj.find(data.pt_data.vehicle_journeys[vjidx].validity_pattern_idx) == vp_vj.end())
                    vp_vj[data.pt_data.vehicle_journeys[vjidx].validity_pattern_idx] = vector_idx();
                vp_vj[data.pt_data.vehicle_journeys[vjidx].validity_pattern_idx].push_back(vjidx);
            }

            BOOST_FOREACH(idx_vector_idx::value_type vec, vp_vj) {
                Route_t r;
                r.idx = route.idx;
                r.nbStops = route.route_point_list.size();
                r.nbTrips = vec.second.size();
                r.vp = data.pt_data.validity_patterns[vec.first];
                r.firstStopTime = stopTimes.size();
                BOOST_FOREACH(auto vjidx, vec.second) {

                    BOOST_FOREACH(navitia::type::idx_t stidx, data.pt_data.vehicle_journeys[vjidx].stop_time_list) {
                        StopTime_t st =  data.pt_data.stop_times[stidx];
                        stopTimes.push_back(st);
                    }
                }
                routes.push_back(r);
            }
        }
    }


    std::map<unsigned int, vector_idx> idxr_routei;
    for(unsigned int i=0; i<routes.size(); ++i) {
        Route_t & route = routes[i];
        if(idxr_routei.find(route.idx) == idxr_routei.end())
            idxr_routei[route.idx] = vector_idx();
        idxr_routei[route.idx].push_back(i);
    }
    std::cout << "Chargement des routes : " << t.ms() << std::endl;;

    t.reset();
    for(unsigned int i = 0; i < data.pt_data.stop_points.size(); ++i) {
        std::map<navitia::type::idx_t, int> route_order;
        BOOST_FOREACH(navitia::type::idx_t rpidx, data.pt_data.stop_points[i].route_point_list) {
            navitia::type::RoutePoint & rp = data.pt_data.route_points[rpidx];

            BOOST_FOREACH(navitia::type::idx_t ridx, idxr_routei[rp.route_idx]) {
                if(route_order.find(ridx) == route_order.end())
                    route_order[ridx] = rp.order;
                else if(route_order[ridx] > rp.order)
                    route_order[ridx] = rp.order;
            }

        }
        sp_routeorder[i] = vector_pairint();

        BOOST_FOREACH(auto pair, route_order) {
            sp_routeorder[i].push_back(pair);
        }
    }
    std::cout << "Chargement des sa route" << t.ms() << std::endl;;

    std::cout << "Nb data stop times : " << data.pt_data.stop_times.size() << " stopTimes : " << stopTimes.size() << std::endl;

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

//std::pair<int, bool> communRAPTOR::earliest_trip(unsigned int route, unsigned int order, map_int_pint_t &best, unsigned int count, int orderVj){
//    const unsigned int stop_area = data.pt_data.stop_points.at(data.pt_data.route_points.at(data.pt_data.routes.at(route).route_point_list.at(order)).stop_point_idx).stop_area_idx;

//    if(count > 1) {
//        if((best[stop_area].dt + 2*60).date > best[stop_area].dt.date)
//            return std::make_pair(earliest_trip(route, order, best[stop_area].dt + 2*60, orderVj).first, true);
//        else {
//            return earliest_trip(route, order, best[stop_area].dt + 2*60, orderVj);
//        }
//    } else
//        return earliest_trip(route, order, best[stop_area].dt, orderVj);
//}


//std::pair<int, bool> communRAPTOR::earliest_trip(unsigned int route, unsigned int order, const type_retour &retour, unsigned int count, int orderVj){
//    int temps_correspondance = 0;
//    if(count > 1 &&  orderVj > 0 && retour.stid != navitia::type::invalid_idx) {
//        //        if(data.pt_data.route_points[data.pt_data.stop_times[retour.stid].route_point_idx].route_idx != route) {
//        temps_correspondance = 2 * 60;
//        //        }
//    }
//    DateTime dtTemp = retour.dt + temps_correspondance;

//    if(dtTemp.date > retour.dt.date)
//        return std::make_pair(earliest_trip(route, order, dtTemp, orderVj).first, true);
//    else
//        return earliest_trip(route, order, dtTemp, orderVj);

//}

std::pair<int, bool>  communRAPTOR::earliest_trip(const Route_t & route, unsigned int order, DateTime dt, int orderVj) {

    int it, step,
            upper = (orderVj<0)? route.nbTrips:orderVj+1, count = upper,
            first = 0;

    //Recherche dichotomique du premier trip partant après dt.hour
    while (count>0) {
        it = first; step=count/2; it+=step;
        if (stopTimes[route.firstStopTime + order + it*route.nbStops].departure_time%86400 < dt.hour) {
            first=++it; count-=step+1;
        }
        else count=step;
    }

    //On recherche après le premier trip compatible avec la date dt.date
    for(int i = first; i < upper; ++i)  {
        if(route.vp.check(dt.date)) {
            return std::pair<int, bool>(i, false);
        }
    }

    //Si on en a pas trouvé, on cherche le lendemain
    ++ dt.date;
    for(int i = 0; i < route.nbTrips; ++i)  {
        if(route.vp.check(dt.date)) {
            return std::pair<int, bool>(i, true);
        }
    }

    //Cette route ne comporte aucun trip compatible
    return std::pair<int, bool>(-1, true);
}


std::pair<unsigned int, bool> communRAPTOR::tardiest_trip(unsigned int route, unsigned int stop_area, map_int_pint_t &best, unsigned int count){

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

    //    bool pam = false;
    //    if(dt.hour > 86400) {
    //        dt.normalize();
    //        pam = true;    BOOST_FOREACH(navitia::type::Route route, data.pt_data.routes) {
    //            if(route.line_idx == 9807) {
    //                BOOST_FOREACH(unsigned int rpidx, route.route_point_list) {
    //                    //                if(data.pt_data.stop_points.at(data.pt_data.route_points.at(rpidx).stop_point_idx).stop_area_idx == 6344){
    //                    BOOST_FOREACH(unsigned int vjidx, data.pt_data.route_points.at(rpidx).vehicle_journey_list) {
    //                        std::cout <<data.pt_data.stop_points.at(data.pt_data.route_points.at(rpidx).stop_point_idx).stop_area_idx
    //                                 << " " << data.pt_data.stop_times.at(data.pt_data.vehicle_journeys.at(vjidx).stop_time_list.at(data.pt_data.route_points.at(rpidx).order)).departure_time
    //                                 << " " << data.pt_data.stop_times.at(data.pt_data.vehicle_journeys.at(vjidx).stop_time_list.at(data.pt_data.route_points.at(rpidx).order)).arrival_time  << std::endl;
    //                    }
    //                    //                }
    //                }
    //            }
    //        }

    //    }

    //    int rp_id = get_rp_id(route, stop_area);
    //    if(rp_id == -1)
    //        return std::pair<unsigned int, bool>(-1, false);

    //    auto itdebug = data.pt_data.route_points[rp_id].vehicle_journey_list_arrival.rend();
    //    for(auto it = std::lower_bound(data.pt_data.route_points[rp_id].vehicle_journey_list_arrival.rbegin(),
    //                                   data.pt_data.route_points[rp_id].vehicle_journey_list_arrival.rend(),
    //                                   dt.hour, compare_rp_reverse(data.pt_data.route_points.at(rp_id).order, data));
    //        it != data.pt_data.route_points[rp_id].vehicle_journey_list_arrival.rend(); ++it) {
    //        if(itdebug == data.pt_data.route_points[rp_id].vehicle_journey_list_arrival.rend())
    //            itdebug = it;
    //        navitia::type::ValidityPattern vp = data.pt_data.validity_patterns[data.pt_data.vehicle_journeys[*it].validity_pattern_idx];
    //        if(vp.check(dt.date)) {/*
    //            if(data.pt_data.stop_times.at(data.pt_data.vehicle_journeys[*it].stop_time_list.at(data.pt_data.route_points.at(rp_id).order)).arrival_time %86400 <= dt.hour)*/
    //            return std::pair<unsigned int, bool>(*it, pam);
    //            //            else {
    //            //                std::cout << "Route point : " << rp_id << " " << data.pt_data.stop_times.at(data.pt_data.vehicle_journeys[*itdebug].stop_time_list.at(data.pt_data.route_points.at(rp_id).order)).arrival_time
    //            //                          << " " << *itdebug << " " << data.pt_data.stop_times.at(data.pt_data.vehicle_journeys[*it].stop_time_list.at(data.pt_data.route_points.at(rp_id).order)).arrival_time << " " << *it
    //            //                          << " " << dt.hour <<  std::endl;
    //            //                exit(1);
    //            //            }

    //        }
    //    }
    //    --dt.date;

    //    if(dt.date < 0)
    //        return std::pair<unsigned int, bool>(-1, false);

    //    for(auto  it = std::lower_bound(data.pt_data.route_points[rp_id].vehicle_journey_list_arrival.rbegin(),
    //                                    data.pt_data.route_points[rp_id].vehicle_journey_list_arrival.rend(),
    //                                    86400, compare_rp_reverse(data.pt_data.route_points[rp_id].order, data));
    //        it != data.pt_data.route_points[rp_id].vehicle_journey_list_arrival.rend(); ++it) {
    //        navitia::type::ValidityPattern vp = data.pt_data.validity_patterns[data.pt_data.vehicle_journeys[*it].validity_pattern_idx];
    //        if(vp.check(dt.date)) {
    //            return std::pair<unsigned int, bool>(*it, true);
    //        }
    //    }
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


queue_t communRAPTOR::make_queue2(std::vector<unsigned int> &stops) {
    queue_t retour;

    //    BOOST_FOREACH(unsigned int said, stops) {
    //        BOOST_FOREACH(auto pair, sa_routeorder[said]) {
    //            if(retour.find(pair.first) == retour.end())
    //                retour[pair.first] = pair.second;
    //            else if(retour[pair.first] > pair.second)
    //                retour[pair.first] = pair.second;
    //        }
    //    }

    return retour;
}

void RAPTOR::make_queue3(boost::dynamic_bitset<> &stops, boost::dynamic_bitset<> & routesValides, queue_t &Q) {
    Q.assign(routes.size(), std::numeric_limits<int>::max());

    for(auto spid = stops.find_first(); spid != stops.npos; spid = stops.find_next(spid)) {
        BOOST_FOREACH(auto pair, sp_routeorder[spid]) {
            if(routesValides.test(pair.first)) {
                if(Q[pair.first] > pair.second)
                    Q[pair.first] = pair.second;
            }
        }

        stops.set(spid, false);
    }
}


Path monoRAPTOR::compute_raptor(vector_idxretour departs, vector_idxretour destinations) {
    map_retour_t retour;
    map_int_pint_t best;
    best.resize(data.pt_data.stop_points.size());
    //    best.reserve(sizeof(type_retour) * data.pt_data.stop_areas.size());
    std::vector<unsigned int> marked_stop;


    retour.push_back(map_int_pint_t());
    retour.back().resize(data.pt_data.stop_points.size());
    BOOST_FOREACH(auto item, departs) {
        retour.back()[item.first] = item.second;
        best[item.first] = item.second;
        marked_stop.push_back(item.first);
    }

    best_dest b_dest;

    BOOST_FOREACH(auto item, destinations) {
        b_dest.ajouter_destination(item.first, item.second);
    }

    unsigned int count = 1;


    boucleRAPTOR(marked_stop, retour, best, b_dest, count);
    //    std::cout << "Fin boucle : " << std::flush << data.pt_data.vehicle_journeys[data.pt_data.stop_times[retour[1][17389].stid].vehicle_journey_idx].route_idx << std::endl;

    //    BOOST_FOREACH(auto item1, retour) {
    //        std::cout << "Count : " << item1.first << std::endl;

    //        BOOST_FOREACH(auto item2, item1.second) {
    //            std::cout << "Stop area : " << item2.first << " : "  << item2.second.dt.date << " " << item2.second.dt.hour << " " << item2.second.stid << std::endl;
    //        }
    //    }


    if(b_dest.best_now != type_retour()) {
        unsigned int destination_idx = b_dest.best_now_spid;
        return makeBestPath(retour, best, departs, destination_idx, count);
    }
    Path result;
    return result;
}


std::vector<Path> monoRAPTOR::compute_all(vector_idxretour departs, vector_idxretour destinations) {
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

void RAPTOR::marcheapied(/*std::vector<unsigned int>*/boost::dynamic_bitset<> & marked_stop, map_retour_t &retour, map_int_pint_t &best, best_dest &b_dest, unsigned int count) {

    for(auto stop_point= marked_stop.find_first(); stop_point != marked_stop.npos; stop_point = marked_stop.find_next(stop_point)) {
        BOOST_FOREACH(auto conn, foot_path[stop_point]) {
            const type_retour & retour_temp = retour[count][stop_point];
            const DateTime dtTemp = retour_temp.dt + conn.duration;
            if(best[conn.destination_sp].dt > dtTemp) {
                const type_retour nRetour = type_retour(navitia::type::invalid_idx, stop_point, dtTemp, connection);
                best[conn.destination_sp] = nRetour;
                retour[count][conn.destination_sp] = nRetour;
                b_dest.ajouter_best(conn.destination_sp, nRetour);
                marked_stop.set(conn.destination_sp);
            }
        }
    }
}

void RAPTOR::setRoutesValides(boost::dynamic_bitset<> &routesValides, std::vector<unsigned int> &marked_stop, map_retour_t &retour) {

    //On cherche la premiere date
    int date = std::numeric_limits<int>::max();
    BOOST_FOREACH(unsigned int said, marked_stop) {
        if(retour[0][said].dt.date < date)
            date = retour[0][said].dt.date;
    }

    //On active les routes
    int i = 0;
    BOOST_FOREACH(Route_t route, routes) {
        routesValides.set(i, (route.vp.check(date) || route.vp.check(date+1)));
        ++i;
    }

}



void RAPTOR::boucleRAPTOR(std::vector<unsigned int> &marked_stop, map_retour_t &retour, map_int_pint_t &best, best_dest &b_dest, unsigned int & count) {
    queue_t Q;


    unsigned int said, route;
    int t;

    boost::dynamic_bitset<> routesValides(routes.size());

    boost::dynamic_bitset<> marked_sp(data.pt_data.stop_points.size());
    BOOST_FOREACH(auto said, marked_stop) {
        marked_sp.set(said);
    }

    setRoutesValides(routesValides, marked_stop, retour);

    marcheapied(marked_sp, retour, best, b_dest, 0);


    int nbboucle = 0;
    //    Q.resize(routes.size());

    while(marked_sp.any()/* && count < 7*/) {
        retour.push_back(map_int_pint_t());
        retour.back().resize(data.pt_data.stop_points.size());
        make_queue3(marked_sp, routesValides, Q);
        route = 0;
        BOOST_FOREACH(queue_t::value_type vq, Q) {
            t = -1;
            DateTime workingDt;
            int embarquement = -1;
            const Route_t & route_ = routes[route] ;
            for(int i = vq; i < route_.nbStops; ++i) {
                ++nbboucle;
                int spid = data.pt_data.route_points[data.pt_data.routes[route_.idx].route_point_list[i]].stop_point_idx;
                if(t >= 0) {
                    const StopTime_t & st = stopTimes[get_stop_time_idx(route_, t, i)];
                    workingDt.update(st.arrival_time%86400);
                    //On stocke, et on marque pour explorer par la suite
                    if(workingDt < std::min(best[spid].dt, b_dest.best_now.dt)) {
                        const type_retour retour_temp = type_retour(st.idx, embarquement, workingDt);
                        retour.back()[spid] = retour_temp;
                        best[spid]          = retour_temp;
                        b_dest.ajouter_best(spid, retour_temp);
                        marked_sp.set(spid);
                    }
                }

                //Si on peut arriver plus tôt à l'arrêt en passant par une autre rout
                const type_retour & retour_temp = retour[count-1][spid];

                if((retour_temp.type != uninitialized) &&
                        ((retour_temp.dt.date < workingDt.date)  ||
                         ((retour_temp.dt.date == workingDt.date) && (retour_temp.dt.hour <= get_temps_depart(route_, t, i))))) {
                    int et_temp;
                    bool pam;
                    std::tie(et_temp, pam) = earliest_trip(route_, i, retour_temp.dt, t);
                    if(et_temp >= 0) {
                        t = et_temp;
                        workingDt = retour_temp.dt;
                        embarquement = spid;
                        if(pam) {
                            ++workingDt.date;
                            workingDt.hour = get_temps_depart(route_, t,i);
                        }
                    }
                }
            }

            route ++;
        }
        marcheapied(marked_sp, retour, best, b_dest, count);

        ++count;
    }
}


void reverseRAPTOR::boucleRAPTOR(std::vector<unsigned int> &marked_stop, map_retour_t &retour, map_int_pint_t &best, best_dest &b_dest, unsigned int & count) {
    //    map_int_int_t Q;

    //    int et_temp ;

    //    int t, stid, said;
    //    unsigned int route, p;
    //    bool pam;
    //    b_dest.reverse();


    //    //    std::cout << "retour 0 1 " << retour.at(0).at(1).dt << std::endl;
    //    while(((Q.size() > 0) /*& (count < 10)*/) || count == 1) {

    //        Q = make_queue(marked_stop);
    //        marked_stop.clear();


    //        BOOST_FOREACH(map_int_int_t::value_type vq, Q) {
    //            route = vq.first;
    //            p = vq.second;
    //            t = -1;
    //            int temps_depart = retour[count - 1][p].dt.hour;
    //            int working_date = retour[count - 1][p].dt.date;
    //            int prev_temps = temps_depart;
    //            stid = -1;
    //            int embarquement = p;
    //            for(int i = get_rp_order(route, p); i >= 0; --i) {
    //                said = get_sa_rp(i, route);


    //                if(t != -1) {

    //                    stid = data.pt_data.vehicle_journeys[t].stop_time_list[i];
    //                    temps_depart = data.pt_data.stop_times[stid].departure_time%86400;


    //                    //Passe minuit
    //                    if(prev_temps < (data.pt_data.stop_times[stid].arrival_time%86400))
    //                        --working_date;
    //                    //On stocke, et on marque pour explorer par la suite
    //                    type_retour max_said;
    //                    if(best.count(said) > 0)
    //                        max_said = std::max(best[said], b_dest.best_now);
    //                    else
    //                        max_said = b_dest.best_now;
    //                    if(type_retour(-1, DateTime(working_date,data.pt_data.stop_times[stid].arrival_time%86400)) > max_said) {
    //                        retour[count][said]  = type_retour(stid, embarquement, DateTime(working_date, data.pt_data.stop_times[stid].departure_time%86400));
    //                        best[said] = type_retour(stid, DateTime(working_date, data.pt_data.stop_times[stid].departure_time%86400));
    //                        b_dest.ajouter_best_reverse(said, type_retour(stid, DateTime(working_date, data.pt_data.stop_times[stid].departure_time%86400)));
    //                        if(std::find(marked_stop.begin(), marked_stop.end(), said) == marked_stop.end()) {
    //                            marked_stop.push_back(said);
    //                        }
    //                    }
    //                }

    //                //Si on peut arriver plus tôt à l'arrêt en passant par une autre route
    //                if(retour[count-1].count(said) > 0)  {
    //                    if(retour[count-1][said].dt >= DateTime(working_date, temps_depart)){
    //                        std::tie(et_temp, pam) = tardiest_trip(route, said, retour, count);
    //                        if(et_temp >=0) {

    //                            t = et_temp;
    //                            working_date = retour[count -1][said].dt.date;
    //                            stid = data.pt_data.vehicle_journeys[t].stop_time_list[i];
    //                            embarquement = said;
    //                            if(pam /*|| data.pt_data.stop_times.at(stid).arrival_time > 86400*/)
    //                                --working_date;
    //                        }
    //                    }
    //                }
    //                if(stid != -1)
    //                    prev_temps = data.pt_data.stop_times[stid].departure_time % 86400;
    //            }
    //        }


    //        std::vector<unsigned int> marked_stop_copy;
    //        marked_stop_copy = marked_stop;
    //        BOOST_FOREACH(auto stop_area, marked_stop_copy) {

    //            BOOST_FOREACH(auto stop_p, data.pt_data.stop_areas.at(stop_area).stop_point_list) {
    //                auto it_fp = foot_path.find(stop_p);
    //                if(it_fp != foot_path.end()) {
    //                    BOOST_FOREACH(auto connection_idx, (*it_fp).second) {

    //                        unsigned int saiddest = data.pt_data.stop_points.at(data.pt_data.connections[connection_idx].destination_stop_point_idx).stop_area_idx;

    //                        if(best.count(saiddest) > 0)
    //                            best[saiddest] = std::max(best[saiddest], type_retour(navitia::type::invalid_idx, stop_area, DateTime(retour[count][stop_area].dt - data.pt_data.connections[connection_idx].duration), connection));
    //                        else
    //                            best[saiddest] = type_retour(navitia::type::invalid_idx, stop_area, DateTime(retour[count][stop_area].dt - data.pt_data.connections[connection_idx].duration), connection);
    //                        if(retour[count].count(saiddest) > 0)
    //                            retour[count][saiddest] = std::max(retour[count][saiddest], type_retour(navitia::type::invalid_idx, stop_area, DateTime(retour[count][stop_area].dt - data.pt_data.connections[connection_idx].duration), connection));
    //                        else
    //                            retour[count][saiddest] = type_retour(navitia::type::invalid_idx, stop_area, DateTime(retour[count][stop_area].dt - data.pt_data.connections[connection_idx].duration), connection);
    //                        b_dest.ajouter_best(saiddest, type_retour(navitia::type::invalid_idx, stop_area, DateTime(retour[count][stop_area].dt - data.pt_data.connections[connection_idx].duration), connection));
    //                        marked_stop.push_back(saiddest);
    //                    }
    //                }
    //            }
    //        }



    //        ++count;
    //    }

}


Path monoRAPTOR::makeBestPath(map_retour_t &retour, map_int_pint_t &best, vector_idxretour departs, unsigned int destination_idx, unsigned int count) {
    unsigned int countb = 1;
    for(;countb<=count;++countb) {
        if(!(retour[countb][destination_idx].dt == DateTime())) {
            if((retour[countb][destination_idx].stid == best[destination_idx].stid) && (retour[countb][destination_idx].dt.hour == best[destination_idx].dt.hour) && (retour[countb][destination_idx].dt.date == best[destination_idx].dt.date)) {
                break;
            }
        }
    }

    return makePath(retour, best, departs, destination_idx, countb);
}

std::vector<Path> monoRAPTOR::makePathes(map_retour_t &retour, map_int_pint_t &best, vector_idxretour departs, best_dest &b_dest, unsigned int count) {
    std::vector<Path> result;

    for(unsigned int i=1;i<count;++i) {
        BOOST_FOREACH(auto dest, b_dest.map_date_time) {
            if(retour[i][dest.first] != type_retour()) {
                result.push_back(makePath(retour, best, departs, dest.first, i));
            }


        }
    }

    return result;
}

Path RAPTOR::makePath(map_retour_t &retour, map_int_pint_t &best, vector_idxretour departs, unsigned int destination_idx, unsigned int countb) {
    Path result;
    unsigned int current_spid = destination_idx;
    type_retour r = retour[countb][current_spid];
    DateTime workingDate = r.dt;
    navitia::type::StopTime current_st, prec_st;
    int spid_embarquement = -1;

    bool stop = false;
    BOOST_FOREACH(auto item, departs) {
        stop = stop || (item.first == (int)current_spid);
    }

    result.nb_changes = countb - 1;
    while(!stop) {
        bool debut = false, footpath = false;

        if(retour[countb][current_spid].type == vj) {
            if(spid_embarquement == -1) {
                r = retour[countb][current_spid];
                spid_embarquement = r.said_emarquement;
                current_st = data.pt_data.stop_times.at(r.stid);
                workingDate = r.dt;
                workingDate.normalize();
                debut = true;
            }
        } else {
            if(spid_embarquement == -1) {
                r = retour[countb][current_spid];
                workingDate = r.dt;
                workingDate.normalize();
                result.items.push_back(PathItem(current_spid, workingDate, workingDate));
                current_spid = r.said_emarquement;
                spid_embarquement = -1;
                footpath = true;
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
            current_spid = /*data.pt_data.stop_points.at(*/data.pt_data.route_points.at(current_st.route_point_idx).stop_point_idx/*).stop_area_idx*/;
            if(spid_embarquement == (int)current_spid) {
                --countb;
                spid_embarquement = -1;
            }
            result.items.push_back(PathItem(current_spid, DateTime(workingDate.date, current_st.arrival_time), DateTime(workingDate.date, current_st.departure_time),
                                            data.pt_data.routes.at(data.pt_data.route_points.at(current_st.route_point_idx).route_idx).line_idx, data.pt_data.route_points.at(current_st.route_point_idx).route_idx, current_st.vehicle_journey_idx));

        }
        BOOST_FOREACH(auto item, departs) {
            stop = stop || (item.first == (int)current_spid);
        }
    }
    std::reverse(result.items.begin(), result.items.end());


    result.duration = result.items.back().arrival - result.items.front().departure;

    int count_visites = 0;
    BOOST_FOREACH(auto t, best) {
        if(t.type != uninitialized) {
            ++count_visites;
        }
    }
    result.percent_visited = 100*count_visites / data.pt_data.stop_areas.size();

    return result;
}


Path reverseRAPTOR::makePath(map_retour_t &retour, map_int_pint_t &best, vector_idxretour departs, unsigned int destination_idx, unsigned int countb) {
    Path result;
    //    unsigned int current_said = destination_idx;

    //    type_retour r = retour[countb][current_said];
    //    DateTime workingDate = r.dt;
    //    navitia::type::StopTime current_st, prec_st;
    //    int said_embarquement = -1;

    //    bool stop = false;
    //    BOOST_FOREACH(auto item, departs) {
    //        stop = stop || (item.first == (int)current_said);
    //    }

    //    while(!stop) {
    //        bool debut = false, footpath = false;

    //        if(retour.count(countb) > 0) {
    //            if(retour[countb].count(current_said) > 0) {
    //                if(retour[countb][current_said].type == vj) {
    //                    if(said_embarquement == -1) {
    //                        r = retour[countb][current_said];
    //                        said_embarquement = r.said_emarquement;
    //                        current_st = data.pt_data.stop_times.at(r.stid);
    //                        workingDate = r.dt;
    //                        workingDate.hour = current_st.departure_time;
    //                        workingDate.normalize();
    //                        debut = true;
    //                    }
    //                } else {
    //                    if(said_embarquement == -1) {
    //                        r = retour[countb][current_said];
    //                        workingDate = r.dt;
    //                        workingDate.normalize();
    //                        result.items.push_back(PathItem(current_said, workingDate, workingDate));
    //                        current_said = r.said_emarquement;
    //                        said_embarquement = -1;
    //                        footpath = true;
    //                    }
    //                }
    //            }
    //        }
    //        if(!footpath) {
    //            if(!debut) {
    //                prec_st = current_st;
    //                current_st = data.pt_data.stop_times.at(data.pt_data.vehicle_journeys.at(current_st.vehicle_journey_idx).stop_time_list.at(current_st.order+1));
    //                if(current_st.arrival_time%86400 < prec_st.arrival_time%86400 && prec_st.vehicle_journey_idx!=navitia::type::invalid_idx)
    //                    --workingDate.date;
    //                workingDate.hour = current_st.arrival_time;
    //                workingDate.normalize();
    //            }
    //            current_said = data.pt_data.stop_points.at(data.pt_data.route_points.at(current_st.route_point_idx).stop_point_idx).stop_area_idx;
    //            if(said_embarquement == (int)current_said) {
    //                --countb;
    //                ++ result.nb_changes;
    //                said_embarquement = -1;
    //                //                result.items.push_back(PathItem(current_said, DateTime(current_st.arrival_time, workingDate.date), DateTime(current_st.departure_time, workingDate.date),
    //                //                                                data.pt_data.routes.at(data.pt_data.route_points.at(current_st.route_point_idx).route_idx).line_idx));
    //            } /*else {
    //                result.items.push_back(PathItem(current_said, workingDate, workingDate,
    //                                                data.pt_data.routes.at(data.pt_data.route_points.at(current_st.route_point_idx).route_idx).line_idx));
    //            }*/
    //            result.items.push_back(PathItem(current_said, DateTime(workingDate.date, current_st.arrival_time), DateTime(workingDate.date, current_st.departure_time),
    //                                            data.pt_data.routes.at(data.pt_data.route_points.at(current_st.route_point_idx).route_idx).line_idx));
    //            //            std::cout << result << std::endl;

    //        }

    //        BOOST_FOREACH(auto item, departs) {
    //            stop = stop || (item.first == (int)current_said);
    //        }
    //    }


    //    result.duration = result.items.back().arrival - result.items.front().departure;

    //    int count_visites = 0;
    //    BOOST_FOREACH(auto t, best) {
    //        if(t.second.stid != navitia::type::invalid_idx) {
    //            ++count_visites;
    //        }
    //    }
    //    result.percent_visited = 100*count_visites / data.pt_data.stop_areas.size();

    return result;
}


Path communRAPTOR::compute(idx_t departure_idx, idx_t destination_idx, int departure_hour, int departure_day) {
    vector_idxretour departs, destinations;

    BOOST_FOREACH(navitia::type::idx_t spidx, data.pt_data.stop_areas[departure_idx].stop_point_list)
            departs.push_back(std::make_pair(spidx, type_retour(-1, DateTime(departure_day, departure_hour))));

    BOOST_FOREACH(navitia::type::idx_t spidx, data.pt_data.stop_areas[destination_idx].stop_point_list)
            destinations.push_back(std::make_pair(destination_idx, type_retour()));
    return compute_raptor(departs, destinations);
}

std::vector<Path> monoRAPTOR::compute_all(navitia::type::EntryPoint departure, navitia::type::EntryPoint destination, int departure_hour, int departure_day) {
    vector_idxretour departs, destinations;

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

    departs.push_back(std::make_pair((*it_departure).second, type_retour(-1, DateTime(departure_day, departure_hour), 0, 0)));
    destinations.push_back(std::make_pair((*it_destination).second, type_retour()));
    return compute_all(departs, destinations);
}

Path communRAPTOR::compute(const type::GeographicalCoord & departure, double radius, idx_t destination_idx, int departure_hour, int departure_day) {
    vector_idxretour bests, destinations;
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
            bests.push_back(std::make_pair(item.first, type_retour(-1, DateTime(day, temps))));
        }
    }
    destinations.push_back(std::make_pair(destination_idx, type_retour()));

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
        departs[item.first] =  type_retour(-1, DateTime(day, temps), 0, (item.second / 80));
    }


    prox.clear();
    try {
        prox = (retour) (data.street_network.find_nearest(destination, data.pt_data.stop_area_proximity_list, radius_destination));
    } catch(NotFound) {std::cout << "Not found 2 " << std::endl;return ;}
    BOOST_FOREACH(auto item, prox) {
        destinations[item.first] = type_retour((int)(item.second/80));
    }
}

void communRAPTOR::trouverGeo(const type::GeographicalCoord & departure, double radius_depart, const type::GeographicalCoord & destination, double radius_destination,
                              int departure_hour, int departure_day, vector_idxretour &departs, vector_idxretour &destinations) {
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
        departs.push_back(std::make_pair(item.first, type_retour(-1, DateTime(day, temps), 0, (item.second / 80))));
    }


    prox.clear();
    try {
        prox = (retour) (data.street_network.find_nearest(destination, data.pt_data.stop_area_proximity_list, radius_destination));
    } catch(NotFound) {std::cout << "Not found 2 " << std::endl;return ;}
    BOOST_FOREACH(auto item, prox) {
        destinations.push_back(std::make_pair(item.first, type_retour((int)(item.second/80))));
    }
}

Path communRAPTOR::compute(const type::GeographicalCoord & departure, double radius_depart, const type::GeographicalCoord & destination, double radius_destination
                           , int departure_hour, int departure_day) {
    vector_idxretour departs, destinations;

    trouverGeo(departure, radius_depart, destination, radius_destination, departure_hour, departure_day, departs, destinations);

    std::cout << "Nb stations departs : " << departs.size() << " destinations : " << destinations.size() << std::endl;

    Path result = compute_raptor(departs, destinations);
    return result;
}

std::vector<Path> monoRAPTOR::compute_all(const type::GeographicalCoord & departure, double radius_depart, const type::GeographicalCoord & destination, double radius_destination
                                          , int departure_hour, int departure_day) {
    vector_idxretour departs, destinations;

    //    trouverGeo(departure, radius_depart, destination, radius_destination, departure_hour, departure_day, departs, destinations);

    std::cout << "Nb stations departs : " << departs.size() << " destinations : " << destinations.size() << std::endl;

    return compute_all(departs, destinations);
}


}}}
