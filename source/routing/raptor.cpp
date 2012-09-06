#include "raptor.h"
namespace navitia { namespace routing { namespace raptor {

communRAPTOR::communRAPTOR(navitia::type::Data &data) : data(data), cp(data)
{
    //Construction de la liste des marche à pied à partir des connections renseignées
    Timer t;
    std::vector<list_connections> footpath_temp;
    footpath_temp.resize(data.pt_data.route_points.size());
    BOOST_FOREACH(navitia::type::Connection connection, data.pt_data.connections) {
        for(navitia::type::idx_t rpidxdeparture : data.pt_data.stop_points[connection.departure_stop_point_idx].route_point_list) {
            for(navitia::type::idx_t rpidxdestination : data.pt_data.stop_points[connection.destination_stop_point_idx].route_point_list) {
                footpath_temp[rpidxdeparture][rpidxdestination] = Connection_t(rpidxdeparture, rpidxdestination, connection.idx, connection.duration);
                footpath_temp[rpidxdestination][rpidxdeparture] = Connection_t(rpidxdestination, rpidxdeparture, connection.idx, connection.duration);
            }
        }
    }

    //On rajoute des connexions entre les stops points d'un même stop area si elles n'existent pas
    footpath_index.resize(data.pt_data.route_points.size());
    BOOST_FOREACH(navitia::type::RoutePoint rp, data.pt_data.route_points) {
        navitia::type::StopArea sa = data.pt_data.stop_areas[data.pt_data.stop_points[rp.stop_point_idx].stop_area_idx];
        footpath_index[rp.idx].first = foot_path.size();

        int size = 0;
        for(auto conn : footpath_temp[rp.idx]) {
            if(conn.second.duration > 0) {
                foot_path.push_back(conn.second);
                ++size;
            }
        }

        BOOST_FOREACH(navitia::type::idx_t spidx, sa.stop_point_list) {
            navitia::type::StopPoint sp = data.pt_data.stop_points[spidx];
            for(navitia::type::idx_t rpidx : sp.route_point_list) {
                if(rp.idx != rpidx) {
                    if(footpath_temp[rp.idx].find(rpidx) == footpath_temp[rp.idx].end()) {
                        foot_path.push_back(Connection_t(rp.idx, rpidx, 2*60));
                        ++size;
                    }
                }
            }
        }
        footpath_index[rp.idx].second = size;
    }

    std::cout << "Chargement des foot paths : " << t.ms() << std::endl;
    t.reset();


    typedef std::unordered_map<navitia::type::idx_t, vector_idx> idx_vector_idx;
    for(navitia::type::Route & route : data.pt_data.routes) {

        if(route.route_point_list.size() > 0) {
            idx_vector_idx vp_vj;
            //Je regroupe entre elles les routes ayant le même validity pattern
            BOOST_FOREACH(navitia::type::idx_t vjidx, route.vehicle_journey_list) {
                if(vp_vj.find(data.pt_data.vehicle_journeys[vjidx].validity_pattern_idx) == vp_vj.end())
                    vp_vj[data.pt_data.vehicle_journeys[vjidx].validity_pattern_idx] = vector_idx();
                vp_vj[data.pt_data.vehicle_journeys[vjidx].validity_pattern_idx].push_back(vjidx);
            }

            for(idx_vector_idx::value_type vec : vp_vj) {
                if(vec.second.size() > 0) {
                    Route_t r;
                    r.idx = route.idx;
                    r.nbStops = route.route_point_list.size();
                    r.nbTrips = vec.second.size();
                    r.vp = data.pt_data.validity_patterns[vec.first];
                    r.firstStopTime = stopTimes.size();
                    for(auto vjidx : vec.second) {
                        for(navitia::type::idx_t stidx : data.pt_data.vehicle_journeys[vjidx].stop_time_list) {
                            StopTime_t st =  data.pt_data.stop_times[stidx];
                            stopTimes.push_back(st);
                        }
                    }
                    routes.push_back(r);
                }
            }
        }
    }

    std::cout << "Chargement des routes : " << t.ms() << std::endl;;


    std::cout << "Nb data stop times : " << data.pt_data.stop_times.size() << " stopTimes : " << stopTimes.size() << std::endl;
}


int  communRAPTOR::earliest_trip(const Route_t & route, unsigned int order, DateTime dt, int orderVj) {
    int it, step,
            upper = (orderVj<0)? route.nbTrips:orderVj+1, count = upper,
            first = 0;

    if(route.vp.check(dt.date())) {
        //Recherche dichotomique du premier trip partant après dt.hour
        while (count>0) {
            it = first; step=count/2; it+=step;
            if (stopTimes[route.firstStopTime + order + it*route.nbStops].departure_time < dt.hour()) {
                first=++it; count-=step+1;
            }
            else count=step;
        }

        if(first != upper)
            return first;
    }

    //Si on en a pas trouvé, on cherche le lendemain
    dt.date_increment();
    if(route.vp.check(dt.date())) {
        return 0;
    }

    //Cette route ne comporte aucun trip compatible
    return -1;
}


int  communRAPTOR::tardiest_trip(const Route_t & route, unsigned int order, DateTime dt, int orderVj) {
    int current_trip/*, step*/,
            upper =  route.nbTrips/*, count = upper*/,
            first = (orderVj<0)? 0:orderVj+1;

    if(route.vp.check(dt.date())) {
        //Recherche dichotomique du premier trip avant après dt.hour
        current_trip = upper;
        while ((upper - first) >1) {
            current_trip = first + (upper - first) / 2;
            int current_stop_time = route.firstStopTime + order + current_trip*route.nbStops;
            if (stopTimes[current_stop_time].arrival_time > dt.hour()) {
                upper = current_trip;
            }
            else first = current_trip;
        }
        if(stopTimes[route.firstStopTime + order + first*route.nbStops].arrival_time > dt.hour())
            --first;

        if(first != upper && first >= 0)
            return first;
    }

    //Si on en a pas trouvé, on cherche le lendemain
    dt.date_decrement();
    if(route.vp.check(dt.date())) {
        return route.nbTrips - 1;
    }

    //Cette route ne comporte aucun trip compatible
    return -1;
}







void RAPTOR::make_queue(boost::dynamic_bitset<> &stops, boost::dynamic_bitset<> & routesValides, queue_t &Q) {
    Q.assign(routes.size(), std::numeric_limits<int>::max());

    for(auto routeid = routesValides.find_first(); routeid != routesValides.npos; routeid = routesValides.find_next(routeid)) {

        for(navitia::type::idx_t rpidx : data.pt_data.routes[routes[routeid].idx].route_point_list) {
            if(stops.test(rpidx)) {
                Q[routeid] = data.pt_data.route_points[rpidx].order;
                break;
            }
        }
    }
    stops.reset();
}

void RAPTOR::make_queuereverse(const boost::dynamic_bitset<> &stops, const boost::dynamic_bitset<> &routesValides, queue_t &Q) {
    Q.assign(routes.size(), std::numeric_limits<int>::min());
    for(auto routeid = routesValides.find_first(); routeid != routesValides.npos; routeid = routesValides.find_next(routeid)) {

        const auto & rplist = data.pt_data.routes[routes[routeid].idx].route_point_list;
        for(auto rpidxit = rplist.rbegin(); rpidxit != rplist.rend(); ++rpidxit) {
            if(stops.test(*rpidxit)) {
                Q[routeid] = data.pt_data.route_points[*rpidxit].order;
                break;
            }
        }
    }
}

void RAPTOR::marcheapied(/*std::vector<unsigned int>*/boost::dynamic_bitset<> & marked_stop, map_retour_t &retour, map_int_pint_t &best, best_dest &b_dest, unsigned int count) {

    auto it = foot_path.begin();
    int last = 0;
    for(auto route_point= marked_stop.find_first(); route_point != marked_stop.npos; route_point = marked_stop.find_next(route_point)) {
        const auto & index = footpath_index[route_point];
        advance(it, index.first - last);
        const auto end = it + index.second;
        for(; it != end; ++it) {
            if(route_point != (*it).destination_rp) {
                const type_retour & retour_temp = retour[count][route_point];
                const DateTime dtTemp = retour_temp.dt + (*it).duration;
                if(dtTemp < best[(*it).destination_rp].dt) {
                    const type_retour nRetour = type_retour(navitia::type::invalid_idx, route_point, dtTemp, connection);
                    best[(*it).destination_rp] = nRetour;
                    retour[count][(*it).destination_rp] = nRetour;
                    b_dest.ajouter_best((*it).destination_rp, nRetour);
                    marked_stop.set((*it).destination_rp);
                }
            }
        }
        last = index.first + index.second;
    }
}


void RAPTOR::marcheapiedreverse(boost::dynamic_bitset<> & marked_stop, map_retour_t &retour, map_int_pint_t &best, best_dest &b_dest, unsigned int count) {
    auto it = foot_path.begin();
    int last = 0;
    for(auto route_point = marked_stop.find_first(); route_point != marked_stop.npos; route_point = marked_stop.find_next(route_point)) {
        const auto & index = footpath_index[route_point];
        advance(it, index.first - last);
        const auto end = it + index.second;
        for(; it != end; ++it) {
            if(route_point != (*it).destination_rp) {
                const type_retour & retour_temp = retour[count][route_point];
                const DateTime dtTemp = retour_temp.dt - (*it).duration;
                if(dtTemp > best[(*it).destination_rp].dt) {
                    if((*it).destination_rp == 131377)
                        std::cout << " Debug marche a pied " << count << " " << dtTemp.hour() << " " << retour_temp.dt.hour() << std::endl;
                    const type_retour nRetour = type_retour(navitia::type::invalid_idx, route_point, dtTemp, connection);
                    best[(*it).destination_rp] = nRetour;
                    retour[count][(*it).destination_rp] = nRetour;
                    b_dest.ajouter_best_reverse((*it).destination_rp, nRetour);
                    marked_stop.set((*it).destination_rp);
                } /*else if(dtTemp == best[(*it).destination_rp].dt) {
                    marked_stop.set((*it).destination_rp);
                }*/
            }
        }
        last = index.first + index.second;
    }
}

Path RAPTOR::compute_raptor(vector_idxretour departs, vector_idxretour destinations) {
    map_retour_t retour;
    map_int_pint_t best;
    best.resize(data.pt_data.route_points.size());
    //    best.reserve(sizeof(type_retour) * data.pt_data.stop_areas.size());
    std::vector<unsigned int> marked_stop;


    retour.push_back(retour_constant);
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

    if(b_dest.best_now.type != uninitialized) {
        unsigned int destination_idx = b_dest.best_now_rpid;
        return makeBestPath(retour, best, departs, destination_idx, count);
    }
    Path result;
    return result;
}

Path RAPTOR::compute_raptor_reverse(vector_idxretour departs, vector_idxretour destinations) {
    map_retour_t retour;
    map_int_pint_t best;

    type_retour min;
    min.dt = DateTime::min;
    best.assign(data.pt_data.route_points.size(), min);
    std::vector<unsigned int> marked_stop;



    retour.push_back(retour_constant_reverse);
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

    b_dest.reverse();
    boucleRAPTORreverse(marked_stop, retour, best, b_dest, count);

    if(b_dest.best_now.type != uninitialized) {
        unsigned int destination_idx = b_dest.best_now_rpid;
        return makeBestPathreverse(retour, best, departs, destination_idx, count);
    }
    Path result;
    return result;
}


Path RAPTOR::compute_raptor_rabattement(vector_idxretour departs, vector_idxretour destinations) {
    map_retour_t retour;
    map_int_pint_t best;
    //    best.reserve(sizeof(type_retour) * data.pt_data.stop_areas.size());
    std::vector<unsigned int> marked_stop;


    retour.push_back(retour_constant);
    best.resize(data.pt_data.route_points.size());
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


    count = 1;
    retour.clear();

    retour.push_back(retour_constant_reverse);
    marked_stop.clear();
    b_dest.reinit();
    b_dest.reverse();
    BOOST_FOREACH(auto item, departs) {
        b_dest.ajouter_destination(item.first, item.second);
        if(best[item.first].dt != DateTime::inf) {
            b_dest.ajouter_best_reverse(item.first, best[item.first]);
        }
    }

    BOOST_FOREACH(auto item, destinations) {
        retour.back()[item.first] = best[item.first];
        marked_stop.push_back(item.first);
    }

    boucleRAPTORreverse(marked_stop, retour, best, b_dest, count);

    if(b_dest.best_now.type != uninitialized) {
        unsigned int destination_idx = b_dest.best_now_rpid;
        return makeBestPathreverse(retour, best, destinations, destination_idx, count);
    }
    return Path();
}




std::vector<Path> RAPTOR::compute_all(vector_idxretour departs, vector_idxretour destinations) {
    map_retour_t retour;
    map_int_pint_t best;
    std::vector<Path> result;
    std::vector<unsigned int> marked_stop;


    retour.push_back(retour_constant);
    best.resize(data.pt_data.route_points.size());
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


    if(b_dest.best_now.type != uninitialized) {
        auto temp = makePathes(retour, best, departs, b_dest, count);
        result.insert(result.begin(), temp.begin(), temp.end());
    }

    for(auto & it : best) {
//        if(it->dt == DateTime::inf)
            it.dt = DateTime::min;
    }

    count = 1;
    retour.clear();
    retour.push_back(retour_constant_reverse);
    marked_stop.clear();

    retour.back()[b_dest.best_now_rpid] = b_dest.best_now;
    best[b_dest.best_now_rpid] = b_dest.best_now;
    marked_stop.push_back(b_dest.best_now_rpid);


    b_dest.reinit();
    b_dest.reverse();
    type_retour t_min;
    t_min.dt = DateTime::inf;
    BOOST_FOREACH(auto item, departs) {
        b_dest.ajouter_destination(item.first, t_min);
//        b_dest.ajouter_best_reverse(item.first, item.second);
    }

//    BOOST_FOREACH(auto item, destinations) {

//        retour.back()[item.first] = best[item.first];
//        marked_stop.push_back(item.first);
//    }


    boucleRAPTORreverse(marked_stop, retour, best, b_dest, count);



    if(b_dest.best_now.type != uninitialized){
        auto temp = makePathesreverse(retour, best, destinations, b_dest, count);
        result.insert(result.end(), temp.begin(), temp.end());
        return result;
    }

    return result;
}



void RAPTOR::setRoutesValides(boost::dynamic_bitset<> &routesValides, std::vector<unsigned int> &marked_stop, map_retour_t &retour) {

    //On cherche la premiere date
    uint32_t date = std::numeric_limits<uint32_t>::max();
    for(unsigned int rpid : marked_stop) {
        if(retour[0][rpid].dt.date() < date)
            date = retour[0][rpid].dt.date();
    }

    if(date == std::numeric_limits<uint32_t>::max()) {
        routesValides.clear();
    } else {
        //On active les routes
        int i = 0;
        for(Route_t & route : routes) {
            routesValides.set(i, (route.vp.check(date) || route.vp.check(date+1)));
            ++i;
        }
    }



}

void RAPTOR::setRoutesValidesreverse(boost::dynamic_bitset<> &routesValides, std::vector<unsigned int> &marked_stop, map_retour_t &retour) {

    //On cherche la premiere date
    uint32_t date = std::numeric_limits<uint32_t>::min();
    for(unsigned int rpid : marked_stop) {
        if(retour[0][rpid].dt.date() > date)
            date = retour[0][rpid].dt.date();
    }

    //On active les routes
    int i = 0;
    if(date > 0) {
        for(Route_t & route : routes) {
            routesValides.set(i, (route.vp.check(date) || route.vp.check(date-1)));
            ++i;
        }
    } else if(date == 0) {
        for(Route_t & route : routes) {
            routesValides.set(i, route.vp.check(date));
            ++i;
        }
    } else {
        routesValides.clear();
    }
}



void RAPTOR::boucleRAPTOR(std::vector<unsigned int> &marked_stop, map_retour_t &retour, map_int_pint_t &best, best_dest &b_dest, unsigned int & count) {
    queue_t Q;
    unsigned int routeidx;
    int t;
    bool end = false;

    boost::dynamic_bitset<> routesValides(routes.size());
    boost::dynamic_bitset<> marked_rp(data.pt_data.route_points.size());
    BOOST_FOREACH(auto said, marked_stop) {
        marked_rp.set(said, true);
    }
    setRoutesValides(routesValides, marked_stop, retour);

    marcheapied(marked_rp, retour, best, b_dest, 0);

    while(!end) {
        end = true;

        retour.push_back(retour_constant);

        make_queue(marked_rp, routesValides, Q);
        routeidx = 0;
        BOOST_FOREACH(queue_t::value_type vq, Q) {
            t = -1;
            DateTime workingDt;
            int embarquement = -1;
            const Route_t & route = routes[routeidx];
            for(int i = vq; i < route.nbStops; ++i) {
                int rpid = data.pt_data.routes[route.idx].route_point_list[i];
                if(t >= 0) {
                    const StopTime_t & st = stopTimes[get_stop_time_idx(route, t, i)];
                    workingDt.update(st.arrival_time);
                    //On stocke, et on marque pour explorer par la suite
                    if(workingDt < std::min(best[rpid].dt, b_dest.best_now.dt)) {
                        const type_retour retour_temp = type_retour(st.idx, embarquement, workingDt);
                        retour.back()[rpid] = retour_temp;
                        best[rpid]          = retour_temp;
                        b_dest.ajouter_best(rpid, retour_temp);
                        marked_rp.set(rpid);
                        end = false;
                    }
                }

                //Si on peut arriver plus tôt à l'arrêt en passant par une autre route
                const type_retour & retour_temp = retour[count-1][rpid];
                if((retour_temp.type != uninitialized) &&
                        (((retour_temp.dt.hour() <= get_temps_depart(route, t, i)) && (retour_temp.dt.date() == workingDt.date()) ) ||
                         ((retour_temp.dt.date() < workingDt.date()) ))) {
                    int etemp = earliest_trip(route, i, retour_temp.dt, t);
                    if(etemp >= 0 && t!=etemp) {
                        t = etemp;
                        workingDt = retour_temp.dt;
                        embarquement = rpid;
                        workingDt.update(get_temps_depart(route, t,i));
                    }
                }
            }

            ++routeidx;
        }
        marcheapied(marked_rp, retour, best, b_dest, count);
        ++count;
    }
}


void RAPTOR::boucleRAPTORreverse(std::vector<unsigned int> &marked_stop, map_retour_t &retour, map_int_pint_t &best, best_dest &b_dest, unsigned int & count) {
    queue_t Q;


    unsigned int routeidx;
    int t;
    boost::dynamic_bitset<> routesValides(routes.size());

    boost::dynamic_bitset<> marked_rp(data.pt_data.route_points.size());
    marked_rp.reset();
    BOOST_FOREACH(auto rpid, marked_stop) {
        marked_rp.set(rpid);
    }
    setRoutesValidesreverse(routesValides, marked_stop, retour);

    marcheapiedreverse(marked_rp, retour, best, b_dest, 0);


    for(unsigned int i = 0; i < best.size(); ++i)
        best[i].dt = DateTime::min;

    //    std::cout << "retour 0 1 " << retour.at(0).at(1).dt << std::endl;
    bool end = false;
    while(!end) {
        end = true;
        retour.push_back(retour_constant_reverse);
        make_queuereverse(marked_rp, routesValides, Q);
        marked_rp.reset();
        routeidx = 0;
        BOOST_FOREACH(queue_t::value_type vq, Q) {
            t = -1;
            DateTime workingDt = DateTime::min;
            int embarquement = -1;
            const Route_t & route = routes[routeidx];
            for(int i = vq; i >=0; --i) {
                int rpid = data.pt_data.routes[route.idx].route_point_list[i];

                if(t >= 0) {
                    const StopTime_t & st = stopTimes[get_stop_time_idx(route, t, i)];
                    workingDt.updatereverse(st.departure_time);
                    //On stocke, et on marque pour explorer par la suite
                    if(workingDt > std::max(best[rpid].dt, b_dest.best_now.dt)) {
                        const type_retour retour_temp = type_retour(st.idx, embarquement, workingDt);
                        retour.back()[rpid] = retour_temp;
                        best[rpid]          = retour_temp;
                        b_dest.ajouter_best_reverse(rpid, retour_temp);
                        marked_rp.set(rpid);
                        end = false;
                    } /*else if(workingDt == std::max(best[rpid].dt, b_dest.best_now.dt)) {
                        marked_rp.set(rpid);
                        end = false;
                    }*/
                }

                //Si on peut arriver plus tôt à l'arrêt en passant par une autre route

                const type_retour & retour_temp = retour[count-1][rpid];
                uint32_t temps_arrivee = get_temps_arrivee(route, t, i);

                if((retour_temp.type != uninitialized) &&
                        ( (temps_arrivee == std::numeric_limits<uint32_t>::max()) ||
                          ((retour_temp.dt.hour() >= temps_arrivee) && (retour_temp.dt.date() == workingDt.date()) ) ||
                          ((retour_temp.dt.date() > workingDt.date()) ))) {
                    int etemp = tardiest_trip(route, i, retour_temp.dt, t);
                    if(etemp >=0 && t!=etemp) {
                        t = etemp;
                        workingDt = retour_temp.dt;
                        embarquement = rpid;
                        //                        workingDt.updatereverse(get_temps_departreverse(route, t,i));
                    }
                }
            }
            ++routeidx;
        }
        marcheapiedreverse(marked_rp, retour, best, b_dest, count);
        ++count;

    }
}


Path RAPTOR::makeBestPath(map_retour_t &retour, map_int_pint_t &best, vector_idxretour departs, unsigned int destination_idx, unsigned int count) {
    unsigned int countb = 0;
    for(;countb<count;++countb) {
        if(retour[countb][destination_idx].type != uninitialized) {
            if((retour[countb][destination_idx].stid == best[destination_idx].stid) && (retour[countb][destination_idx].dt == best[destination_idx].dt)) {
                break;
            }
        }
    }

    return makePath(retour, best, departs, destination_idx, countb);
}

Path RAPTOR::makeBestPathreverse(map_retour_t &retour, map_int_pint_t &best, vector_idxretour departs, unsigned int destination_idx, unsigned int count) {
    unsigned int countb = 0;
    for(;countb<count;++countb) {
        if(retour[countb][destination_idx].type != uninitialized) {
            if((retour[countb][destination_idx].stid == best[destination_idx].stid) && (retour[countb][destination_idx].dt == best[destination_idx].dt)) {
                break;
            }
        }
    }
    if(countb<count)
        return makePathreverse(retour, best, departs, destination_idx, countb);
    else {
        return Path();
    }
}

std::vector<Path> RAPTOR::makePathes(map_retour_t &retour, map_int_pint_t &best, vector_idxretour departs, best_dest &b_dest, unsigned int count) {
    std::vector<Path> result;

    for(unsigned int i=1;i<count;++i) {
        DateTime dt;
        int spid = std::numeric_limits<int>::max();
        BOOST_FOREACH(auto dest, b_dest.map_date_time) {
            if(retour[i][dest.first].type != uninitialized && dest.second.dt < dt) {
                dt = dest.second.dt;
                spid = dest.first;
            }
        }
        if(spid != std::numeric_limits<int>::max())
            result.push_back(makePath(retour, best, departs, spid, i));
    }

    return result;
}
std::vector<Path> RAPTOR::makePathesreverse(map_retour_t &retour, map_int_pint_t &best, vector_idxretour departs, best_dest &b_dest, unsigned int count) {
    std::vector<Path> result;

    for(unsigned int i=1;i<count;++i) {
        DateTime dt;
        int spid = std::numeric_limits<int>::max();
        BOOST_FOREACH(auto dest, b_dest.map_date_time) {
            if(retour[i][dest.first].type != uninitialized && dest.second.dt < dt) {
                dt = dest.second.dt;
                spid = dest.first;
            }
        }
        if(spid != std::numeric_limits<int>::max())
            result.push_back(makePathreverse(retour, best, departs, spid, i));
    }

    return result;
}


Path RAPTOR::makePath(map_retour_t &retour, map_int_pint_t &best, vector_idxretour departs, unsigned int destination_idx, unsigned int countb, bool reverse) {
    Path result;
    unsigned int current_rpid = destination_idx;
    type_retour r = retour[countb][current_rpid];
    DateTime workingDate = r.dt;
    navitia::type::StopTime current_st;
    navitia::type::idx_t spid_embarquement = navitia::type::invalid_idx;

    bool stop = false;
    for(auto item : departs) {
        stop = stop || item.first == current_rpid;
    }

    result.nb_changes = countb - 1;
    PathItem item;
    // On boucle tant
    while(!stop) {

        // Est-ce qu'on a une section marche à pied ?
        if(retour[countb][current_rpid].type == connection) {
            r = retour[countb][current_rpid];
            workingDate = r.dt;
            workingDate.normalize();

            if(!reverse)
                item = PathItem(retour[countb][r.said_emarquement].dt, workingDate);
            else
                item = PathItem(workingDate, retour[countb][r.said_emarquement].dt);
            int debugsaid = data.pt_data.stop_points[data.pt_data.route_points[current_rpid].stop_point_idx].stop_area_idx ;
            item.stop_points.push_back(data.pt_data.route_points[current_rpid].stop_point_idx);
            item.type = walking;
            item.stop_points.push_back(data.pt_data.route_points[r.said_emarquement].stop_point_idx);
            result.items.push_back(item);

            spid_embarquement = navitia::type::invalid_idx;
            current_rpid = r.said_emarquement;

        } else { // Sinon c'est un trajet TC
            // Est-ce que qu'on a affaire à un nouveau trajet ?
            if(spid_embarquement == navitia::type::invalid_idx) {
                r = retour[countb][current_rpid];
                spid_embarquement = r.said_emarquement;
                current_st = data.pt_data.stop_times.at(r.stid);
                workingDate = r.dt;

                item = PathItem();
                item.type = public_transport;

                if(!reverse)
                    item.arrival = workingDate;
                else
                    item.departure = DateTime(workingDate.date(), current_st.departure_time);

                while(spid_embarquement != current_rpid) {
                    navitia::type::StopTime prec_st = current_st;
                    if(!reverse)
                        current_st = data.pt_data.stop_times.at(data.pt_data.vehicle_journeys.at(current_st.vehicle_journey_idx).stop_time_list.at(current_st.order-1));
                    else
                        current_st = data.pt_data.stop_times.at(data.pt_data.vehicle_journeys.at(current_st.vehicle_journey_idx).stop_time_list.at(current_st.order+1));

                    if(!reverse && current_st.arrival_time%86400 > prec_st.arrival_time%86400 && prec_st.vehicle_journey_idx!=navitia::type::invalid_idx)
                        workingDate.date_decrement();
                    else if(reverse && current_st.arrival_time%86400 < prec_st.arrival_time%86400 && prec_st.vehicle_journey_idx!=navitia::type::invalid_idx)
                        workingDate.date_increment();

                    workingDate = DateTime(workingDate.date(), current_st.arrival_time);
                    item.stop_points.push_back(data.pt_data.route_points[current_rpid].stop_point_idx);
                    item.vj_idx = current_st.vehicle_journey_idx;

                    current_rpid = current_st.route_point_idx;
                }
                item.stop_points.push_back(data.pt_data.route_points[current_rpid].stop_point_idx);

                if(!reverse)
                    item.departure = DateTime(workingDate.date(), current_st.departure_time);
                else
                    item.arrival = workingDate;

                result.items.push_back(item);
                --countb;
                spid_embarquement = navitia::type::invalid_idx ;

            }
        }

        for(auto item : departs) {
            stop = stop || item.first == current_rpid;
        }
    }

    if(!reverse){
        std::reverse(result.items.begin(), result.items.end());
        for(auto & item : result.items) {
            std::reverse(item.stop_points.begin(), item.stop_points.end());
        }
    }

    if(result.items.size() > 0)
        result.duration = result.items.back().arrival - result.items.front().departure;
    else
        result.duration = 0;
    int count_visites = 0;
    for(auto t: best) {
        if(t.type != uninitialized) {
            ++count_visites;
        }
    }
    result.percent_visited = 100*count_visites / data.pt_data.stop_points.size();

    return result;
}


Path RAPTOR::makePathreverse(map_retour_t &retour, map_int_pint_t &best, vector_idxretour departs, unsigned int destination_idx, unsigned int countb) {
    return makePath(retour, best, departs, destination_idx, countb, true);
}


Path communRAPTOR::compute(idx_t departure_idx, idx_t destination_idx, int departure_hour, int departure_day, senscompute sens) {
    vector_idxretour departs, destinations;


    switch(sens) {

    case partirapresrab:
        for(navitia::type::idx_t spidx : data.pt_data.stop_areas[departure_idx].stop_point_list) {
            for(navitia::type::idx_t rpidx : data.pt_data.stop_points[spidx].route_point_list) {
                departs.push_back(std::make_pair(rpidx, type_retour(-1, DateTime(departure_day, departure_hour))));
            }
        }


        for(navitia::type::idx_t spidx : data.pt_data.stop_areas[destination_idx].stop_point_list) {
            for(navitia::type::idx_t rpidx : data.pt_data.stop_points[spidx].route_point_list) {
                destinations.push_back(std::make_pair(rpidx, type_retour()));
            }
        }
        return compute_raptor_rabattement(departs, destinations); break;
    case arriveravant :
        for(navitia::type::idx_t spidx : data.pt_data.stop_areas[destination_idx].stop_point_list) {
            for(navitia::type::idx_t rpidx : data.pt_data.stop_points[spidx].route_point_list) {
                departs.push_back(std::make_pair(rpidx, type_retour(-1, DateTime(departure_day, departure_hour))));
            }
        }


        for(navitia::type::idx_t spidx: data.pt_data.stop_areas[departure_idx].stop_point_list) {
            for(navitia::type::idx_t rpidx: data.pt_data.stop_points[spidx].route_point_list) {
                destinations.push_back(std::make_pair(rpidx, type_retour()));
            }
        }

        return compute_raptor_reverse(departs, destinations); break;
    default :
        for(navitia::type::idx_t spidx : data.pt_data.stop_areas[departure_idx].stop_point_list) {
            for(navitia::type::idx_t rpidx: data.pt_data.stop_points[spidx].route_point_list)  {
                departs.push_back(std::make_pair(rpidx, type_retour(-1, DateTime(departure_day, departure_hour))));
            }
        }


        for(navitia::type::idx_t spidx : data.pt_data.stop_areas[destination_idx].stop_point_list) {
            for(navitia::type::idx_t rpidx : data.pt_data.stop_points[spidx].route_point_list)  {
                destinations.push_back(std::make_pair(rpidx, type_retour()));
            }
        }
        return compute_raptor(departs, destinations); break;
    }
}

Path communRAPTOR::compute_reverse(idx_t departure_idx, idx_t destination_idx, int departure_hour, int departure_day) {
    vector_idxretour departs, destinations;

    BOOST_FOREACH(navitia::type::idx_t spidx, data.pt_data.stop_areas[destination_idx].stop_point_list) {
        BOOST_FOREACH(navitia::type::idx_t rpidx, data.pt_data.stop_points[spidx].route_point_list)  {
            departs.push_back(std::make_pair(rpidx, type_retour(-1, DateTime(departure_day, departure_hour))));
        }
    }


    BOOST_FOREACH(navitia::type::idx_t spidx, data.pt_data.stop_areas[departure_idx].stop_point_list) {
        BOOST_FOREACH(navitia::type::idx_t rpidx, data.pt_data.stop_points[spidx].route_point_list)  {
            destinations.push_back(std::make_pair(rpidx, type_retour()));
        }
    }

    return compute_raptor_reverse(departs, destinations);
}

Path communRAPTOR::compute_rabattement(idx_t departure_idx, idx_t destination_idx, int departure_hour, int departure_day) {
    vector_idxretour departs, destinations;

    BOOST_FOREACH(navitia::type::idx_t spidx, data.pt_data.stop_areas[departure_idx].stop_point_list) {
        BOOST_FOREACH(navitia::type::idx_t rpidx, data.pt_data.stop_points[spidx].route_point_list)  {
            departs.push_back(std::make_pair(rpidx, type_retour(-1, DateTime(departure_day, departure_hour))));
        }
    }


    BOOST_FOREACH(navitia::type::idx_t spidx, data.pt_data.stop_areas[destination_idx].stop_point_list) {
        BOOST_FOREACH(navitia::type::idx_t rpidx, data.pt_data.stop_points[spidx].route_point_list)  {
            destinations.push_back(std::make_pair(rpidx, type_retour()));
        }
    }


    return compute_raptor_rabattement(departs, destinations);
}

std::vector<Path> RAPTOR::compute_all(navitia::type::EntryPoint departure, navitia::type::EntryPoint destination, int departure_hour, int departure_day) {
    //Ne fonctionne pas !
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
            prox = (retour) (data.street_network.find_nearest(departure, data.pt_data.stop_point_proximity_list, radius));
        } catch(NotFound) {return Path();}
        BOOST_FOREACH(auto item, prox) {
            for(navitia::type::idx_t rpidx: data.pt_data.stop_points[item.first].route_point_list) {
                int temps = departure_hour + (item.second / 80);
                int day;
                if(temps > 86400) {
                    temps = temps % 86400;
                    day = departure_day + 1;
                } else {
                    day = departure_day;
                }
                bests.push_back(std::make_pair(rpidx, type_retour(-1, DateTime(day, temps))));
            }

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
        prox = (retour) (data.street_network.find_nearest(departure, data.pt_data.stop_point_proximity_list, radius_depart));
    } catch(NotFound) {std::cout << "Not found 1 " << std::endl; return ;}

