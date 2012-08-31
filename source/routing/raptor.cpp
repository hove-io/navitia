#include "raptor.h"
namespace navitia { namespace routing { namespace raptor {

communRAPTOR::communRAPTOR(navitia::type::Data &data) : data(data), cp(data)
{
    //Construction de la liste des marche à pied à partir des connections renseignées
    Timer t;
    std::vector<list_connections> footpath_temp, footpathreverse_temp;
    footpath_temp.resize(data.pt_data.stop_points.size());
    footpathreverse_temp.resize(data.pt_data.stop_points.size());
    BOOST_FOREACH(navitia::type::Connection connection, data.pt_data.connections) {
        footpath_temp[connection.departure_stop_point_idx].push_back(Connection_t(connection.departure_stop_point_idx, connection.destination_stop_point_idx, connection.idx, connection.duration));
        footpath_temp[connection.destination_stop_point_idx].push_back(Connection_t(connection.departure_stop_point_idx, connection.destination_stop_point_idx, connection.idx, connection.duration));
    }

    //On rajoute des connexions entre les stops points d'un même stop area si elles n'existent pas
    footpath_index.resize(data.pt_data.stop_points.size());
    BOOST_FOREACH(navitia::type::StopPoint sp, data.pt_data.stop_points) {

        navitia::type::StopArea sa = data.pt_data.stop_areas.at(sp.stop_area_idx);
        footpath_index[sp.idx].first = foot_path.size();

        int size = footpath_temp[sp.idx].size();
        foot_path.insert(foot_path.end(), footpath_temp[sp.idx].begin(), footpath_temp[sp.idx].end());



        BOOST_FOREACH(navitia::type::idx_t spidx, sa.stop_point_list) {

            if(sp.idx != spidx) {
                bool find = false;
                BOOST_FOREACH(Connection_t connection, footpath_temp[sp.idx]) {
                    if(connection.destination_sp == spidx) {
                        find = true;
                        break;
                    }
                }
                if(!find) {
                    foot_path.push_back(Connection_t(sp.idx, spidx, 2*60));
                    ++size;
                }

            }
        }
        footpath_index[sp.idx].second = size;
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
    //    sp_indexrouteorder.resize(data.pt_data.stop_points.size());
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
        sp_indexrouteorder.push_back(std::make_pair(sp_routeorder.size(), route_order.size()));
        //        sp_routeorder[i] = vector_pairint();

        BOOST_FOREACH(auto pair, route_order) {
            sp_routeorder.push_back(pair);
        }
    }
    std::cout << "Chargement des sa route" << t.ms() << std::endl;;

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
            //current_trip = first; step=count/2; current_trip+=step;
            current_trip = first + (upper - first) / 2;
            int current_stop_time = route.firstStopTime + order + current_trip*route.nbStops;
            if (stopTimes[current_stop_time].arrival_time > dt.hour()) {
                upper = current_trip;
                //first=++current_trip; count-=step+1;
            }
            else first = current_trip;
        }

        if(first != upper)
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

    auto it = sp_routeorder.begin();
    int last = 0;

