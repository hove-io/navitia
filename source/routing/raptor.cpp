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
        std::vector<navitia::type::idx_t> to_mark;
        for(auto stop_point = marked_sp.find_first(); stop_point != marked_sp.npos; stop_point = marked_sp.find_next(stop_point)) {
            const auto & index = data.dataRaptor.footpath_index[stop_point];
            const type_retour & retour_temp = retour[count][stop_point];
            int prec_duration = -1;
            DateTime departure, arrival = retour_temp.arrival;
            //std::advance(it, index.first - last);
            it += index.first - last;
            const auto end = it + index.second;

            for(; it != end; ++it) {
                navitia::type::idx_t destination = (*it).destination_stop_point_idx;
                if(stop_point != destination) {
                    if(it->duration != prec_duration) {
                        departure = arrival + it->duration;
                        prec_duration = it->duration;
                    }
                    if(departure <= best[destination].arrival) {
                        const type_retour nRetour = type_retour(departure, departure, stop_point);
                            best[destination] = nRetour;
                            retour[count][destination] = nRetour;
                            if(!b_dest.ajouter_best(destination, nRetour, count)) {
                                to_mark.push_back(destination);
                            }

                        /*else if(count == 0 || retour[count - 1][destination].type == uninitialized) {
                            retour[count][destination] = nRetour;
                        }*/
                    }
                }
            }
            last = index.first + index.second;
        }

        for(auto destination : to_mark)
            marked_sp.set(destination);
    }








    void RAPTOR::marcheapiedreverse() {
        auto it = data.dataRaptor.foot_path.begin();
        int last = 0;
        std::vector<navitia::type::idx_t> to_mark;

        for(auto stop_point = marked_sp.find_first(); stop_point != marked_sp.npos; stop_point = marked_sp.find_next(stop_point)) {
            const auto & index = data.dataRaptor.footpath_index[stop_point];
            const type_retour & retour_temp = retour[count][stop_point];
            int prec_duration = -1;
            DateTime arrival, departure = retour_temp.departure;
            it += index.first - last;
            const auto end = it + index.second;
            for(; it != end; ++it) {
                navitia::type::idx_t destination = it->destination_stop_point_idx;
                if(stop_point != destination) {
                    if(it->duration != prec_duration) {
                        arrival = departure - it->duration;
                        prec_duration = it->duration;
                    }
                    if(arrival >= best[destination].departure) {
                        const type_retour nRetour = type_retour(arrival, arrival, stop_point);
                            best[destination] = nRetour;
                            retour[count][destination] = nRetour;
                            if(!b_dest.ajouter_best_reverse(destination, nRetour, count)) {
                                to_mark.push_back(destination);
                        } /*else if(count == 0 || retour[count - 1][destination].type == uninitialized) {
                            retour[count][destination] = nRetour;
                            best[destination] = nRetour;
                        }*/
                    }
                }
            }
            last = index.first + index.second;
        }

        for(auto destination : to_mark)
            marked_sp.set(destination);
    }




    void RAPTOR::init(std::vector<std::pair<type::idx_t, double> > departs, 
                      std::vector<std::pair<type::idx_t, double> > destinations,
                      const DateTime dep, DateTime borne,  const bool clockwise, const bool reset) {

        if(reset) {
            if(clockwise) {
                retour.assign(20, data.dataRaptor.retour_constant);
                best = data.dataRaptor.retour_constant;
                b_dest.reinit(borne, clockwise);
            } else {
                retour.assign(20, data.dataRaptor.retour_constant_reverse);
                best = data.dataRaptor.retour_constant_reverse;
                if(borne == DateTime::inf)
                    borne = DateTime::min;
                b_dest.reinit(borne, clockwise);
            }
        }

        if(!clockwise) {
            auto temp = departs;
            departs = destinations;
            destinations = temp;
//            departs.swap(destinations);
        }

        for(auto item : departs) {
            DateTime departure = dep, arrival = dep;
            if(clockwise) {
                arrival = arrival + (item.second / 1.38) ;
            } else {
                departure = departure - (item.second / 1.38);
            }
            auto r = type_retour(arrival, departure);
            retour[0][item.first] = r;
            best[item.first] = r;
            marked_sp.set(item.first);
        }

        for(auto item : destinations) {
            if(clockwise) {
                b_dest.ajouter_destination(item.first, 0, (item.second/1.38));
                best[item.first].arrival = borne;
            }
            else { 
                b_dest.ajouter_destination(item.first, (item.second/1.38), 0);
                best[item.first].departure = borne;
            }
        }

        count = 1;
    }



    std::vector<Path> 
    RAPTOR::compute_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                        const std::vector<std::pair<type::idx_t, double> > &destinations,
                        const DateTime &dt_depart, const DateTime &borne) {
            std::vector<Path> result;

            init(departs, destinations, dt_depart, borne, true, true);
            set_routes_valides(dt_depart);
            boucleRAPTOR();


            if(b_dest.best_now.type == uninitialized) {
                return result;
            }


            init(departs, destinations, b_dest.best_now.arrival, dt_depart, false, true);

            boucleRAPTORreverse();

            if(b_dest.best_now.type != uninitialized){
                auto temp = makePathesreverse(destinations);
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

        set_routes_valides(dt_departs.front());
        
        bool reset = true;
        for(auto dep : dt_departs) {
            init(departs, destinations, dep, borne, true, reset);
            boucleRAPTOR();
            bests.push_back(b_dest);
            reset = false;
        }

        for(unsigned int i=0; i<dt_departs.size(); ++i) {
            auto b = bests[i];
            auto dt_depart = dt_departs[i];
            if(b.best_now.type != uninitialized) {
                init(departs, destinations, dt_depart, b.best_now.arrival, false, true);
                boucleRAPTORreverse();
                if(b_dest.best_now.type != uninitialized){
                    auto temp = makePathesreverse(destinations);
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
                                 std::vector<DateTime> dt_departs, const DateTime &borne) {
         std::vector<Path> result;
         std::vector<best_dest> bests;

         std::sort(dt_departs.begin(), dt_departs.end(), [](DateTime dt1, DateTime dt2) {return dt1 < dt2;});

         set_routes_valides(dt_departs.front());

         bool reset = true;
         for(auto dep : dt_departs) {
             init(departs, destinations, dep, borne, false, reset);
             boucleRAPTORreverse();
             bests.push_back(b_dest);
             reset = false;
         }

         for(unsigned int i=0; i<dt_departs.size(); ++i) {
             auto b = bests[i];
             auto dt_depart = dt_departs[i];
             if(b.best_now.type != uninitialized) {
                 
                 init(departs, destinations, dt_depart, b.best_now.departure, true, true);
                 boucleRAPTOR();
                 if(b_dest.best_now.type != uninitialized){
                     auto temp = makePathes(departs);
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

        init(departs, destinations, dt_depart, borne, false, true);
        set_routes_valides(dt_depart);
        boucleRAPTORreverse();

        if(b_dest.best_now.type == uninitialized) {
            return result;
        }
        auto tmp = makePathesreverse(destinations);
        result.insert(result.end(), tmp.begin(), tmp.end());

        init(departs, destinations, dt_depart, b_dest.best_now.departure, true, true);

        boucleRAPTOR();

        if(b_dest.best_now.type != uninitialized){
            auto temp = makePathes(departs);
            result.insert(result.end(), temp.begin(), temp.end());
        }
        return result;
    }



    void RAPTOR::set_routes_valides(const DateTime& dtDepart) {

        uint32_t date = dtDepart.date();

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

    int RAPTOR::min_time_to_wait(navitia::type::idx_t /*rp1*/, navitia::type::idx_t /*rp2*/) {
        return 120;
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
                            //On stocke, et on marque pour explorer par la suite
                            workingDt.update(it_st->arrival_time);
                            const auto & min = std::min(best[spid].arrival, b_dest.best_now.arrival);
                            if(workingDt < min) {
                                working_retour[spid] = type_retour(*it_st, workingDt, embarquement, true);
                                best[spid]           = working_retour[spid] ;
                                if(!b_dest.ajouter_best(spid, working_retour[spid], count)) {
                                    marked_sp.set(spid);
                                    end = false;
                                }
                            } else if(workingDt == min &&
                                      prec_retour[spid].type == uninitialized) {
                                auto r = type_retour(*it_st, workingDt, embarquement, true);
                                if(b_dest.ajouter_best(spid, r, count))
                                    working_retour[spid] = r;
                            }
                        }

                        //Si on peut arriver plus tôt à l'arrêt en passant par une autre route
                        const type_retour & retour_temp = prec_retour[spid];
                        if((retour_temp.type != uninitialized) &&
                            (t == -1 || retour_temp.arrival <= DateTime(workingDt.date(), it_st->departure_time))) {
                                DateTime dt = retour_temp.arrival;

                            if(retour_temp.type == vj) {
                                dt = dt + min_time_to_wait(data.pt_data.stop_times[retour_temp.stop_time_idx].route_point_idx,
                                                           route.route_point_list[i]);
                            }

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
                            //On stocke, et on marque pour explorer par la suite
                            const auto & max = std::max(best[spid].departure, b_dest.best_now.departure);
                            workingDt.updatereverse(it_st->departure_time);
                            if(workingDt > max) {
                                working_retour[spid] = type_retour(*it_st, workingDt, embarquement, false);
                                best[spid]           = working_retour[spid];
                                if(!b_dest.ajouter_best_reverse(spid, working_retour[spid], count)) {
                                    marked_sp.set(spid);
                                    end = false;
                                }
                            } else if(workingDt == max && 
                                      prec_retour[spid].type == uninitialized) {
                                auto r = type_retour(*it_st, workingDt, embarquement, false);
                                if(b_dest.ajouter_best_reverse(spid, r, count))
                                    working_retour[spid] = r;
                            }
                        }

                        const auto & retour_temp = prec_retour[spid];
                        if(retour_temp.type != uninitialized && 
                           (t== -1 || retour_temp.departure >= DateTime(workingDt.date(), it_st->arrival_time))) {
                            DateTime dt = retour_temp.departure;

                            if(retour_temp.type == vj)
                                dt = dt - min_time_to_wait(data.pt_data.stop_times[retour_temp.stop_time_idx].route_point_idx,
                                                           route.route_point_list[i]);


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

    std::vector<Path> RAPTOR::makePathes(std::vector<std::pair<type::idx_t, double> > departs) {
        std::vector<Path> result;

        for(unsigned int i=0;i<=count;++i) {
            DateTime dt;
            int spid = std::numeric_limits<int>::max();
            for(auto dest : b_dest.map_date_time) {
                if(retour[i][dest.first].type != uninitialized && retour[i][dest.first].arrival < dt) {
                    dt = best[dest.first].arrival;
                    spid = dest.first;
                }
            }
            if(spid != std::numeric_limits<int>::max())
                result.push_back(makePath(departs, spid, i));
        }

        return result;
    }





    std::vector<Path> RAPTOR::makePathesreverse(std::vector<std::pair<type::idx_t, double> > departs) {
        std::vector<Path> result;
//        DateTime dt = DateTime::min;

//        for(unsigned int i=0;i<=count;++i) {
//            int spid = std::numeric_limits<int>::max();
//            for(auto dest : b_dest.map_date_time) {
//                if(retour[i][dest.first].type != uninitialized && retour[i][dest.first].departure > dt) {
//                    dt = best[dest.first].departure;
//                    spid = dest.first;
//                }
//            }
//            if(spid != std::numeric_limits<int>::max())
//                result.push_back(makePathreverse(departs, spid, i));
//        }
        result.push_back(makePathreverse(departs, b_dest.best_now_spid, b_dest.count));

        return result;
    }







    Path RAPTOR::makePath(std::vector<std::pair<type::idx_t, double> > departs,
                          unsigned int destination_idx, unsigned int countb, bool reverse) {
        Path result;
        unsigned int current_spid = destination_idx;
        type_retour r = retour[countb][current_spid];
        DateTime workingDate = r.arrival;
        if(reverse)
            workingDate = r.departure;
        
        navitia::type::StopTime current_st;
        navitia::type::idx_t spid_embarquement = navitia::type::invalid_idx;

        bool stop = false;
//        for(auto item : departs) {
//            stop = stop || item.first == current_spid;
//        }

        result.nb_changes = countb - 1;
        PathItem item;
        // On boucle tant
        while(!stop) {

            // Est-ce qu'on a une section marche à pied ?
            if(retour[countb][current_spid].type == connection) {
                r = retour[countb][current_spid];
                if(!reverse)
                    workingDate = r.arrival;
                else
                    workingDate = r.departure;
                 workingDate.normalize();

                if(!reverse)
                    item = PathItem(retour[countb][r.spid_embarquement].arrival, workingDate);
                else
                    item = PathItem(workingDate, retour[countb][r.spid_embarquement].departure);

                item.stop_points.push_back(current_spid);
                item.type = walking;
                item.stop_points.push_back(r.spid_embarquement);
                result.items.push_back(item);

                spid_embarquement = navitia::type::invalid_idx;
                current_spid = r.spid_embarquement;

            } else { // Sinon c'est un trajet TC
                // Est-ce que qu'on a affaire à un nouveau trajet ?
                if(spid_embarquement == navitia::type::invalid_idx) {
                    r = retour[countb][current_spid];
                    spid_embarquement = r.spid_embarquement;
                    current_st = data.pt_data.stop_times.at(r.stop_time_idx);
                    if(!reverse)
                        workingDate = r.arrival;
                    else
                        workingDate = r.departure;

                    item = PathItem();
                    item.type = public_transport;

                    if(!reverse) {
                        item.arrival = workingDate;
                        item.arrivals.push_back(workingDate);
                    } 
                    else {
                        item.departure = DateTime(workingDate.date(), current_st.departure_time);
                        item.departures.push_back(DateTime(workingDate.date(), current_st.departure_time));
                    }

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
                        item.arrivals.push_back(DateTime(workingDate.date(), current_st.arrival_time));
                        item.departures.push_back(DateTime(workingDate.date(), current_st.departure_time));
                        item.vj_idx = current_st.vehicle_journey_idx;

                        current_spid = data.pt_data.route_points[current_st.route_point_idx].stop_point_idx;
                        for(auto item : departs) {
                            stop = stop || item.first == current_spid;
                        }
                        if(stop)
                            break;
                    }
                    item.stop_points.push_back(current_spid);
                    

                    if(!reverse) {
                        item.departure = DateTime(workingDate.date(), current_st.departure_time);
                        item.departures.push_back(DateTime(workingDate.date(), current_st.departure_time));
                    }
                    else {
                        item.arrival = workingDate;
                        item.arrivals.push_back(workingDate);
                    }

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
                std::reverse(item.arrivals.begin(), item.arrivals.end());
                std::reverse(item.departures.begin(), item.departures.end());
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





    Path RAPTOR::makePathreverse(std::vector<std::pair<type::idx_t, double> > departs,
                                 unsigned int destination_idx, unsigned int countb) {
        return makePath(departs, destination_idx, countb, true);
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
