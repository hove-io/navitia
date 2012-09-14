#include "raptor.h"
namespace navitia { namespace routing { namespace raptor {



int RAPTOR::earliest_trip(const dataRAPTOR::Route_t & route, unsigned int order, DateTime dt) const {
    int it, step,
            upper = route.nbTrips, count = upper,
            first = 0;

    if(data.pt_data.validity_patterns[route.vp].check(dt.date())) {
        //Recherche dichotomique du premier trip partant après dt.hour
        while (count>0) {
            it = first; step=count/2; it+=step;
            if (data.dataRaptor.stopTimes[route.firstStopTime + order + it*route.nbStops].departure_time < dt.hour()) {
                first=++it; count-=step+1;
            }
            else count=step;
        }

        if(first != upper)
            return first;
    }

    //Si on en a pas trouvé, on cherche le lendemain
    dt.date_increment();
    if(data.pt_data.validity_patterns[route.vp].check(dt.date())) {
        return 0;
    }

    //Cette route ne comporte aucun trip compatible
    return -1;
}


int RAPTOR::tardiest_trip(const dataRAPTOR::Route_t & route, unsigned int order, DateTime dt) const{
    int current_trip/*, step*/,
            upper =  route.nbTrips/*, count = upper*/,
            first = 0;

    if(data.pt_data.validity_patterns[route.vp].check(dt.date())) {
        //Recherche dichotomique du premier trip avant après dt.hour
        current_trip = upper;
        while ((upper - first) >1) {
            current_trip = first + (upper - first) / 2;
            int current_stop_time = route.firstStopTime + order + current_trip*route.nbStops;
            if (data.dataRaptor.stopTimes[current_stop_time].arrival_time > dt.hour()) {
                upper = current_trip;
            }
            else first = current_trip;
        }
        if(data.dataRaptor.stopTimes[route.firstStopTime + order + first*route.nbStops].arrival_time > dt.hour())
            --first;

        if(first != upper && first >= 0)
            return first;
    }

    //Si on en a pas trouvé, on cherche le lendemain
    dt.date_decrement();
    if(data.pt_data.validity_patterns[route.vp].check(dt.date())) {
        return route.nbTrips - 1;
    }

    //Cette route ne comporte aucun trip compatible
    return -1;
}



void RAPTOR::make_queue() {
    Q.assign(data.dataRaptor.routes.size(), std::numeric_limits<int>::max());

    auto it = data.dataRaptor.sp_routeorder_const.begin();
    int last = 0, last_index = 0;
    auto it_index = data.dataRaptor.sp_indexrouteorder.begin();
    for(auto stop = marked_sp.find_first(); stop != marked_sp.npos; stop = marked_sp.find_next(stop)) {
        advance(it_index, stop - last_index);
        last_index = stop;

        advance(it, it_index->first - last);
        last = it_index->first + it_index->second;
        const auto end = it+it_index->second;
        for(; it!=end; ++it) {
            if(vp_valides.test(data.dataRaptor.routes[it->first].vp) && (Q[it->first] > it->second))
                Q[it->first] = it->second;
        }
    }
    marked_sp.reset();
}



void RAPTOR::make_queuereverse() {
    Q.assign(data.dataRaptor.routes.size(), std::numeric_limits<int>::min());
    auto it = data.dataRaptor.sp_routeorder_const_reverse.begin();
    int last = 0, last_index = 0;
    auto it_index = data.dataRaptor.sp_indexrouteorder_reverse.begin();
    for(auto stop = marked_sp.find_first(); stop != marked_sp.npos; stop = marked_sp.find_next(stop)) {
        advance(it_index, stop - last_index);
        last_index = stop;

        advance(it, it_index->first - last);
        last = it_index->first + it_index->second;
        const auto end = it+it_index->second;
        for(; it!=end; ++it) {
            if(vp_valides.test(data.dataRaptor.routes[it->first].vp) && (Q[it->first] < it->second))
                Q[it->first] = it->second;
        }
    }
    marked_sp.reset();
}

void RAPTOR::marcheapied() {

    auto it = data.dataRaptor.foot_path.begin();
    int last = 0;
    for(auto stop_point = marked_sp.find_first(); stop_point != marked_sp.npos; stop_point = marked_sp.find_next(stop_point)) {
        const auto & index = data.dataRaptor.footpath_index[stop_point];
        const type_retour & retour_temp = retour[count][stop_point];
        int prec_duration = -1;
        DateTime dtTemp = retour_temp.dt;
        std::advance(it, index.first - last);
        const auto end = it + index.second;

        for(; it != end; ++it) {
            navitia::type::idx_t destination = (*it).destination_stop_point_idx;
            if(stop_point != destination) {
                if((*it).duration != prec_duration) {
                    dtTemp = retour_temp.dt + (*it).duration;
                    prec_duration = (*it).duration;
                }
                if(dtTemp < best[destination].dt) {
                    const type_retour nRetour = type_retour(navitia::type::invalid_idx, stop_point, dtTemp, connection);
                    best[destination] = nRetour;
                    retour[count][destination] = nRetour;
                    b_dest.ajouter_best(destination, nRetour, count);
                    marked_sp.set(destination);
                }
            }
        }
        last = index.first + index.second;
    }
}


void RAPTOR::marcheapiedreverse(unsigned int count) {
    auto it = data.dataRaptor.foot_path.begin();
    int last = 0;
    for(auto stop_point = marked_sp.find_first(); stop_point != marked_sp.npos; stop_point = marked_sp.find_next(stop_point)) {
        const auto & index = data.dataRaptor.footpath_index[stop_point];
        std::advance(it, index.first - last);
        const auto end = it + index.second;
        for(; it != end; ++it) {
            if(stop_point != (*it).destination_stop_point_idx) {
                const type_retour & retour_temp = retour[count][stop_point];
                const DateTime dtTemp = retour_temp.dt - (*it).duration;
                if(dtTemp > best[(*it).destination_stop_point_idx].dt) {
                    const type_retour nRetour = type_retour(navitia::type::invalid_idx, stop_point, dtTemp, connection);
                    best[(*it).destination_stop_point_idx] = nRetour;
                    retour[count][(*it).destination_stop_point_idx] = nRetour;
                    b_dest.ajouter_best_reverse((*it).destination_stop_point_idx, nRetour, count);
                    marked_sp.set((*it).destination_stop_point_idx);
                }
            }
        }
        last = index.first + index.second;
    }
}


Path RAPTOR::compute_raptor(vector_idxretour departs, vector_idxretour destinations, senscompute sens) {
    type_retour tr;
    if(sens == arriveravant || sens == arriveravantrab)
        tr.dt = DateTime::min;
    else
        tr.dt = DateTime::inf;

    if((sens == arriveravant) || (sens == arriveravantrab))
        departs.swap(destinations);


    best.assign(data.pt_data.stop_points.size(), tr);
    b_dest.reinit();
    if((sens == arriveravant) || (sens == arriveravantrab))
         b_dest.reverse();
    //    best.reserve(sizeof(type_retour) * data.pt_data.stop_areas.size());
    std::vector<unsigned int> marked_stop;

    if((sens == arriveravant) || (sens == arriveravantrab))
        retour.assign(20, data.dataRaptor.retour_constant_reverse);
    else
        retour.assign(20, data.dataRaptor.retour_constant);

    for(auto & item : departs) {
        retour[0][item.first] = item.second;
        best[item.first] = item.second;
        marked_stop.push_back(item.first);
    }


    for(auto & item : destinations) {
        b_dest.ajouter_destination(item.first, item.second);
    }

    if((sens == partirapres) || (sens == partirapresrab)) {
        setVPValides(marked_stop);
        boucleRAPTOR(marked_stop);
    } else {
        setVPValidesreverse(marked_stop);
        boucleRAPTORreverse(marked_stop);
    }


    if(b_dest.best_now.type != uninitialized) {
        unsigned int destination_idx = b_dest.best_now_spid;
        if(sens == partirapres)
            return makeBestPath(retour, best, departs, destination_idx, b_dest.count);
        else if(sens == arriveravant)
            return makeBestPathreverse(retour, best, departs, destination_idx, b_dest.count);
    } else {
        Path result;
        return result;
    }

    if(sens == partirapresrab)
        tr.dt = DateTime::min;
    else
        tr.dt = DateTime::inf;

    std::reverse(retour.begin(), retour.begin() + b_dest.count + 1);
    //On remplace les infinis par des minimums
    for(auto it = retour.begin(); it != retour.begin() + b_dest.count +1 ; ++it) {
        std::replace_if(it->begin(), it->end(), [](type_retour t){return t.type == uninitialized ;}, tr);
    }
    for(auto it = retour.begin() + b_dest.count + 1; it != retour.end(); ++it) {
        if(sens == partirapresrab)
            *it = data.dataRaptor.retour_constant_reverse;
        else if(sens == arriveravantrab)
            *it = data.dataRaptor.retour_constant;
    }

    marked_stop.clear();
    marked_stop.push_back(b_dest.best_now_spid);
    b_dest.reinit();

    if(sens == partirapresrab)
        b_dest.reverse();

    for(auto & item : departs) {
        b_dest.ajouter_destination(item.first, item.second);
    }

    if(sens == partirapresrab)
        best.assign(data.pt_data.stop_points.size(), tr);
    else if(sens == arriveravantrab)
        best.assign(data.pt_data.stop_points.size(), type_retour());

    if(sens == partirapresrab)
        boucleRAPTORreverse(marked_stop);
    else if(sens == arriveravantrab)
        boucleRAPTOR(marked_stop);

    if(b_dest.best_now.type != uninitialized) {
        unsigned int destination_idx = b_dest.best_now_spid;
        if(sens == partirapresrab)
            return makeBestPathreverse(retour, best, destinations, destination_idx, b_dest.count);
        else
            return makeBestPath(retour, best, destinations, destination_idx, b_dest.count);
    }



    Path result;
    return result;
}

std::vector<Path> RAPTOR::compute_all(vector_idxretour departs, vector_idxretour destinations) {
    std::vector<Path> result;
    std::vector<unsigned int> marked_stop;

    retour.assign(20, data.dataRaptor.retour_constant);
    best.assign(data.pt_data.stop_points.size(), type_retour());
    b_dest.reinit();
    for(auto item : departs) {
        retour[0][item.first] = item.second;
        best[item.first] = item.second;
        marked_stop.push_back(item.first);
    }

    for(auto item : destinations) {
        b_dest.ajouter_destination(item.first, item.second);
    }

    count = 1;

    setVPValides(marked_stop);
    boucleRAPTOR(marked_stop);

    if(b_dest.best_now.type != uninitialized) {
        auto temp = makePathes(retour, best, departs, b_dest, count);
        result.insert(result.begin(), temp.begin(), temp.end());
    } else {
        return result;
    }

    std::reverse(retour.begin(), retour.begin() + b_dest.count + 1);

    type_retour m;
    m.dt = DateTime::min;
    //On remplace les infinis par des minimums
    for(auto it = retour.begin(); it != retour.begin() + b_dest.count +1 ; ++it)
        std::replace_if(it->begin(), it->end(), [](type_retour t){return t.type == uninitialized ;}, m);
    for(auto it = retour.begin() + b_dest.count + 1; it != retour.end(); ++it)
        *it = data.dataRaptor.retour_constant_reverse;

    marked_stop.clear();
    marked_stop.push_back(b_dest.best_now_spid);
    b_dest.reinit();
    b_dest.reverse();

    boucleRAPTORreverse(marked_stop);

    if(b_dest.best_now.type != uninitialized){
        auto temp = makePathesreverse(retour, best, destinations, b_dest, count);
        result.insert(result.end(), temp.begin(), temp.end());
        return result;
    }

    return result;
}

