#include "raptor.h"
namespace navitia { namespace routing { namespace raptor {


    int RAPTOR::earliest_trip(const type::Route & route, const unsigned int order, const DateTime &dt) const{
        //On cherche le plus petit stop time de la route >= dt.hour()
        std::vector<uint32_t>::const_iterator begin = data.dataRaptor.departure_times.begin() + 
                     data.dataRaptor.first_stop_time[route.idx] + 
                     order * data.dataRaptor.nb_trips[route.idx];
        std::vector<uint32_t>::const_iterator end = begin + data.dataRaptor.nb_trips[route.idx];


        auto it = std::lower_bound(begin, end, dt.hour(),
                                    [](uint32_t departure_time, uint32_t hour){
                                    return departure_time < hour;});
        int idx = it - data.dataRaptor.departure_times.begin();
        auto date = dt.date();

        //On renvoie le premier trip valide
        for(; it != end; ++it) {
            if(data.dataRaptor.validity_patterns[data.dataRaptor.vp_idx_forward[idx]].test(date))
                return data.pt_data.stop_times[data.dataRaptor.st_idx_forward[idx]].vehicle_journey_idx;
            ++idx;
        }

        //Si on en a pas trouvé, on cherche le lendemain
        
        ++date;
        idx = begin - data.dataRaptor.departure_times.begin();
        for(it = begin; it != end; ++it) {
            if(data.dataRaptor.validity_patterns[data.dataRaptor.vp_idx_forward[idx]].test(date))
                return data.pt_data.stop_times[data.dataRaptor.st_idx_forward[idx]].vehicle_journey_idx;
            ++idx;
        }

        //Cette route ne comporte aucun trip compatible
        return -1;
    }










    int RAPTOR::tardiest_trip(const type::Route & route, const unsigned int order, const DateTime &dt) const{
        //On cherche le plus grand stop time de la route <= dt.hour()
        const auto begin = data.dataRaptor.arrival_times.begin() +  
            data.dataRaptor.first_stop_time[route.idx] + 
            order * data.dataRaptor.nb_trips[route.idx];
        const auto end = begin + data.dataRaptor.nb_trips[route.idx];

        auto it = std::lower_bound(begin, end, dt.hour(),
                [](uint32_t arrival_time, uint32_t hour){
                return arrival_time > hour;}
                );

        int idx = it - data.dataRaptor.arrival_times.begin();
        auto date = dt.date();
        //On renvoie le premier trip valide
        for(; it != end; ++it) {
            if(data.dataRaptor.validity_patterns[data.dataRaptor.vp_idx_backward[idx]].test(date))
                return data.pt_data.stop_times[data.dataRaptor.st_idx_backward[idx]].vehicle_journey_idx;
            ++idx;
        }


        //Si on en a pas trouvé, on cherche la veille
        if(date > 0) {
            --date;
            idx = begin - data.dataRaptor.arrival_times.begin();
            for(it = begin; it != end; ++it) {
                if(data.dataRaptor.validity_patterns[data.dataRaptor.vp_idx_backward[idx]].test(date))
                    return data.pt_data.stop_times[data.dataRaptor.st_idx_backward[idx]].vehicle_journey_idx;
                ++idx;
            }
        }


        //Cette route ne comporte aucun trip compatible
        return -1;
    }









    void RAPTOR::make_queue() {
        Q.assign(data.pt_data.routes.size(), std::numeric_limits<int>::max());

        auto it = data.dataRaptor.sp_routeorder_const.begin();
        int last = 0;
        for(auto stop = marked_sp.find_first(); stop != marked_sp.npos; stop = marked_sp.find_next(stop)) {
            auto index = data.dataRaptor.sp_indexrouteorder[stop];

            it += index.first - last;
            last = index.first + index.second;
            const auto end = it+index.second;
            for(; it!=end; ++it) {
                if((it->second < Q[it->first]))
                    Q[it->first] = it->second;
            }
            marked_sp.flip(stop);
        }
    }