    for(auto spid = stops.find_first(); spid != stops.npos; spid = stops.find_next(spid)) {
        const auto & index = sp_indexrouteorder[spid];
        advance(it, index.first - last);
        const auto end = it + index.second;
        for(; it != end; ++it) {
            if(routesValides.test(it->first)) {
                if(Q[it->first] > it->second)
                    Q[it->first] = it->second;
            }
        }
        last = index.first + index.second;

        stops.set(spid, false);
    }
}

void RAPTOR::make_queuereverse(boost::dynamic_bitset<> &stops, boost::dynamic_bitset<> & routesValides, queue_t &Q) {
    Q.assign(routes.size(), std::numeric_limits<int>::min());

    auto it = sp_routeorder.begin();
    int last = 0;

    for(auto spid = stops.find_first(); spid != stops.npos; spid = stops.find_next(spid)) {
        const auto & index = sp_indexrouteorder[spid];
        advance(it, index.first - last);
        const auto end = it + index.second;
        for(; it != end; ++it) {
            if(routesValides.test(it->first)) {
                if(Q[it->first] < it->second)
                    Q[it->first] = it->second;
            }
        }
        last = index.first + index.second;

        stops.set(spid, false);
    }
}

void RAPTOR::marcheapied(/*std::vector<unsigned int>*/boost::dynamic_bitset<> & marked_stop, map_retour_t &retour, map_int_pint_t &best, best_dest &b_dest, unsigned int count) {

    auto it = foot_path.begin();
    int last = 0;
    for(auto stop_point= marked_stop.find_first(); stop_point != marked_stop.npos; stop_point = marked_stop.find_next(stop_point)) {
        const auto & index = footpath_index[stop_point];
        advance(it, index.first - last);
        const auto end = it + index.second;
        for(; it != end; ++it) {
            const type_retour & retour_temp = retour[count][stop_point];
            const DateTime dtTemp = retour_temp.dt + (*it).duration;
            if(dtTemp < best[(*it).destination_sp].dt) {
                const type_retour nRetour = type_retour(navitia::type::invalid_idx, stop_point, dtTemp, connection);
                best[(*it).destination_sp] = nRetour;
                retour[count][(*it).destination_sp] = nRetour;
                b_dest.ajouter_best((*it).destination_sp, nRetour);
                marked_stop.set((*it).destination_sp);
            }
        }
        last = index.first + index.second;
    }
}


void RAPTOR::marcheapiedreverse(boost::dynamic_bitset<> & marked_stop, map_retour_t &retour, map_int_pint_t &best, best_dest &b_dest, unsigned int count) {
    auto it = foot_path.begin();
    int last = 0;
    for(auto stop_point= marked_stop.find_first(); stop_point != marked_stop.npos; stop_point = marked_stop.find_next(stop_point)) {
        const auto & index = footpath_index[stop_point];
        advance(it, index.first - last);
        const auto end = it + index.second;
        for(; it != end; ++it) {
            const type_retour & retour_temp = retour[count][stop_point];
            const DateTime dtTemp = retour_temp.dt - (*it).duration;
            if(dtTemp > best[(*it).departure_sp].dt) {
                const type_retour nRetour = type_retour(navitia::type::invalid_idx, stop_point, dtTemp, connection);
                best[(*it).departure_sp] = nRetour;
                retour[count][(*it).departure_sp] = nRetour;
                b_dest.ajouter_best((*it).departure_sp, nRetour);
                marked_stop.set((*it).departure_sp);
            }
        }
        last = index.first + index.second;
    }
}

Path RAPTOR::compute_raptor(vector_idxretour departs, vector_idxretour destinations) {
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

    if(b_dest.best_now.type != uninitialized) {
        unsigned int destination_idx = b_dest.best_now_spid;
        return makeBestPath(retour, best, departs, destination_idx, count);
    }
    Path result;
    return result;
}

Path RAPTOR::compute_raptor_reverse(vector_idxretour departs, vector_idxretour destinations) {
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


    boucleRAPTORreverse(marked_stop, retour, best, b_dest, count);

    if(b_dest.best_now.type != uninitialized) {
        unsigned int destination_idx = b_dest.best_now_spid;
        return makeBestPathreverse(retour, best, departs, destination_idx, count);
    }
    Path result;
    return result;
}


Path RAPTOR::compute_raptor_rabattement(vector_idxretour departs, vector_idxretour destinations) {
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

    unsigned int count = 1, countb = 1;


    boucleRAPTOR(marked_stop, retour, best, b_dest, count);

    for(;countb<=count;++countb) {
        if(retour[countb][b_dest.best_now_spid].type != uninitialized) {
            if((retour[countb][b_dest.best_now_spid].stid == best[b_dest.best_now_spid].stid) && (retour[countb][b_dest.best_now_spid].dt == best[b_dest.best_now_spid].dt)) {
                break;
            }
        }
    }
    count = 1;
    retour.clear();
    retour.push_back(map_int_pint_t());
    retour.back().resize(data.pt_data.stop_points.size());
    marked_stop.clear();
    b_dest.reinit();
    b_dest.reverse();
    BOOST_FOREACH(auto item, destinations) {
        retour.back()[item.first] = best[item.first];
        marked_stop.push_back(item.first);
    }

    BOOST_FOREACH(auto item, departs) {
        b_dest.ajouter_destination(item.first, item.second);
    }

    boucleRAPTORreverse(marked_stop, retour, best, b_dest, count);

    if(b_dest.best_now.type != uninitialized) {
        unsigned int destination_idx = b_dest.best_now_spid;
        return makeBestPathreverse(retour, best, destinations, destination_idx, count);
    }
    Path result;
    return result;
}




std::vector<Path> RAPTOR::compute_all(vector_idxretour departs, vector_idxretour destinations) {
    map_retour_t retour;
    map_int_pint_t best;
    std::vector<unsigned int> marked_stop;


    retour.push_back(map_int_pint_t());
    retour.back().resize(data.pt_data.stop_points.size());
    best.resize(data.pt_data.stop_points.size());
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



void RAPTOR::setRoutesValides(boost::dynamic_bitset<> &routesValides, std::vector<unsigned int> &marked_stop, map_retour_t &retour) {

    //On cherche la premiere date
    uint32_t date = std::numeric_limits<uint32_t>::max();
    BOOST_FOREACH(unsigned int said, marked_stop) {
        if(retour[0][said].dt.date() < date)
            date = retour[0][said].dt.date();
    }

    if(date == std::numeric_limits<uint32_t>::max()) {
        routesValides.clear();
    } else {
        //On active les routes
        int i = 0;
        BOOST_FOREACH(Route_t route, routes) {
            routesValides.set(i, (route.vp.check(date) || route.vp.check(date+1)));
            ++i;
        }
    }



}

void RAPTOR::setRoutesValidesreverse(boost::dynamic_bitset<> &routesValides, std::vector<unsigned int> &marked_stop, map_retour_t &retour) {

    //On cherche la premiere date
    uint32_t date = std::numeric_limits<uint32_t>::min();
    BOOST_FOREACH(unsigned int said, marked_stop) {
        if(retour[0][said].dt.date() > date)
            date = retour[0][said].dt.date();
    }

    //On active les routes
    int i = 0;
    if(date > 0) {
        BOOST_FOREACH(Route_t route, routes) {
            routesValides.set(i, (route.vp.check(date) || route.vp.check(date-1)));
            ++i;
        }
    } else if(date == 0) {
        BOOST_FOREACH(Route_t route, routes) {
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
    boost::dynamic_bitset<> marked_sp(data.pt_data.stop_points.size());
    BOOST_FOREACH(auto said, marked_stop) {
        marked_sp.set(said, true);
    }
    setRoutesValides(routesValides, marked_stop, retour);

    marcheapied(marked_sp, retour, best, b_dest, 0);

    while(!end) {
        end = true;

        retour.push_back(retour_constant);
        make_queue(marked_sp, routesValides, Q);
        routeidx = 0;
        BOOST_FOREACH(queue_t::value_type vq, Q) {
            t = -1;
            DateTime workingDt;
            int embarquement = -1;
            const Route_t & route = routes[routeidx];
            for(int i = vq; i < route.nbStops; ++i) {
                int spid = data.pt_data.route_points[data.pt_data.routes[route.idx].route_point_list[i]].stop_point_idx;
                if(t >= 0) {
                    const StopTime_t & st = stopTimes[get_stop_time_idx(route, t, i)];
                    workingDt.update(st.arrival_time);
                    //On stocke, et on marque pour explorer par la suite
                    if(workingDt < std::min(best[spid].dt, b_dest.best_now.dt)) {
                        const type_retour retour_temp = type_retour(st.idx, embarquement, workingDt);
                        retour.back()[spid] = retour_temp;
                        best[spid]          = retour_temp;
                        b_dest.ajouter_best(spid, retour_temp);
                        marked_sp.set(spid);
                        end = false;
                    }
                }

                //Si on peut arriver plus tôt à l'arrêt en passant par une autre route
                const type_retour & retour_temp = retour[count-1][spid];
                if((retour_temp.type != uninitialized) &&
                        (((retour_temp.dt.hour() <= get_temps_depart(route, t, i)) && (retour_temp.dt.date() == workingDt.date()) ) ||
                         ((retour_temp.dt.date() < workingDt.date()) ))) {
                    int etemp = earliest_trip(route, i, retour_temp.dt, t);
                    //                    std::tie(t, pam) = earliest_trip(route, i, retour_temp.dt, t);
                    if(etemp >= 0 && (etemp < t || t == -1)) {
                        t = etemp;
                        workingDt = retour_temp.dt;
                        embarquement = spid;
                        workingDt.update(get_temps_depart(route, t,i));

                    }
                }
            }

            ++routeidx;
        }
        marcheapied(marked_sp, retour, best, b_dest, count);
        ++count;
    }
}


void RAPTOR::boucleRAPTORreverse(std::vector<unsigned int> &marked_stop, map_retour_t &retour, map_int_pint_t &best, best_dest &b_dest, unsigned int & count) {
    queue_t Q;


    unsigned int routeidx;
    int t;
    boost::dynamic_bitset<> routesValides(routes.size());

    boost::dynamic_bitset<> marked_sp(data.pt_data.stop_points.size());
    BOOST_FOREACH(auto said, marked_stop) {
        marked_sp.set(said);
    }

    setRoutesValidesreverse(routesValides, marked_stop, retour);

    marcheapiedreverse(marked_sp, retour, best, b_dest, 0);

    b_dest.reverse();
    for(unsigned int i = 0; i < best.size(); ++i)
        best[i].dt = DateTime::min;

    //    std::cout << "retour 0 1 " << retour.at(0).at(1).dt << std::endl;
    bool end = false;
    while(!end) {
        end = true;
        retour.push_back(retour_constant_reverse);
        make_queuereverse(marked_sp, routesValides, Q);
        routeidx = 0;
        BOOST_FOREACH(queue_t::value_type vq, Q) {
            t = -1;
            DateTime workingDt = DateTime::min;
            int embarquement = -1;
            const Route_t & route = routes[routeidx];
            for(int i = vq; i >=0; --i) {
                int spid = data.pt_data.route_points[data.pt_data.routes[route.idx].route_point_list[i]].stop_point_idx;

                if(t >= 0) {
                    const StopTime_t & st = stopTimes[get_stop_time_idx(route, t, i)];
                    workingDt.updatereverse(st.arrival_time);
                    //On stocke, et on marque pour explorer par la suite
                    if(workingDt > std::max(best[spid].dt, b_dest.best_now.dt)) {
                        const type_retour retour_temp = type_retour(st.idx, embarquement, workingDt);
                        retour.back()[spid] = retour_temp;
                        best[spid]          = retour_temp;
                        b_dest.ajouter_best_reverse(spid, retour_temp);
                        marked_sp.set(spid);
                        end = false;
                    }
                }

                //Si on peut arriver plus tôt à l'arrêt en passant par une autre route
                const type_retour & retour_temp = retour[count-1][spid];
                if((retour_temp.type != uninitialized) &&
                        (((retour_temp.dt.hour() >= get_temps_departreverse(route, t, i)) && (retour_temp.dt.date() == workingDt.date()) ) ||
                         ((retour_temp.dt.date() > workingDt.date()) ))) {
                    int etemp = tardiest_trip(route, i, retour_temp.dt, t);
                    if(etemp >=0 && t!=etemp) {
                        t = etemp;
                        workingDt = retour[count - 1][spid].dt;
                        embarquement = spid;
                        //                        workingDt.updatereverse(get_temps_departreverse(route, t,i));
                    }
                }
            }
            ++routeidx;
        }
        marcheapiedreverse(marked_sp, retour, best, b_dest, count);
        ++count;

    }
}


Path RAPTOR::makeBestPath(map_retour_t &retour, map_int_pint_t &best, vector_idxretour departs, unsigned int destination_idx, unsigned int count) {
    unsigned int countb = 1;
    for(;countb<=count;++countb) {
        if(retour[countb][destination_idx].type != uninitialized) {
            if((retour[countb][destination_idx].stid == best[destination_idx].stid) && (retour[countb][destination_idx].dt == best[destination_idx].dt)) {
                break;
            }
        }
    }

    return makePath(retour, best, departs, destination_idx, countb);
}

Path RAPTOR::makeBestPathreverse(map_retour_t &retour, map_int_pint_t &best, vector_idxretour departs, unsigned int destination_idx, unsigned int count) {
    unsigned int countb = 1;
    for(;countb<=count;++countb) {
        if(retour[countb][destination_idx].type != uninitialized) {
            if((retour[countb][destination_idx].stid == best[destination_idx].stid) && (retour[countb][destination_idx].dt == best[destination_idx].dt)) {
                break;
            }
        }
    }

    return makePathreverse(retour, best, departs, destination_idx, countb);
}

std::vector<Path> RAPTOR::makePathes(map_retour_t &retour, map_int_pint_t &best, vector_idxretour departs, best_dest &b_dest, unsigned int count) {
    std::vector<Path> result;

    for(unsigned int i=1;i<count;++i) {
        DateTime dt;
        int spid = std::numeric_limits<int>::max();
        BOOST_FOREACH(auto dest, b_dest.map_date_time) {
            if(retour[i][dest.first] != type_retour() && dest.second.dt < dt) {
                dt = dest.second.dt;
                spid = dest.first;
            }
        }
        if(spid != std::numeric_limits<int>::max())
            result.push_back(makePath(retour, best, departs, spid, i));
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
                debut = true;
            }
        } else if(retour[countb][current_spid].type == connection) {
            //            if(spid_embarquement == -1) {
            r = retour[countb][current_spid];
            workingDate = r.dt;
            workingDate.normalize();
            result.items.push_back(PathItem(/*data.pt_data.stop_points[*/current_spid/*].stop_area_idx*/, workingDate, workingDate));
            current_spid = r.said_emarquement;
            result.items.push_back(PathItem(/*data.pt_data.stop_points[*/r.said_emarquement/*].stop_area_idx*/, workingDate, workingDate));
            spid_embarquement = -1;
            footpath = true;
            //            }
        }

        if(!footpath) {
            if(!debut) {
                prec_st = current_st;
                current_st = data.pt_data.stop_times.at(data.pt_data.vehicle_journeys.at(current_st.vehicle_journey_idx).stop_time_list.at(current_st.order-1));
                if(current_st.arrival_time%86400 > prec_st.arrival_time%86400 && prec_st.vehicle_journey_idx!=navitia::type::invalid_idx)
                    workingDate.date_decrement();
                workingDate = DateTime(workingDate.date(), current_st.arrival_time);
            }
            current_spid = /*data.pt_data.stop_points.at(*/data.pt_data.route_points.at(current_st.route_point_idx).stop_point_idx/*).stop_area_idx*/;
            if(spid_embarquement == (int)current_spid) {
                --countb;
                spid_embarquement = -1;
            }
            result.items.push_back(PathItem(/*data.pt_data.stop_points[*/current_spid/*].stop_area_idx*/, DateTime(workingDate.date(), current_st.arrival_time), DateTime(workingDate.date(), current_st.departure_time),
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
    result.percent_visited = 100*count_visites / data.pt_data.stop_points.size();

    return result;
}


Path RAPTOR::makePathreverse(map_retour_t &retour, map_int_pint_t &best, vector_idxretour departs, unsigned int destination_idx, unsigned int countb) {
    Path result;
    unsigned int current_said = destination_idx;

    type_retour r = retour[countb][current_said];
    DateTime workingDate = r.dt;
    navitia::type::StopTime current_st, prec_st;
    int said_embarquement = -1;

    bool stop = false;
    BOOST_FOREACH(auto item, departs) {
        stop = stop || (item.first == (int)current_said);
    }

    while(!stop) {
        bool debut = false, footpath = false;

        if(retour[countb][current_said].type == vj) {
            if(said_embarquement == -1) {
                r = retour[countb][current_said];
                said_embarquement = r.said_emarquement;
                current_st = data.pt_data.stop_times.at(r.stid);
                workingDate = r.dt;
                debut = true;
            }
        } else {
            if(said_embarquement == -1) {
                r = retour[countb][current_said];
                workingDate = r.dt;
                workingDate.normalize();
                result.items.push_back(PathItem(current_said, workingDate, workingDate));
                current_said = r.said_emarquement;
                said_embarquement = -1;
                footpath = true;
            }
        }

        if(!footpath) {
            if(!debut) {
                prec_st = current_st;
                current_st = data.pt_data.stop_times.at(data.pt_data.vehicle_journeys.at(current_st.vehicle_journey_idx).stop_time_list.at(current_st.order+1));
                if(current_st.arrival_time%86400 < prec_st.arrival_time%86400 && prec_st.vehicle_journey_idx!=navitia::type::invalid_idx)
                    workingDate.date_decrement();
                workingDate = DateTime(workingDate.date(), current_st.arrival_time);
            }
            current_said = data.pt_data.stop_points.at(data.pt_data.route_points.at(current_st.route_point_idx).stop_point_idx).stop_area_idx;
            if(said_embarquement == (int)current_said) {
                --countb;
                ++ result.nb_changes;
                said_embarquement = -1;
            }
            result.items.push_back(PathItem(current_said, DateTime(workingDate.date(), current_st.arrival_time), DateTime(workingDate.date(), current_st.departure_time),
                                            data.pt_data.routes.at(data.pt_data.route_points.at(current_st.route_point_idx).route_idx).line_idx));
        }

        BOOST_FOREACH(auto item, departs) {
            stop = stop || (item.first == (int)current_said);
        }
    }


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


Path communRAPTOR::compute(idx_t departure_idx, idx_t destination_idx, int departure_hour, int departure_day, senscompute sens) {
    vector_idxretour departs, destinations;


    switch(sens) {
    case partirapres:
        BOOST_FOREACH(navitia::type::idx_t spidx, data.pt_data.stop_areas[departure_idx].stop_point_list)
                departs.push_back(std::make_pair(spidx, type_retour(-1, DateTime(departure_day, departure_hour))));

        BOOST_FOREACH(navitia::type::idx_t spidx, data.pt_data.stop_areas[destination_idx].stop_point_list)
                destinations.push_back(std::make_pair(spidx, type_retour()));
        return compute_raptor(departs, destinations); break;
    case partirapresrab:
        BOOST_FOREACH(navitia::type::idx_t spidx, data.pt_data.stop_areas[departure_idx].stop_point_list)
                departs.push_back(std::make_pair(spidx, type_retour(-1, DateTime(departure_day, departure_hour))));

        BOOST_FOREACH(navitia::type::idx_t spidx, data.pt_data.stop_areas[destination_idx].stop_point_list)
                destinations.push_back(std::make_pair(spidx, type_retour()));
        return compute_raptor_rabattement(departs, destinations); break;
    case arriveravant :
        BOOST_FOREACH(navitia::type::idx_t spidx, data.pt_data.stop_areas[destination_idx].stop_point_list)
                departs.push_back(std::make_pair(spidx, type_retour(-1, DateTime(departure_day, departure_hour))));

        BOOST_FOREACH(navitia::type::idx_t spidx, data.pt_data.stop_areas[departure_idx].stop_point_list)
                destinations.push_back(std::make_pair(spidx, type_retour()));
        return compute_raptor_reverse(departs, destinations); break;

    default :
        BOOST_FOREACH(navitia::type::idx_t spidx, data.pt_data.stop_areas[departure_idx].stop_point_list)
                departs.push_back(std::make_pair(spidx, type_retour(-1, DateTime(departure_day, departure_hour))));

        BOOST_FOREACH(navitia::type::idx_t spidx, data.pt_data.stop_areas[destination_idx].stop_point_list)
                destinations.push_back(std::make_pair(spidx, type_retour()));

        return compute_raptor(departs, destinations); break;
    }
}

Path communRAPTOR::compute_reverse(idx_t departure_idx, idx_t destination_idx, int departure_hour, int departure_day) {
    vector_idxretour departs, destinations;

    BOOST_FOREACH(navitia::type::idx_t spidx, data.pt_data.stop_areas[destination_idx].stop_point_list)
            departs.push_back(std::make_pair(spidx, type_retour(-1, DateTime(departure_day, departure_hour))));

    BOOST_FOREACH(navitia::type::idx_t spidx, data.pt_data.stop_areas[departure_idx].stop_point_list)
            destinations.push_back(std::make_pair(spidx, type_retour()));
    return compute_raptor_reverse(departs, destinations);
}

Path communRAPTOR::compute_rabattement(idx_t departure_idx, idx_t destination_idx, int departure_hour, int departure_day) {
    vector_idxretour departs, destinations;

    BOOST_FOREACH(navitia::type::idx_t spidx, data.pt_data.stop_areas[departure_idx].stop_point_list)
            departs.push_back(std::make_pair(spidx, type_retour(-1, DateTime(departure_day, departure_hour))));

    BOOST_FOREACH(navitia::type::idx_t spidx, data.pt_data.stop_areas[destination_idx].stop_point_list)
            destinations.push_back(std::make_pair(spidx, type_retour()));

    return compute_raptor_rabattement(departs, destinations);
}

std::vector<Path> RAPTOR::compute_all(navitia::type::EntryPoint departure, navitia::type::EntryPoint destination, int departure_hour, int departure_day) {
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
        prox = (retour) (data.street_network.find_nearest(departure, data.pt_data.stop_point_proximity_list, radius_depart));
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
        prox = (retour) (data.street_network.find_nearest(destination, data.pt_data.stop_point_proximity_list, radius_destination));
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
        prox = (retour) (data.street_network.find_nearest(departure, data.pt_data.stop_point_proximity_list, radius_depart));
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
        prox = (retour) (data.street_network.find_nearest(destination, data.pt_data.stop_point_proximity_list, radius_destination));
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

std::vector<Path> RAPTOR::compute_all(const type::GeographicalCoord & departure, double radius_depart, const type::GeographicalCoord & destination, double radius_destination
                                      , int departure_hour, int departure_day) {
    vector_idxretour departs, destinations;

    trouverGeo(departure, radius_depart, destination, radius_destination, departure_hour, departure_day, departs, destinations);

    std::cout << "Nb stations departs : " << departs.size() << " destinations : " << destinations.size() << std::endl;

    return compute_all(departs, destinations);
}


}}}