 std::vector<Path> RAPTOR::compute_reverse_all(vector_idxretour departs, vector_idxretour destinations) {
     std::vector<Path> result;
     type_retour min;
     min.dt = DateTime::min;
     best.assign(data.pt_data.stop_points.size(), min);
     b_dest.reinit();
     b_dest.reverse();
     std::vector<unsigned int> marked_stop;

     retour.assign(20, data.dataRaptor.retour_constant_reverse);
     for(auto & item : destinations) {
         retour[0][item.first] = item.second;
         best[item.first] = item.second;
         marked_stop.push_back(item.first);
     }

     for(auto & item : departs) {
         b_dest.ajouter_destination(item.first, item.second);
     }

     setVPValidesreverse(marked_stop);
     boucleRAPTORreverse(marked_stop);

     if(b_dest.best_now.type != uninitialized) {
         auto temp = makePathesreverse(retour, best, destinations, b_dest, count);
         result.insert(result.begin(), temp.begin(), temp.end());
     } else {
         return result;
     }

     std::reverse(retour.begin(), retour.begin() + b_dest.count + 1);

     type_retour m;
     m.dt = DateTime::inf;
     //On remplace les minimums par des infinis
     for(auto it = retour.begin(); it != retour.begin() + b_dest.count +1 ; ++it)
         std::replace_if(it->begin(), it->end(), [](type_retour t){return t.type == uninitialized ;}, m);
     for(auto it = retour.begin() + b_dest.count + 1; it != retour.end(); ++it)
         *it = data.dataRaptor.retour_constant;

     marked_stop.clear();
     marked_stop.push_back(b_dest.best_now_spid);
     b_dest.reinit();

     boucleRAPTOR(marked_stop);

     if(b_dest.best_now.type != uninitialized){
         auto temp = makePathes(retour, best, departs, b_dest, count);
         result.insert(result.end(), temp.begin(), temp.end());
     }
     return result;
 }

void RAPTOR::setVPValides(const std::vector<unsigned int> &marked_stop) {
    //On cherche la premiere date
    DateTime dt = DateTime::inf;
    for(unsigned int spid : marked_stop) {
        if(retour[0][spid].dt < dt)
            dt = retour[0][spid].dt;
    }
    uint32_t date = dt.date();
    if(dt.date() == std::numeric_limits<uint32_t>::max()) {
        for(auto pair : data.dataRaptor.sp_routeorder_const)
            sp_routeorder.push_back(std::make_pair(pair.first, std::numeric_limits<int>::max()));
    } else {

        for(auto vp : data.pt_data.validity_patterns) {
            vp_valides.set(vp.idx, vp.check2(date));
        }
    }


}

void RAPTOR::setRoutesValides(const std::vector<unsigned int> &marked_stop) {

    //On cherche la premiere date
    DateTime dt = DateTime::inf;
    for(unsigned int spid : marked_stop) {
        if(retour[0][spid].dt < dt)
            dt = retour[0][spid].dt;
    }

    uint32_t date = dt.date();
    if(date == std::numeric_limits<uint32_t>::max()) {
        for(auto vp : data.pt_data.validity_patterns) {
            vp_valides.set(vp.idx, false);
        }
    } else {
        //On cherche les vp valides
        boost::dynamic_bitset<> vp_valides(data.pt_data.validity_patterns.size());

        for(auto vp : data.pt_data.validity_patterns) {
            vp_valides.set(vp.idx, vp.check2(date));
        }

        //On active les routes
        sp_routeorder = data.dataRaptor.sp_routeorder_const;
        for(auto & pair : sp_routeorder) {
            const auto & route = data.dataRaptor.routes[pair.first];
            if(!vp_valides.test(route.vp)) {
                pair.second = std::numeric_limits<uint32_t>::max();
            }
        }
    }
}

void RAPTOR::setRoutesValidesreverse(std::vector<unsigned int> &marked_stop) {
    //On cherche la premiere date
    DateTime dt = DateTime::min;
    for(unsigned int spid : marked_stop) {
        if(retour[0][spid].dt > dt)
            dt = retour[0][spid].dt;
    }

    uint32_t date = dt.date();
    if(date > 0) {
        //On cherche les vp valides
        boost::dynamic_bitset<> vp_valides(data.pt_data.validity_patterns.size());

        for(auto vp : data.pt_data.validity_patterns) {
            vp_valides.set(vp.idx, vp.check2(date-1));
        }

        for(auto & pair : sp_routeorder) {
            const auto & route = data.dataRaptor.routes[pair.first];
            if(!vp_valides.test(route.vp)) {
                pair.second = std::numeric_limits<uint32_t>::max();
            }
        }
    } else if(date == 0) {


        //On cherche les vp valides
        boost::dynamic_bitset<> vp_valides(data.pt_data.validity_patterns.size());

        for(auto vp : data.pt_data.validity_patterns) {
            vp_valides.set(vp.idx, vp.check(date));
        }

        for(auto & pair : sp_routeorder) {
            const auto & route = data.dataRaptor.routes[pair.first];
            if(!vp_valides.test(route.vp)) {
                pair.second = std::numeric_limits<uint32_t>::max();
            }
        }
    } else {
        for(auto pair : data.dataRaptor.sp_routeorder_const)
            sp_routeorder.push_back(std::make_pair(pair.first, std::numeric_limits<int>::max()));
    }
}

void RAPTOR::setVPValidesreverse(std::vector<unsigned int> &marked_stop) {

    //On cherche la premiere date
    DateTime dt = DateTime::min;
    for(unsigned int spid : marked_stop) {
        if(retour[0][spid].dt > dt)
            dt = retour[0][spid].dt;
    }

    uint32_t date = dt.date();
    //On active les routes
    sp_routeorder = data.dataRaptor.sp_routeorder_const_reverse;
    if(date > 0) {
        //On cherche les vp valides
        for(auto vp : data.pt_data.validity_patterns) {
            vp_valides.set(vp.idx, vp.check2(date-1));
       }
    } else if(date == 0) {
        //On cherche les vp valides

        for(auto vp : data.pt_data.validity_patterns) {
            vp_valides.set(vp.idx, vp.check(date));
        }

        for(auto & pair : sp_routeorder) {
            const auto & route = data.dataRaptor.routes[pair.first];
            if(!vp_valides.test(route.vp)) {
                pair.second = std::numeric_limits<uint32_t>::max();
            }
        }
    } else {
        for(auto vp : data.pt_data.validity_patterns) {
            vp_valides.set(vp.idx, false);
        }
    }
}



void RAPTOR::boucleRAPTOR(const std::vector<unsigned int> &marked_stop) {
    unsigned int routeidx;
    int t;
    bool end = false;
    count = 0;


    for(auto said : marked_stop) {
        marked_sp.set(said, true);
    }


    marcheapied();

    while(!end) {
        ++count;
        end = true;
        make_queue();
        routeidx = 0;
        if(count == retour.size())
            retour.push_back(data.dataRaptor.retour_constant);
        const map_int_pint_t & prec_retour = retour[count -1];
        for(queue_t::value_type vq : Q) {
            t = -1;
            DateTime workingDt;
            int embarquement = -1;
            const auto & route = data.dataRaptor.routes[routeidx];
            for(int i = vq; i < route.nbStops; ++i) {
                int spid = data.pt_data.route_points[data.pt_data.routes[route.idx].route_point_list[i]].stop_point_idx;
                if(t >= 0) {
                    const auto & st = data.dataRaptor.stopTimes[data.dataRaptor.get_stop_time_idx(route, t, i)];
                    workingDt.update(st.arrival_time);
                    //On stocke, et on marque pour explorer par la suite
                    if(workingDt < std::min(best[spid].dt, b_dest.best_now.dt)) {
                        const type_retour retour_temp = type_retour(st.idx, embarquement, workingDt);
                        retour[count][spid] = retour_temp;
                        best[spid]          = retour_temp;
                        b_dest.ajouter_best(spid, retour_temp, count);
                        marked_sp.set(spid);
                        end = false;
                    }
                }

                //Si on peut arriver plus tôt à l'arrêt en passant par une autre route
                const type_retour & retour_temp = prec_retour[spid];

                if((retour_temp.type != uninitialized) &&
                        (((retour_temp.dt.hour() <= data.dataRaptor.get_temps_depart(route, t, i)) && (retour_temp.dt.date() == workingDt.date()) ) ||
                         ((retour_temp.dt.date() < workingDt.date()) ))) {
                    DateTime dt =  retour_temp.dt;
                    if(retour_temp.type == vj)
                        dt = dt + 120;

                    int etemp = earliest_trip(route, i, dt);

                    if(etemp >= 0) {
                        t = etemp;
                        workingDt = retour_temp.dt;
                        embarquement = spid;
                        workingDt.update(data.dataRaptor.get_temps_depart(route, t,i));
                    }
                }
            }

            ++routeidx;
        }
        marcheapied();
    }
}


void RAPTOR::boucleRAPTORreverse(std::vector<unsigned int> &marked_stop) {
    unsigned int routeidx;
    int t;

    marked_sp.reset();
    for(auto rpid : marked_stop) {
        marked_sp.set(rpid);
    }

    count = 0;

    marcheapiedreverse(0);



    bool end = false;
    while(!end) {
        end = true;
        if(count == retour.size())
            retour.push_back(data.dataRaptor.retour_constant_reverse);
        make_queuereverse();
        routeidx = 0;
        ++count;
        for(queue_t::value_type vq : Q) {
            t = -1;
            DateTime workingDt = DateTime::min;
            int embarquement = navitia::type::invalid_idx;
            const auto & route = data.dataRaptor.routes[routeidx];
            for(int i = vq; i >=0; --i) {
                int spid = data.pt_data.route_points[data.pt_data.routes[route.idx].route_point_list[i]].stop_point_idx;

                if(t >= 0) {
                    const auto & st = data.dataRaptor.stopTimes[data.dataRaptor.get_stop_time_idx(route, t, i)];
                    workingDt.updatereverse(st.departure_time);
                    //On stocke, et on marque pour explorer par la suite
                    if(workingDt >= std::max(best[spid].dt, b_dest.best_now.dt)) {
                        const type_retour retour_temp = type_retour(st.idx, embarquement, workingDt);
                        retour[count][spid] = retour_temp;
                        best[spid]          = retour_temp;
                        if(!b_dest.ajouter_best_reverse(spid, retour_temp, count)) {
                            marked_sp.set(spid);
                            end = false;
                        }

                    }
                }

                //Si on peut arriver plus tôt à l'arrêt en passant par une autre route
                const type_retour & retour_temp = retour[count-1][spid];
                uint32_t temps_arrivee = data.dataRaptor.get_temps_arrivee(route, t, i);
                if((retour_temp.type != uninitialized) &&
                        ( (temps_arrivee == std::numeric_limits<uint32_t>::max()) ||
                          ((retour_temp.dt.hour() >= temps_arrivee) && (retour_temp.dt.date() == workingDt.date()) ) ||
                          ((retour_temp.dt.date() > workingDt.date()) ))) {
                    int etemp = tardiest_trip(route, i, retour_temp.dt);
                    if(etemp >=0 && t!=etemp) {
                        if(t!=etemp) {
                            embarquement = spid;
                        }
                        t = etemp;
                        workingDt = retour_temp.dt;
                    }
                }
            }
            ++routeidx;


        }
        marcheapiedreverse(count);
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
        return makePathreverse(retour, best, departs, destination_idx, count);
}

std::vector<Path> RAPTOR::makePathes(map_retour_t &retour, map_int_pint_t &best, vector_idxretour departs, best_dest &b_dest, unsigned int count) {
    std::vector<Path> result;

    for(unsigned int i=1;i<count;++i) {
        DateTime dt;
        int spid = std::numeric_limits<int>::max();
        for(auto dest : b_dest.map_date_time) {
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
        for(auto dest : b_dest.map_date_time) {
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
    unsigned int current_spid = destination_idx;
    type_retour r = retour[countb][current_spid];
    DateTime workingDate = r.dt;
    navitia::type::StopTime current_st;
    navitia::type::idx_t spid_embarquement = navitia::type::invalid_idx;

    bool stop = false;
    for(auto item : departs) {
        stop = stop || item.first == current_spid;
    }

    result.nb_changes = countb - 1;
    PathItem item;
    // On boucle tant
    while(!stop) {

        // Est-ce qu'on a une section marche à pied ?
        if(retour[countb][current_spid].type == connection) {
            r = retour[countb][current_spid];
            workingDate = r.dt;
            workingDate.normalize();

            if(!reverse)
                item = PathItem(retour[countb][r.said_emarquement].dt, workingDate);
            else
                item = PathItem(workingDate, retour[countb][r.said_emarquement].dt);

            item.stop_points.push_back(current_spid);
            item.type = walking;
            item.stop_points.push_back(r.said_emarquement);
            result.items.push_back(item);

            spid_embarquement = navitia::type::invalid_idx;
            current_spid = r.said_emarquement;

        } else { // Sinon c'est un trajet TC
            // Est-ce que qu'on a affaire à un nouveau trajet ?
            if(spid_embarquement == navitia::type::invalid_idx) {
                r = retour[countb][current_spid];
                spid_embarquement = r.said_emarquement;
                current_st = data.pt_data.stop_times.at(r.stid);
                workingDate = r.dt;

                item = PathItem();
                item.type = public_transport;

                if(!reverse)
                    item.arrival = workingDate;
                else
                    item.departure = DateTime(workingDate.date(), current_st.departure_time);

                while(spid_embarquement != current_spid) {
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
                    item.stop_points.push_back(current_spid);
                    item.vj_idx = current_st.vehicle_journey_idx;

                    current_spid = data.pt_data.route_points[current_st.route_point_idx].stop_point_idx;
                    for(auto item : departs) {
                        stop = stop || item.first == current_spid;
                    }
                    if(stop)
                        break;
                }
                item.stop_points.push_back(current_spid);

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
            stop = stop || item.first == current_spid;
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


Path RAPTOR::compute(idx_t departure_idx, idx_t destination_idx, int departure_hour, int departure_day, senscompute sens) {
    vector_idxretour departs, destinations;

    for(navitia::type::idx_t spidx : data.pt_data.stop_areas[departure_idx].stop_point_list) {
        departs.push_back(std::make_pair(spidx, type_retour(navitia::type::invalid_idx, DateTime(departure_day, departure_hour), depart)));
    }

    for(navitia::type::idx_t spidx : data.pt_data.stop_areas[destination_idx].stop_point_list) {
        destinations.push_back(std::make_pair(spidx, type_retour(navitia::type::invalid_idx, DateTime(departure_day, departure_hour), depart)));
    }

    return compute_raptor(departs, destinations, sens);

//    if(sens == partirapres)
//        return compute_raptor(departs, destinations);
//    else if(sens == arriveravant)
//        return compute_raptor_reverse(departs, destinations);
//    else
//        return compute_raptor_rabattement(departs, destinations);
}

Path RAPTOR::compute_reverse(idx_t departure_idx, idx_t destination_idx, int departure_hour, int departure_day) {
    vector_idxretour departs, destinations;

    for(navitia::type::idx_t spidx : data.pt_data.stop_areas[destination_idx].stop_point_list) {
        departs.push_back(std::make_pair(spidx, type_retour(navitia::type::invalid_idx, DateTime(departure_day, departure_hour))));

    }


    for(navitia::type::idx_t spidx : data.pt_data.stop_areas[departure_idx].stop_point_list) {
        destinations.push_back(std::make_pair(spidx, type_retour()));

    }

    return compute_raptor(departs, destinations, arriveravant);
}

Path RAPTOR::compute_rabattement(idx_t departure_idx, idx_t destination_idx, int departure_hour, int departure_day) {
    vector_idxretour departs, destinations;

    for(navitia::type::idx_t spidx : data.pt_data.stop_areas[departure_idx].stop_point_list) {
        departs.push_back(std::make_pair(spidx, type_retour(navitia::type::invalid_idx, DateTime(departure_day, departure_hour), depart)));
    }


    for(navitia::type::idx_t spidx : data.pt_data.stop_areas[destination_idx].stop_point_list) {
        destinations.push_back(std::make_pair(spidx, type_retour()));
    }


    return compute_raptor(departs, destinations, partirapresrab);
}

std::vector<Path> RAPTOR::compute_all(navitia::type::EntryPoint departure, navitia::type::EntryPoint destination, int departure_hour, int departure_day, senscompute sens) {
    vector_idxretour departs, destinations;

    DateTime dt_depart(departure_day, departure_hour);
    //Gestion des départs
    switch(departure.type) {
    case navitia::type::Type_e::eStopArea:
    {
        auto it_departure = data.pt_data.stop_area_map.find(departure.external_code);
        if(it_departure == data.pt_data.stop_area_map.end()) {
            return std::vector<Path>();
        } else {
            for(auto spidx : data.pt_data.stop_areas[it_departure->second].stop_point_list) {
                departs.push_back(std::make_pair(spidx, type_retour(navitia::type::invalid_idx, dt_depart, 0)));
            }
        }
    }
        break;
    case type::Type_e::eStopPoint:
    {
        auto it_departure = data.pt_data.stop_point_map.find(departure.external_code);
        if(it_departure == data.pt_data.stop_point_map.end()) {
            return std::vector<Path>();
        } else {
            departs.push_back(std::make_pair(data.pt_data.stop_points[it_departure->second].idx, type_retour(navitia::type::invalid_idx, DateTime(departure_day, departure_hour),0)));
        }
    }
        break;
    case type::Type_e::eCoord:

        departs = trouverGeo(departure.coordinates, 300, departure_hour, departure_day);
        break;
    default:
       return std::vector<Path>();
    }

    //Gestion des arrivées
    switch(destination.type) {
    case navitia::type::Type_e::eStopArea:
    {
        auto it_destination = data.pt_data.stop_area_map.find(destination.external_code);
        if(it_destination == data.pt_data.stop_area_map.end()) {
            return std::vector<Path>();
        } else {
            for(auto spidx : data.pt_data.stop_areas[it_destination->second].stop_point_list) {
                destinations.push_back(std::make_pair(spidx, type_retour(navitia::type::invalid_idx, dt_depart, 0)));
            }
        }
     }
        break;
    case type::Type_e::eStopPoint:
    {
        auto it_destination =data.pt_data.stop_point_map.find(destination.external_code);
        if(it_destination == data.pt_data.stop_point_map.end()) {
            return std::vector<Path>();
        } else {
            destinations.push_back(std::make_pair(data.pt_data.stop_points[it_destination->second].idx, type_retour(navitia::type::invalid_idx, DateTime(departure_day, departure_hour),0)));
        }

    }
        break;
    case type::Type_e::eCoord:
        destinations = trouverGeo(destination.coordinates, 300, departure_hour, departure_day);
        break;
    default:
        return std::vector<Path>();

    }

    std::cout << "Nombre de départs : " << departs.size() << " destinations : " << destinations.size() << std::endl;
    if(sens == partirapres)
        return compute_all(departs, destinations);
    else
        return compute_reverse_all(departs, destinations);
}


vector_idxretour RAPTOR::trouverGeo(const type::GeographicalCoord & departure, const double radius_depart,  const int departure_hour, const int departure_day) const{
    typedef std::vector< std::pair<idx_t, double> >   retour_prox;

    vector_idxretour toReturn;
    retour_prox prox;
    try {
        prox = (retour_prox) (data.street_network.find_nearest(departure, data.pt_data.stop_point_proximity_list, radius_depart));
    } catch(navitia::proximitylist::NotFound) {std::cout << "Not found 1 " << std::endl; return toReturn;}


    for(auto item : prox) {
        int temps = departure_hour + (item.second / 80);
        int day;
        if(temps > 86400) {
            temps = temps % 86400;
            day = departure_day + 1;
        } else {
            day = departure_day;
        }
        toReturn.push_back(std::make_pair(item.first, type_retour(navitia::type::invalid_idx, DateTime(day, temps), 0, (item.second / 80))));
    }

    return toReturn;
}



}}}