    void RAPTOR::make_queuereverse() {
        Q.assign(data.pt_data.routes.size(), std::numeric_limits<int>::min());
        auto it = data.dataRaptor.sp_routeorder_const_reverse.begin();
        int last = 0;
        for(auto stop = marked_sp.find_first(); stop != marked_sp.npos; stop = marked_sp.find_next(stop)) {
            auto index = data.dataRaptor.sp_indexrouteorder_reverse[stop];

            it += index.first - last;
            last = index.first + index.second;
            const auto end = it+index.second;
            for(; it!=end; ++it) {
                if((it->second > Q[it->first]))
                    Q[it->first] = it->second;
            }
            marked_sp.flip(stop);
        }
    }










    void RAPTOR::marcheapied() {

        auto it = data.dataRaptor.foot_path.begin();
        int last = 0;
        for(auto stop_point = marked_sp.find_first(); stop_point != marked_sp.npos; stop_point = marked_sp.find_next(stop_point)) {
            const auto & index = data.dataRaptor.footpath_index[stop_point];
            const type_retour & retour_temp = retour[count][stop_point];
            int prec_duration = -1;
            DateTime dtTemp = retour_temp.dt;
            //std::advance(it, index.first - last);
            it += index.first - last;
            const auto end = it + index.second;

            for(; it != end; ++it) {
                navitia::type::idx_t destination = (*it).destination_stop_point_idx;
                if(stop_point != destination) {
                    if(it->duration != prec_duration) {
                        dtTemp = retour_temp.dt + it->duration;
                        prec_duration = it->duration;
                    }
                    if(dtTemp <= best[destination].dt) {
                        const type_retour nRetour = type_retour(navitia::type::invalid_idx, stop_point, dtTemp, connection);
                        if(!b_dest.ajouter_best(destination, nRetour, count)) {
                            best[destination] = nRetour;
                            retour[count][destination] = nRetour;
                            marked_sp.set(destination);
                        } else if(count == 0 || retour[count - 1][destination].type == uninitialized) {
                            retour[count][destination] = nRetour;
                        }
                    }
                }
            }
            last = index.first + index.second;
        }
    }








    void RAPTOR::marcheapiedreverse() {
        auto it = data.dataRaptor.foot_path.begin();
        int last = 0;
        for(auto stop_point = marked_sp.find_first(); stop_point != marked_sp.npos; stop_point = marked_sp.find_next(stop_point)) {
            const auto & index = data.dataRaptor.footpath_index[stop_point];
            const type_retour & retour_temp = retour[count][stop_point];
            int prec_duration = -1;
            DateTime dtTemp = retour_temp.dt;
            it += index.first - last;
            const auto end = it + index.second;
            for(; it != end; ++it) {
                navitia::type::idx_t destination = it->destination_stop_point_idx;
                if(stop_point != destination) {
                    if(it->duration != prec_duration) {
                        dtTemp = retour_temp.dt - it->duration;
                        prec_duration = it->duration;
                    }
                    if(dtTemp >= best[destination].dt) {
                        const type_retour nRetour = type_retour(navitia::type::invalid_idx, stop_point, dtTemp, connection);
                        if(!b_dest.ajouter_best_reverse(destination, nRetour, count)) {
                            best[destination] = nRetour;
                            retour[count][destination] = nRetour;
                            marked_sp.set(destination);
                        } else if(count == 0 || retour[count - 1][destination].type == uninitialized) {
                            retour[count][destination] = nRetour;
                            best[destination] = nRetour;
                        }
                    }
                }
            }
            last = index.first + index.second;
        }
    }