    BOOST_FOREACH(auto item, prox) {
        for(navitia::type::idx_t rpidx: data.pt_data.stop_points[item.first].route_point_list) {
            int temps = departure_hour + (item.second / 80);
            int day;
            if(temps > 86400) {
                temps = temps % 86400;
                day = departure_day + 1;
            } else {
                day = departure_day;
            }
            departs[rpidx] =  type_retour(-1, DateTime(day, temps), 0, (item.second / 80));
        }

    }


    prox.clear();
    try {
        prox = (retour) (data.street_network.find_nearest(destination, data.pt_data.stop_point_proximity_list, radius_destination));
    } catch(NotFound) {std::cout << "Not found 2 " << std::endl;return ;}
    BOOST_FOREACH(auto item, prox) {
        for(navitia::type::idx_t rpidx : data.pt_data.stop_points[item.first].route_point_list) {
            destinations[rpidx] = type_retour((int)(item.second/80));
        }
    }
}

void communRAPTOR::trouverGeo(const type::GeographicalCoord & departure, double radius_depart, const type::GeographicalCoord & destination, double radius_destination,
                              int departure_hour, int departure_day, vector_idxretour &departs, vector_idxretour &destinations) {
    typedef std::vector< std::pair<idx_t, double> > retour;

    retour prox;

    try {
        prox = (retour) (data.street_network.find_nearest(departure, data.pt_data.stop_point_proximity_list, radius_depart));
    } catch(NotFound) {std::cout << "Not found 1 " << std::endl; return ;}


    BOOST_FOREACH(auto item, prox) {
        for(navitia::type::idx_t rpidx : data.pt_data.stop_points[item.first].route_point_list) {
            int temps = departure_hour + (item.second / 80);
            int day;
            if(temps > 86400) {
                temps = temps % 86400;
                day = departure_day + 1;
            } else {
                day = departure_day;
            }
            departs.push_back(std::make_pair(rpidx, type_retour(-1, DateTime(day, temps), 0, (item.second / 80))));
        }

    }


    prox.clear();
    try {
        prox = (retour) (data.street_network.find_nearest(destination, data.pt_data.stop_point_proximity_list, radius_destination));
    } catch(NotFound) {std::cout << "Not found 2 " << std::endl;return ;}
    BOOST_FOREACH(auto item, prox) {
        for(navitia::type::idx_t rpidx : data.pt_data.stop_points[item.first].route_point_list) {
            destinations.push_back(std::make_pair(rpidx, type_retour((int)(item.second/80))));
        }

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

std::vector<Path> RAPTOR::compute_all(const type::GeographicalCoord & departure, double radius_depart, const type::GeographicalCoord & destination, double radius_destination
                                      , int departure_hour, int departure_day) {
    vector_idxretour departs, destinations;

    trouverGeo(departure, radius_depart, destination, radius_destination, departure_hour, departure_day, departs, destinations);

    std::cout << "Nb stations departs : " << departs.size() << " destinations : " << destinations.size() << std::endl;

    return compute_all(departs, destinations);
}


}}}