    vector_idxretour 
    RAPTOR::to_idxretour(const std::vector<std::pair<type::idx_t, double> > &elements, const DateTime &dt, bool dep_or_dest){
        vector_idxretour result;
        for(auto item : elements) {
            DateTime dtemp = dt;
            if(dt != DateTime::min && dt != DateTime::inf) {
                dtemp = dtemp + (item.second / 80);
            }
            if(dep_or_dest)
                result.push_back(std::make_pair(item.first, 
                            type_retour(navitia::type::invalid_idx, dtemp, 
                                0, (item.second / 80))));
            else 
                result.push_back(std::make_pair(item.first,
                            type_retour(navitia::type::invalid_idx, dtemp,
                                (item.second / 80), 0)));
        }
        return result;
    }



    void RAPTOR::init(vector_idxretour departs, vector_idxretour destinations, bool clockwise, DateTime borne,
                      bool reset) {

        if(reset) {
            if(clockwise) {
                retour.assign(20, data.dataRaptor.retour_constant);
                best = data.dataRaptor.retour_constant;
                b_dest.reinit(borne);
            } else {
                retour.assign(20, data.dataRaptor.retour_constant_reverse);
                best = data.dataRaptor.retour_constant_reverse;
                if(borne == DateTime::inf)
                    borne = DateTime::min;
                b_dest.reinit(borne);
            }
        }

        if(!clockwise) {
            departs.swap(destinations);
        }

        for(auto item : departs) {
            retour[0][item.first] = item.second;
            retour[0][item.first].type = depart;
            best[item.first] = item.second;
            marked_sp.set(item.first);
        }

        for(auto item : destinations) {
            b_dest.ajouter_destination(item.first, item.second);
            best[item.first].dt = borne;
        }

        count = 1;
    }



    std::vector<Path> 
    RAPTOR::compute_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                        const std::vector<std::pair<type::idx_t, double> > &destinations,
                        const DateTime &dt_depart, const DateTime &borne) {
            std::vector<Path> result;

            init(to_idxretour(departs, dt_depart, true) , to_idxretour(destinations, DateTime::inf, false), true, borne);
            set_routes_valides();
            boucleRAPTOR();


            if(b_dest.best_now.type == uninitialized) {
                return result;
            }

            auto dep = std::make_pair(b_dest.best_now_spid, b_dest.best_now);
            if(dep.second.type == vj)
                dep.second.dt = DateTime(dep.second.dt.date(), data.pt_data.stop_times[b_dest.best_now.stid].departure_time);

            init(to_idxretour(departs, DateTime::min, true), {dep}, false, dt_depart);

            boucleRAPTORreverse();

            if(b_dest.best_now.type != uninitialized){
                auto temp = makePathesreverse(retour, best, destinations, b_dest, count);
                result.insert(result.end(), temp.begin(), temp.end());
                return result;
            }

        return result;
    }




    std::vector<Path> 
    RAPTOR::compute_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                        const std::vector<std::pair<type::idx_t, double> > &destinations,
                        std::vector<DateTime> dt_departs, const DateTime &borne) {
        std::vector<Path> result;
        std::vector<best_dest> bests;

        std::sort(dt_departs.begin(), dt_departs.end(), [](DateTime dt1, DateTime dt2) {return dt1 > dt2;});

        set_routes_valides();
        
        bool reset = true;
        for(auto dep : dt_departs) {
            init(to_idxretour(departs, dep, true) , to_idxretour(destinations, DateTime::inf, false), true, borne , reset);
            boucleRAPTOR();
            bests.push_back(b_dest);
            reset = false;
        }

        for(unsigned int i=0; i<dt_departs.size(); ++i) {
            auto b = bests[i];
            auto dt_depart = dt_departs[i];
            if(b.best_now.type != uninitialized) {
                auto dep = std::make_pair(b.best_now_spid, b.best_now);
                if(dep.second.type == vj)
                    dep.second.dt = DateTime(dep.second.dt.date(), data.pt_data.stop_times[b.best_now.stid].departure_time);
                init(to_idxretour(departs, DateTime::min, true), {dep}, false, dt_depart);
                boucleRAPTORreverse();
                if(b_dest.best_now.type != uninitialized){
                    auto temp = makePathesreverse(retour, best, destinations, b_dest, count);
                    auto path = temp.back();
                    path.request_time = boost::posix_time::ptime(data.meta.production_date.begin(),
                                                              boost::posix_time::seconds(dt_depart.hour()));
                    result.push_back(path);
                }
            }
        }

        return result;
    }




    std::vector<Path> 
    RAPTOR::compute_reverse_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                                const std::vector<std::pair<type::idx_t, double> > &destinations,
                                const DateTime &dt_depart, const DateTime &borne) {
        std::vector<Path> result;

        init(to_idxretour(departs, DateTime::min, false), to_idxretour(destinations, dt_depart, true), false, borne);
        set_routes_valides();
        boucleRAPTORreverse();

        if(b_dest.best_now.type == uninitialized) {
            return result;
        }

        DateTime dep;
        if(b_dest.best_now.type == vj)
            dep = DateTime(b_dest.best_now.dt.date(), data.pt_data.stop_times[b_dest.best_now.stid].departure_time);
        else
            dep = b_dest.best_now.dt;



        init(to_idxretour({std::make_pair(b_dest.best_now_spid, b_dest.best_now.dist_to_dep)}, dep, true),
             to_idxretour(destinations, DateTime::inf, false), true, dt_depart);

        boucleRAPTOR();

        if(b_dest.best_now.type != uninitialized){
            auto temp = makePathes(retour, best, departs, b_dest, count);
            result.insert(result.end(), temp.begin(), temp.end());
        }
        return result;
    }



    void RAPTOR::set_routes_valides() {
        //On cherche la premiere date
        DateTime dt = DateTime::min;
        for(auto spid = marked_sp.find_first(); spid != marked_sp.npos; spid = marked_sp.find_next(spid)) {
            if(retour[0][spid].dt > dt)
                dt = retour[0][spid].dt;
        }

        uint32_t date = dt.date();

        routes_valides.clear();
        routes_valides.resize(data.pt_data.routes.size());
        for(const auto & route : data.pt_data.routes) {
            for(auto vjidx : route.vehicle_journey_list) {
                if(data.pt_data.validity_patterns[data.pt_data.vehicle_journeys[vjidx].validity_pattern_idx].check2(date)) {
                    routes_valides.set(route.idx);
                    break;
                }
            }
        }
    }





    void RAPTOR::boucleRAPTOR() {
        bool end = false;
        count = 0;
        int t=-1, embarquement = -1;
        DateTime workingDt = DateTime::inf;
        marcheapied();

        while(!end) {
            ++count;
            end = true;
            make_queue();
            if(count == retour.size())
                retour.push_back(data.dataRaptor.retour_constant);
            const auto & prec_retour = retour[count -1];
            auto & working_retour = retour[count];
            for(const auto & route : data.pt_data.routes) {
                if(routes_valides.test(route.idx)) {
                    t = -1;
                    embarquement = -1;
                    workingDt = DateTime::inf;
                    std::vector<type::StopTime>::const_iterator it_st;
                    for(int i = Q[route.idx]; i < route.route_point_list.size(); ++i) {
                        int spid = data.pt_data.route_points[route.route_point_list[i]].stop_point_idx;
                        if(t >= 0) {
                            ++it_st;
                            workingDt.update(it_st->arrival_time);
                            //On stocke, et on marque pour explorer par la suite
                            const auto & min = std::min(best[spid].dt, b_dest.best_now.dt);
                            if(workingDt < min) {
                                working_retour[spid] = type_retour(it_st->idx, embarquement, workingDt);
                                best[spid]           = working_retour[spid] ;
                                if(!b_dest.ajouter_best(spid, working_retour[spid], count)) {
                                    marked_sp.set(spid);
                                    end = false;
                                }
                            } else if(workingDt == min &&
                                      prec_retour[spid].type == uninitialized) {
                                auto r = type_retour(it_st->idx, embarquement, workingDt);
                                if(b_dest.ajouter_best(spid, r, count))
                                    working_retour[spid] = r;
                            }
                        }

                        //Si on peut arriver plus tôt à l'arrêt en passant par une autre route
                        const type_retour & retour_temp = prec_retour[spid];
                        if((retour_temp.type != uninitialized) &&
                            (t == -1 || retour_temp.dt <= DateTime(workingDt.date(), it_st->departure_time))) {
                                DateTime dt = retour_temp.dt;

                            if(retour_temp.type == vj)
                                dt = dt + 120;

                            int etemp = earliest_trip(route, i, dt);
                            if(etemp >= 0) {
                                if(t != etemp) {
                                    t = etemp;
                                    embarquement = spid;
                                    it_st = data.pt_data.stop_times.begin() 
                                            + data.pt_data.vehicle_journeys[t].stop_time_list[i];
                                }   
                                workingDt = dt;
                            }
                        }
                    }
                }
            }
            marcheapied();
        }
    }


    void RAPTOR::boucleRAPTORreverse() {
        count = 0;

        DateTime workingDt = DateTime::min;
        int t=-1, embarquement = std::numeric_limits<int>::max();


        marcheapiedreverse();
        bool end = false;
        while(!end) {
            ++count;
            end = true;
            if(count == retour.size())
                retour.push_back(data.dataRaptor.retour_constant_reverse);
            const auto & prec_retour = retour[count -1];
            auto & working_retour = retour[count];
            make_queuereverse();
            for(const auto & route : data.pt_data.routes) {
                if(routes_valides.test(route.idx)) { 
                    t = -1;
                    workingDt = DateTime::min;
                    embarquement = std::numeric_limits<int>::max();
                    std::vector<type::StopTime>::const_iterator it_st;
                    for(int i = Q[route.idx]; i >=0; --i) {
                        int spid = data.pt_data.route_points[route.route_point_list[i]].stop_point_idx;

                        if(t >= 0) {
                            --it_st;
                            workingDt.updatereverse(it_st->departure_time);;
                            //On stocke, et on marque pour explorer par la suite
                            const auto & max = std::max(best[spid].dt, b_dest.best_now.dt);

                            if(workingDt > max) {
                                working_retour[spid] = type_retour(it_st->idx, embarquement, workingDt);
                                best[spid]           = working_retour[spid];
                                if(!b_dest.ajouter_best_reverse(spid, working_retour[spid], count)) {
                                    marked_sp.set(spid);
                                    end = false;
                                }
                            } else if(workingDt == max && 
                                      prec_retour[spid].type == uninitialized) {
                                auto r = type_retour(it_st->idx, embarquement, workingDt);
                                if(b_dest.ajouter_best_reverse(spid, r, count))
                                    working_retour[spid] = r;
                            }
                        }

                        const auto & retour_temp = prec_retour[spid];
                        if(retour_temp.type != uninitialized && 
                           (t== -1 || retour_temp.dt >= DateTime(workingDt.date(), it_st->arrival_time))) {
                            DateTime dt = retour_temp.dt;

                            if(retour_temp.type == vj)
                                dt = dt - 120;


                            int etemp = tardiest_trip(route, i, dt);
                            if(etemp >=0 && t!=etemp) {
                                if(t!=etemp) {
                                    embarquement = spid;
                                    t = etemp;
                                    it_st = data.pt_data.stop_times.begin() 
                                            + data.pt_data.vehicle_journeys[t].stop_time_list[i];
                                }
                                    workingDt = dt;
                            }
                            
                        }
                    }
                }
            }
            marcheapiedreverse();
        }
    }


    Path RAPTOR::makeBestPath(map_retour_t &retour, map_int_pint_t &best, std::vector<std::pair<type::idx_t, double> > departs, unsigned int destination_idx, unsigned int count) {
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




    Path RAPTOR::makeBestPathreverse(map_retour_t &retour, map_int_pint_t &best, std::vector<std::pair<type::idx_t, double> > departs,
                                     unsigned int destination_idx, unsigned int count) {
        return makePathreverse(retour, best, departs, destination_idx, count);
    }





    std::vector<Path> RAPTOR::makePathes(map_retour_t &retour, map_int_pint_t &best, std::vector<std::pair<type::idx_t, double> > departs,
                                         best_dest &b_dest, unsigned int count) {
        std::vector<Path> result;

        for(unsigned int i=1;i<=count;++i) {
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





    std::vector<Path> RAPTOR::makePathesreverse(map_retour_t &retour, map_int_pint_t &best, std::vector<std::pair<type::idx_t, double> > departs,
                                                best_dest &b_dest, unsigned int count) {
        std::vector<Path> result;

        for(unsigned int i=1;i<=count;++i) {
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







    Path RAPTOR::makePath(map_retour_t &retour, map_int_pint_t &best, std::vector<std::pair<type::idx_t, double> > departs,
                          unsigned int destination_idx, unsigned int countb, bool reverse) {
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
                    item = PathItem(retour[countb][r.spid_emarquement].dt, workingDate);
                else
                    item = PathItem(workingDate, retour[countb][r.spid_emarquement].dt);

                item.stop_points.push_back(current_spid);
                item.type = walking;
                item.stop_points.push_back(r.spid_emarquement);
                result.items.push_back(item);

                spid_embarquement = navitia::type::invalid_idx;
                current_spid = r.spid_emarquement;

            } else { // Sinon c'est un trajet TC
                // Est-ce que qu'on a affaire à un nouveau trajet ?
                if(spid_embarquement == navitia::type::invalid_idx) {
                    r = retour[countb][current_spid];
                    spid_embarquement = r.spid_emarquement;
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

                        if(!reverse && current_st.arrival_time%data.dataRaptor.NB_MINUTES_MINUIT > prec_st.arrival_time%data.dataRaptor.NB_MINUTES_MINUIT && prec_st.vehicle_journey_idx!=navitia::type::invalid_idx)
                            workingDate.date_decrement();
                        else if(reverse && current_st.arrival_time%data.dataRaptor.NB_MINUTES_MINUIT < prec_st.arrival_time%data.dataRaptor.NB_MINUTES_MINUIT && prec_st.vehicle_journey_idx!=navitia::type::invalid_idx)
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





    Path RAPTOR::makePathreverse(map_retour_t &retour, map_int_pint_t &best, std::vector<std::pair<type::idx_t, double> > departs,
                                 unsigned int destination_idx, unsigned int countb) {
        return makePath(retour, best, departs, destination_idx, countb, true);
    }






    std::vector<Path> RAPTOR::compute(idx_t departure_idx, idx_t destination_idx, int departure_hour,
                                      int departure_day, bool clockwise) {
        if(clockwise)
            return compute(departure_idx, destination_idx, departure_hour, departure_day, DateTime::inf, clockwise);
        else
            return compute(departure_idx, destination_idx, departure_hour, departure_day, DateTime::min, clockwise);

    }







    std::vector<Path> RAPTOR::compute(idx_t departure_idx, idx_t destination_idx, int departure_hour,
                                      int departure_day, DateTime borne, bool clockwise) {
    
        std::vector<std::pair<type::idx_t, double> > departs, destinations;
        
        for(navitia::type::idx_t spidx : data.pt_data.stop_areas[departure_idx].stop_point_list) {
            departs.push_back(std::make_pair(spidx, 0));
        }

        for(navitia::type::idx_t spidx : data.pt_data.stop_areas[destination_idx].stop_point_list) {
            destinations.push_back(std::make_pair(spidx, 0));
        }

        if(clockwise)
            return compute_all(departs, destinations, DateTime(departure_day, departure_hour), borne);
        else
            return compute_reverse_all(departs, destinations, DateTime(departure_day, departure_hour), borne);


    }
}}}
