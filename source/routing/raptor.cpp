#include "raptor.h"
#include <boost/foreach.hpp>

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
        if(data.dataRaptor.validity_patterns[data.dataRaptor.vp_idx_forward[idx]].test(date)
                && data.pt_data.stop_times[data.dataRaptor.st_idx_forward[idx]].pick_up_allowed())
            return data.pt_data.stop_times[data.dataRaptor.st_idx_forward[idx]].vehicle_journey_idx;
        ++idx;
    }

    //Si on en a pas trouvé, on cherche le lendemain

    ++date;
    idx = begin - data.dataRaptor.departure_times.begin();
    for(it = begin; it != end; ++it) {
        if(data.dataRaptor.validity_patterns[data.dataRaptor.vp_idx_forward[idx]].test(date)
                && data.pt_data.stop_times[data.dataRaptor.st_idx_forward[idx]].pick_up_allowed())
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
        if(data.dataRaptor.validity_patterns[data.dataRaptor.vp_idx_backward[idx]].test(date)
                && data.pt_data.stop_times[data.dataRaptor.st_idx_backward[idx]].drop_off_allowed())
            return data.pt_data.stop_times[data.dataRaptor.st_idx_backward[idx]].vehicle_journey_idx;
        ++idx;
    }

    //Si on en a pas trouvé, on cherche la veille
    if(date > 0) {
        --date;
        idx = begin - data.dataRaptor.arrival_times.begin();
        for(it = begin; it != end; ++it) {
            if(data.dataRaptor.validity_patterns[data.dataRaptor.vp_idx_backward[idx]].test(date)
                    && data.pt_data.stop_times[data.dataRaptor.st_idx_backward[idx]].drop_off_allowed())
                return data.pt_data.stop_times[data.dataRaptor.st_idx_backward[idx]].vehicle_journey_idx;
            ++idx;
        }
    }


    //Cette route ne comporte aucun trip compatible
    return -1;
}









void RAPTOR::make_queue() {
    marked_rp.reset();
    marked_sp.reset();
}









 void RAPTOR::make_queuereverse() {
    marked_rp.reset();
    marked_sp.reset();
 }




void RAPTOR::route_path_connections_forward() {
    std::vector<type::idx_t> to_mark;
    for(auto rp = marked_rp.find_first(); rp != marked_rp.npos; rp = marked_rp.find_next(rp)) {
        BOOST_FOREACH(auto &idx_rpc, data.dataRaptor.footpath_rp_forward.equal_range(rp)) {
            const auto & rpc = idx_rpc.second;
            if(retour[count][rp].arrival < best[rpc.destination_route_point_idx].arrival) {
                retour[count][rpc.destination_route_point_idx] = 
                    type_retour(retour[count][rp].arrival, retour[count][rp].arrival, rp, connection_extension);
                best[rpc.destination_route_point_idx] = retour[count][rpc.destination_route_point_idx];
                to_mark.push_back(rpc.destination_route_point_idx);
            }
        }
    }

    for(auto rp : to_mark) {
        marked_rp.set(rp);
        const auto & route_point = data.pt_data.route_points[rp];
        if(Q[route_point.route_idx] > route_point.order) {
            Q[route_point.route_idx] = route_point.order;
        }
    }
}




void RAPTOR::route_path_connections_backward() {
    std::vector<type::idx_t> to_mark;
    for(auto rp = marked_rp.find_first(); rp != marked_rp.npos; rp = marked_rp.find_next(rp)) {
        BOOST_FOREACH(auto &idx_rpc,  data.dataRaptor.footpath_rp_backward.equal_range(rp)) {
            const auto & rpc = idx_rpc.second;
            if(retour[count][rp].arrival > best[rpc.departure_route_point_idx].arrival) {
                retour[count][rpc.departure_route_point_idx] = 
                    type_retour(retour[count][rp].arrival, retour[count][rp].arrival, rp, connection_extension);
                best[rpc.departure_route_point_idx] = retour[count][rpc.departure_route_point_idx];
                to_mark.push_back(rpc.departure_route_point_idx);
            }
        }
    }

    for(auto rp : to_mark) {
        marked_rp.set(rp);
        const auto & route_point = data.pt_data.route_points[rp];
        if(Q[route_point.route_idx] < route_point.order) {
            Q[route_point.route_idx] = route_point.order;
        }
    }
}




void RAPTOR::marcheapied() {
    auto it = data.dataRaptor.foot_path.begin();
    int last = 0;

    for(auto stop_point_idx = marked_sp.find_first(); stop_point_idx != marked_sp.npos; 
        stop_point_idx = marked_sp.find_next(stop_point_idx)) {
        //On cherche le plus petit rp du sp 
        DateTime earliest_arrival = DateTime::inf;
        type::idx_t earliest_rp = navitia::type::invalid_idx;
        for(auto rpidx : data.pt_data.stop_points[stop_point_idx].route_point_list) {
            if(retour[count][rpidx].arrival < earliest_arrival) {
                earliest_arrival = retour[count][rpidx].arrival;
                earliest_rp = rpidx;
            }
        }
        DateTime earliest_departure = earliest_arrival + 120;
        //On marque tous les route points du stop point
        for(auto rpidx : data.pt_data.stop_points[stop_point_idx].route_point_list) {
            if(rpidx != earliest_rp && earliest_departure  < best[rpidx].arrival) {
               const type_retour nRetour = type_retour(earliest_departure, earliest_departure, earliest_rp);
               best[rpidx] = nRetour;
               retour[count][rpidx] = nRetour;
               const auto & route_point = data.pt_data.route_points[rpidx];
               if(!b_dest.ajouter_best(rpidx, nRetour, count) && Q[route_point.route_idx] > route_point.order) {
//                   marked_rp.set(rpidx);
                   Q[route_point.route_idx] = route_point.order;
               }
            }
        }
            
        //On va maintenant chercher toutes les connexions et on marque tous les route_points concernés
        
        const auto & index = data.dataRaptor.footpath_index[stop_point_idx];
        const type_retour & retour_temp = retour[count][earliest_rp];
        int prec_duration = -1;
        DateTime departure, arrival = retour_temp.arrival;
        it += index.first - last;
        const auto end = it + index.second;

        for(; it != end; ++it) {
            navitia::type::idx_t destination = (*it).destination_stop_point_idx;
            for(auto destination_rp : data.pt_data.stop_points[destination].route_point_list) {
                if(earliest_rp != destination_rp) {
                    if(it->duration != prec_duration) {
                        departure = arrival + it->duration;
                        prec_duration = it->duration;
                    }
                    if(departure <= best[destination_rp].arrival) {
                        const type_retour nRetour = type_retour(departure, departure, earliest_rp);
                        best[destination_rp] = nRetour;
                        retour[count][destination_rp] = nRetour;
                        const auto & route_point = data.pt_data.route_points[destination_rp];
                       if(!b_dest.ajouter_best(destination_rp, nRetour, count) && Q[route_point.route_idx] > route_point.order) {
//                            marked_rp.set(destination_rp);
                            Q[route_point.route_idx] = route_point.order;
                       }
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
    for(auto stop_point_idx = marked_sp.find_first(); stop_point_idx != marked_sp.npos; 
        stop_point_idx = marked_sp.find_next(stop_point_idx)) {
        //On cherche le plus petit rp du sp 
        DateTime tardiest_arrival = DateTime::min;
        type::idx_t tardiest_rp = navitia::type::invalid_idx;
        for(auto rpidx : data.pt_data.stop_points[stop_point_idx].route_point_list) {
            if(retour[count][rpidx].arrival > tardiest_arrival) {
                tardiest_arrival = retour[count][rpidx].arrival;
                tardiest_rp = rpidx;
            }
        }
        DateTime tardiest_departure = tardiest_arrival - 120;
        //On marque tous les route points du stop point
        for(auto rpidx : data.pt_data.stop_points[stop_point_idx].route_point_list) {
            if(rpidx != tardiest_rp && tardiest_departure  > best[rpidx].departure) {
               const type_retour nRetour = type_retour(tardiest_departure, tardiest_departure, tardiest_rp);
               best[rpidx] = nRetour;
               retour[count][rpidx] = nRetour;
               const auto & route_point = data.pt_data.route_points[rpidx];
               if(!b_dest.ajouter_best(rpidx, nRetour, count) && Q[route_point.route_idx] < route_point.order) {
//                   marked_rp.set(rpidx);
                   Q[route_point.route_idx] = route_point.order;
               }
            }
        }
            
        //On va maintenant chercher toutes les connexions et on marque tous les route_points concernés
        const auto & index = data.dataRaptor.footpath_index[stop_point_idx];
        const type_retour & retour_temp = retour[count][tardiest_rp];
        int prec_duration = -1;
        DateTime arrival, departure = retour_temp.departure;
        it += index.first - last;
        const auto end = it + index.second;

        for(; it != end; ++it) {
            navitia::type::idx_t destination = (*it).destination_stop_point_idx;
            for(auto destination_rp : data.pt_data.stop_points[destination].route_point_list) {
                if(tardiest_rp != destination_rp) {
                    if(it->duration != prec_duration) {
                        arrival = departure - it->duration;
                        prec_duration = it->duration;
                    }
                    if(arrival >= best[destination_rp].departure) {
                        const type_retour nRetour = type_retour(arrival, arrival, tardiest_rp);
                        best[destination_rp] = nRetour;
                        retour[count][destination_rp] = nRetour;
                        const auto & route_point = data.pt_data.route_points[destination_rp];
                       if(!b_dest.ajouter_best(destination_rp, nRetour, count) && Q[route_point.route_idx] < route_point.order) {
//                            marked_rp.set(destination_rp);
                            Q[route_point.route_idx] = route_point.order;
                       }
                    }
                }
            }
         }
         last = index.first + index.second;
     }
}


void RAPTOR::init(std::vector<std::pair<type::idx_t, double> > departs,
                  std::vector<std::pair<type::idx_t, double> > destinations,
                  const DateTime &dep, DateTime borne,  const bool clockwise, const bool reset, const bool dep_dest) {

    if(clockwise)
        Q.assign(data.pt_data.routes.size(), std::numeric_limits<int>::max());
    else {
        departs.swap(destinations);
        Q.assign(data.pt_data.routes.size(), -1);
    }

    std::map<type::idx_t, DateTime> dep_dt;


    if(dep_dest) {
        DateTime dt;
        if(!clockwise)
            dt = DateTime::min;
        for(unsigned int i = 1; i <= b_dest.count; ++i) {
            type::idx_t best_rp = type::invalid_idx;
            for(auto dep_dist : departs) {
                for(auto rpidx : data.pt_data.stop_points[dep_dist.first].route_point_list) {
                    if((retour[i][rpidx].type!= uninitialized )&&
                       ( (clockwise && retour[i][rpidx].arrival == best[rpidx].arrival && best[rpidx].arrival < dt) ||
                         (!clockwise && retour[i][rpidx].departure == best[rpidx].departure && best[rpidx].departure > dt))) {
                        if(clockwise)
                            dt = best[rpidx].arrival;
                        else
                            dt = best[rpidx].departure;
                        best_rp = rpidx;
                    }
                }
            }
            if(best_rp != type::invalid_idx)
                dep_dt.insert(std::make_pair(best_rp, dt));
        }
    } else {
        for(auto dep_dist : departs) {
            for(auto rpidx : data.pt_data.stop_points[dep_dist.first].route_point_list) {
                if(clockwise)
                    dep_dt.insert(std::make_pair(rpidx, dep + dep_dist.second/1.38));
                else
                    dep_dt.insert(std::make_pair(rpidx, dep - dep_dist.second/1.38));
            }
        }

    }

    if(reset) {
        retour.clear();

        if(clockwise) {
            retour.push_back(data.dataRaptor.retour_constant);
            best = data.dataRaptor.retour_constant;
            b_dest.reinit(data.pt_data.route_points.size(), borne, clockwise);
        } else {
            retour.push_back(data.dataRaptor.retour_constant_reverse);
            best = data.dataRaptor.retour_constant_reverse;
            if(borne == DateTime::inf)
                borne = DateTime::min;
                b_dest.reinit(data.pt_data.route_points.size(), borne, clockwise);
        }
    }

     for(auto item : departs) {
         bool found = false;
        for(type::idx_t rpidx: data.pt_data.stop_points[item.first].route_point_list) {
            if(dep_dt.find(rpidx) != dep_dt.end()) {
                retour[0][rpidx] = type_retour(dep_dt[rpidx], dep_dt[rpidx]);
                best[rpidx] = retour[0][rpidx];
                const auto & route_point = data.pt_data.route_points[rpidx];
                if(clockwise && Q[route_point.route_idx] > route_point.order)
                    Q[route_point.route_idx] = route_point.order;
                else if(!clockwise &&  Q[route_point.route_idx] < route_point.order)
                    Q[route_point.route_idx] = route_point.order;
                found = true;
            }
            else {
                retour[0][rpidx] = clockwise ? type_retour(DateTime::inf, DateTime::inf) : type_retour(DateTime::min, DateTime::min);
                best[rpidx] = retour[0][rpidx];
            }
        }
        if(found)
            marked_sp.set(item.first);
     }

     for(auto item : destinations) {
        if(clockwise) {
            for(type::idx_t rpidx: data.pt_data.stop_points[item.first].route_point_list) {
                if(routes_valides.test(data.pt_data.route_points[rpidx].route_idx)) {
                    b_dest.ajouter_destination(rpidx, (item.second/1.38));
                    best[rpidx].arrival = borne;
                }
            }
        }
        else {
            for(type::idx_t rpidx: data.pt_data.stop_points[item.first].route_point_list) {
                if(routes_valides.test(data.pt_data.route_points[rpidx].route_idx)) {
                    b_dest.ajouter_destination(rpidx, (item.second/1.38));
                    best[rpidx].departure = borne;
                }
            }
        }
     }

     count = 1;
}



std::vector<Path>
RAPTOR::compute_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                    const std::vector<std::pair<type::idx_t, double> > &destinations,
                    const DateTime &dt_depart, const DateTime &borne, const std::multimap<std::string, std::string> & forbidden) {
    std::vector<Path> result;

    set_routes_valides(dt_depart.date(), forbidden);
    init(departs, destinations, dt_depart, borne, true, true);
    boucleRAPTOR();

    if(b_dest.best_now.type == uninitialized) {
        return result;
    }


    init(departs, destinations, b_dest.best_now.departure, dt_depart/*get_temps_depart(dt_depart, departs)*/, false, true, true);

    boucleRAPTORreverse(false);

    if(b_dest.best_now.type != uninitialized) {
        auto temp = makePathesreverse(destinations, departs);
        result.insert(result.end(), temp.begin(), temp.end());
        return result;
    }

    return result;
}

DateTime RAPTOR::get_temps_depart(const DateTime &dt_depart,const std::vector<std::pair<type::idx_t, double> > &departs) {
    bool stop = false;
    type::idx_t current_rpid = b_dest.best_now_rpid;
    int cnt = b_dest.count;
    DateTime dt = dt_depart;
    int hour = dt_depart.hour();
    while(!stop) {

        if(retour[cnt][current_rpid].type == connection || retour[cnt][current_rpid].type == connection_extension
                ||  retour[cnt][current_rpid].type == connection_guarantee) {
            current_rpid = retour[cnt][current_rpid].rpid_embarquement;
            hour = retour[cnt][current_rpid].arrival.hour();
        } else if(retour[cnt][current_rpid].type == vj){
            type::StopTime st = data.pt_data.stop_times[retour[cnt][current_rpid].stop_time_idx];
            type::idx_t boarding = retour[cnt][current_rpid].rpid_embarquement;
            current_rpid = boarding;
            hour = data.pt_data.stop_times[data.pt_data.vehicle_journeys[st.vehicle_journey_idx].stop_time_list[data.pt_data.route_points[current_rpid].order]].departure_time;
            --cnt;
        }

        stop = retour[cnt][current_rpid].type == depart;
    }

    auto it_depart = std::find_if(departs.begin(), departs.end(), [&](std::pair<type::idx_t, double> p1){return p1.first == data.pt_data.route_points[current_rpid].stop_point_idx;});
    dt.update(hour - std::ceil(it_depart->second / 1.38));
    return dt;
}




std::vector<Path>
RAPTOR::compute_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                    const std::vector<std::pair<type::idx_t, double> > &destinations,
    std::vector<DateTime> dt_departs, const DateTime &borne) {
    std::vector<Path> result;
    std::vector<best_dest> bests;

    std::sort(dt_departs.begin(), dt_departs.end(), [](DateTime dt1, DateTime dt2) {return dt1 > dt2;});

    // Attention ! Et si les départs ne sont pas le même jour ?
    //set_routes_valides(dt_departs.front());

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
            init(departs, destinations, b.best_now.departure, dt_depart, false, true);
            boucleRAPTORreverse();
            if(b_dest.best_now.type != uninitialized){
                auto temp = makePathesreverse(destinations, departs);
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

    std::sort(dt_departs.begin(), dt_departs.end());

    // Attention ! Et si les départs ne sont pas le même jour ?
   // set_routes_valides(dt_departs.front().date);

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
                auto temp = makePathes(departs, destinations);
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
                            const DateTime &dt_depart, const DateTime &borne, const std::multimap<std::string, std::string> & forbidden) {
    std::vector<Path> result;

    set_routes_valides(dt_depart.date(), forbidden);
    init(departs, destinations, dt_depart, borne, false, true);
    boucleRAPTORreverse();

    if(b_dest.best_now.type == uninitialized) {
        return result;
    }

//    auto t = makePathesreverse(destinations);
//    result.insert(result.end(), t.begin(), t.end());

    init(departs, destinations, b_dest.best_now.arrival, dt_depart, true, true);
    boucleRAPTOR();

    if(b_dest.best_now.type != uninitialized) {
        auto temp = makePathes(departs, destinations);
        result.insert(result.end(), temp.begin(), temp.end());
    }
    return result;
}



void RAPTOR::set_routes_valides(uint32_t date, const std::multimap<std::string, std::string> & forbidden) {
    routes_valides.clear();
    routes_valides.resize(data.pt_data.routes.size());
    for(const auto & route : data.pt_data.routes) {
        const navitia::type::Line & line = data.pt_data.lines[route.line_idx];
        const navitia::type::ModeType & mode = data.pt_data.mode_types[route.mode_type_idx];

        // On gère la liste des interdits
        bool forbidden_route = false;
        for(auto pair : forbidden){
            if(pair.first == "line" && pair.second == line.external_code)
                forbidden_route = true;
            if(pair.first == "route" && pair.second == route.external_code)
                forbidden_route = true;
            if(pair.first == "mode" && pair.second == mode.external_code)
                forbidden_route = true;
        }

        // Si la route n'a pas été bloquée par un paramètre, on la valide s'il y a au moins une circulation à j-1/j+1
        if(!forbidden_route){
            for(auto vjidx : route.vehicle_journey_list) {
                if(data.pt_data.validity_patterns[data.pt_data.vehicle_journeys[vjidx].validity_pattern_idx].check2(date)) {
                    routes_valides.set(route.idx);
                    break;
                }
            }
        }
    }
}



struct raptor_visitor {
    RAPTOR & raptor;
    raptor_visitor(RAPTOR & raptor) : raptor(raptor) {}
    int embarquement_init() const {
        return type::invalid_idx;
    }

    DateTime working_datetime_init() const {
        return DateTime::inf;
    }

    bool better(const DateTime &a, const DateTime &b) const {
        return a < b;
    }

    bool better(const type_retour &a, const type_retour &b) const {
        return a.arrival < b.arrival;
    }

    bool better_or_equal(const type_retour &a, const DateTime &current_dt, const type::StopTime &st) {
        return previous_datetime(a) <= current_datetime(current_dt.date(), st);
    }

    void walking(){
        raptor.marcheapied();
    }

    void route_path_connections() {
        raptor.route_path_connections_forward();
    }

    void make_queue(){
        raptor.make_queue();
    }

    int best_trip(const type::Route & route, int order, const DateTime & date_time) const {
        return raptor.earliest_trip(route, order, date_time);
    }

    void one_more_step() {
        raptor.retour.push_back(raptor.data.dataRaptor.retour_constant);
    }

    bool store_better(const navitia::type::idx_t rpid, DateTime &working_date_time, const type_retour&bound,
                      const navitia::type::StopTime &st, const navitia::type::idx_t embarquement) {
        auto & working_retour = raptor.retour[raptor.count];
        working_date_time.update(st.arrival_time);
        if(better(working_date_time, bound.arrival) && st.drop_off_allowed()) {
            working_retour[rpid] = type_retour(st, working_date_time, embarquement, true);
            raptor.best[rpid] = working_retour[rpid];
            if(!raptor.b_dest.ajouter_best(rpid, working_retour[rpid], raptor.count)) {
                raptor.marked_rp.set(rpid);
                raptor.marked_sp.set(raptor.data.pt_data.route_points[rpid].stop_point_idx);
                return false;
            }
        } else if(working_date_time == bound.arrival &&
                  raptor.retour[raptor.count-1][rpid].type == uninitialized) {
            auto r = type_retour(st, working_date_time, embarquement, true);
            if(raptor.b_dest.ajouter_best(rpid, r, raptor.count)) {
                working_retour[rpid] = r;
                raptor.best[rpid] = r;
            }
        }
        return true;
    }

    DateTime previous_datetime(const type_retour & tau) const {
        return tau.arrival;
    }

    DateTime current_datetime(int date, const type::StopTime & stop_time) const {
        return DateTime(date, stop_time.departure_time);
    }

    std::pair<std::vector<type::RoutePoint>::const_iterator, std::vector<type::RoutePoint>::const_iterator>
    route_points(const type::Route &route, size_t order) const {
        return std::make_pair(raptor.data.pt_data.route_points.begin() + route.route_point_list[order],
                              raptor.data.pt_data.route_points.begin() + route.route_point_list.back() + 1);
    }

    std::vector<type::StopTime>::const_iterator first_stoptime(size_t position) const {
        return raptor.data.pt_data.stop_times.begin() + position;
    }


    int min_time_to_wait(navitia::type::idx_t /*rp1*/, navitia::type::idx_t /*rp2*/) {
        return 120;
    }

    void update(DateTime & dt, const type::StopTime & st) {
        dt.update(st.arrival_time);
    }

    void reset_queue_item(int &item) {
        item = std::numeric_limits<int>::max();
    }
};

struct raptor_reverse_visitor {
    RAPTOR & raptor;
    raptor_reverse_visitor(RAPTOR & raptor) : raptor(raptor) {}

    int embarquement_init() const {
        return std::numeric_limits<int>::max();
    }

    DateTime working_datetime_init() const {
        return DateTime::min;
    }

    bool better(const DateTime &a, const DateTime &b) const {
        return a > b;
    }

    bool better(const type_retour &a, const type_retour &b) const {
        return a.departure > b.departure;
    }

    bool better_or_equal(const type_retour &a, const DateTime &current_dt, const type::StopTime &st) {
        return previous_datetime(a) >= current_datetime(current_dt.date(), st);
    }

    void walking() {
        raptor.marcheapiedreverse();
    }

    void route_path_connections() {
        raptor.route_path_connections_backward();
    }

    void make_queue(){
        raptor.make_queuereverse();
    }

    int best_trip(const type::Route & route, int order, const DateTime & date_time) const {
        return raptor.tardiest_trip(route, order, date_time);
    }

    void one_more_step() {
        raptor.retour.push_back(raptor.data.dataRaptor.retour_constant_reverse);
    }

    bool store_better(const navitia::type::idx_t rpid, DateTime &working_date_time, const type_retour&bound,
                      const navitia::type::StopTime st, const navitia::type::idx_t embarquement) {
        auto & working_retour = raptor.retour[raptor.count];
        working_date_time.updatereverse(st.departure_time);
        if(better(working_date_time, bound.departure) && st.pick_up_allowed()) {
            working_retour[rpid] = type_retour(st, working_date_time, embarquement, false);
            raptor.best[rpid] = working_retour[rpid];
            if(!raptor.b_dest.ajouter_best_reverse(rpid, working_retour[rpid], raptor.count)) {
                raptor.marked_rp.set(rpid);
                raptor.marked_sp.set(raptor.data.pt_data.route_points[rpid].stop_point_idx);
                return false;
            }
        } else if(working_date_time == bound.departure &&
                  raptor.retour[raptor.count-1][rpid].type == uninitialized) {
            auto r = type_retour(st, working_date_time, embarquement, false);
            if(raptor.b_dest.ajouter_best_reverse(rpid, r, raptor.count))
                working_retour[rpid] = r;
        }
        return true;
    }

    DateTime previous_datetime(const type_retour & tau) const {
        return tau.departure;
    }

    DateTime current_datetime(int date, const type::StopTime & stop_time) const {
        return DateTime(date, stop_time.arrival_time);
    }

    std::pair<std::vector<type::RoutePoint>::const_reverse_iterator, std::vector<type::RoutePoint>::const_reverse_iterator>
    route_points(const type::Route &route, size_t order) const {
        const auto begin = raptor.data.pt_data.route_points.rbegin() + raptor.data.pt_data.route_points.size() - route.route_point_list[order] - 1;
        const auto end = begin + order + 1;
        return std::make_pair(begin, end);
    }

    std::vector<type::StopTime>::const_reverse_iterator first_stoptime(size_t position) const {
        return raptor.data.pt_data.stop_times.rbegin() + raptor.data.pt_data.stop_times.size() - position - 1;
    }

    int min_time_to_wait(navitia::type::idx_t /*rp1*/, navitia::type::idx_t /*rp2*/) {
        return -120;
    }

    void update(DateTime & dt, const type::StopTime & st) {
        dt.updatereverse(st.departure_time);
    }

    void reset_queue_item(int &item) {
        item = -1;
    }
};

template<typename Visitor>
void RAPTOR::raptor_loop(Visitor visitor, bool global_pruning) {
    bool end = false;
    count = 0;
    int t=-1, embarquement = visitor.embarquement_init();
    DateTime workingDt = visitor.working_datetime_init();
    uint32_t l_zone = std::numeric_limits<uint32_t>::max();

    visitor.walking();
    while(!end) {
        ++count;
        end = true;
        if(count == retour.size())
            visitor.one_more_step();
        const auto & prec_retour = retour[count -1];
        visitor.make_queue();
        for(const auto & route : data.pt_data.routes) {
            if(Q[route.idx] != std::numeric_limits<int>::max() && Q[route.idx] != -1 && routes_valides.test(route.idx)) {
                t = -1;
                embarquement = visitor.embarquement_init();
                workingDt = visitor.working_datetime_init();
                decltype(visitor.first_stoptime(0)) it_st;
                BOOST_FOREACH(const type::RoutePoint & rp, visitor.route_points(route, Q[route.idx])) {
                    if(t >= 0) {
                        ++it_st;
                        if(l_zone == std::numeric_limits<uint32_t>::max() || l_zone != it_st->local_traffic_zone) {
                            //On stocke, et on marque pour explorer par la suite
                            if(visitor.better(best[rp.idx], b_dest.best_now) || !global_pruning)
                                end = visitor.store_better(rp.idx, workingDt, best[rp.idx], *it_st, embarquement) && end;
                            else
                                end = visitor.store_better(rp.idx, workingDt, b_dest.best_now, *it_st, embarquement) && end;
                        }
                    }

                    //Si on peut arriver plus tôt à l'arrêt en passant par une autre route
                    const type_retour & retour_temp = prec_retour[rp.idx];
                    if(retour_temp.type != uninitialized &&
                       (t == -1 || visitor.better_or_equal(retour_temp, workingDt, *it_st))) {

                        int etemp = visitor.best_trip(route, rp.order, visitor.previous_datetime(retour_temp));
                        if(etemp >= 0 && t != etemp) {
                            t = etemp;
                            embarquement = rp.idx;
                            it_st = visitor.first_stoptime(data.pt_data.vehicle_journeys[t].stop_time_list[rp.order]);

                            workingDt = visitor.previous_datetime(retour_temp);
                            visitor.update(workingDt, *it_st);
                            l_zone = it_st->local_traffic_zone;
                        }
                    }
                }
            }
            visitor.reset_queue_item(Q[route.idx]);
        }
        visitor.route_path_connections();
        visitor.walking();
    }
}

void RAPTOR::boucleRAPTOR(){
    auto visitor = raptor_visitor(*this);
    raptor_loop(visitor);
}

void RAPTOR::boucleRAPTORreverse(bool global_pruning){
    auto visitor = raptor_reverse_visitor(*this);
    raptor_loop(visitor, global_pruning);
}

std::vector<Path> RAPTOR::makePathes(std::vector<std::pair<type::idx_t, double> > departs, std::vector<std::pair<type::idx_t, double> > destinations) {
    std::vector<Path> result;
    DateTime dt;
    for(unsigned int i=1;i<=count;++i) {
        int rpid = std::numeric_limits<int>::max();
        for(auto spid_dist : destinations) {
            for(auto dest : data.pt_data.stop_points[spid_dist.first].route_point_list) {
                if(retour[i][dest].type != uninitialized && retour[i][dest].arrival + spid_dist.second < dt) {
                    dt = best[dest].arrival + spid_dist.second;
                    rpid = dest;
                }
            }
        }
        if(rpid != std::numeric_limits<int>::max())
            result.push_back(makePath(departs, rpid, i));
    }

    return result;
}





std::vector<Path> RAPTOR::makePathesreverse(std::vector<std::pair<type::idx_t, double> > departs, std::vector<std::pair<type::idx_t, double> > destinations) {
    std::vector<Path> result;

    DateTime dt = DateTime::min;
    for(unsigned int i=1;i<=count;++i) {
        int rpid = std::numeric_limits<int>::max();
        for(auto spid_dist : destinations) {
            for(auto dest : data.pt_data.stop_points[spid_dist.first].route_point_list) {
                if(retour[i][dest].type != uninitialized && retour[i][dest].departure - spid_dist.second > dt) {
                    dt = best[dest].departure - spid_dist.second;
                    rpid = dest;
                }
            }
        }
        if(rpid != std::numeric_limits<int>::max())
            result.push_back(makePathreverse(departs, rpid, i));
    }
    return result;
}







Path RAPTOR::makePath(std::vector<std::pair<type::idx_t, double> > departs,
                      unsigned int destination_idx, unsigned int countb, bool reverse) {
    Path result;
    unsigned int current_rpid = destination_idx;
    type_retour r = retour[countb][current_rpid];
    DateTime workingDate;
    if(!reverse)
        workingDate = r.arrival;
    else
        workingDate = r.departure;

    navitia::type::StopTime current_st;
    navitia::type::idx_t rpid_embarquement = navitia::type::invalid_idx;

    bool stop = false;
    //        for(auto item : departs) {
    //            stop = stop || item.first == current_spid;
    //        }

    PathItem item;
    // On boucle tant
    while(!stop) {

        // Est-ce qu'on a une section marche à pied ?
        if(retour[countb][current_rpid].type == connection || retour[countb][current_rpid].type == connection_extension
                ||  retour[countb][current_rpid].type == connection_guarantee) {
            r = retour[countb][current_rpid];
            auto r2 = retour[countb][r.rpid_embarquement];
            if(reverse) {
                item = PathItem(r.departure, r2.arrival);
            } else {
                item = PathItem(r2.arrival, r.departure);
            }

            item.stop_points.push_back(data.pt_data.route_points[current_rpid].stop_point_idx);
            item.stop_points.push_back(data.pt_data.route_points[r.rpid_embarquement].stop_point_idx);
            if(r.type == connection)
                item.type = walking;
            else if(r.type == connection_extension)
                item.type = extension;
            else if(r.type == connection_guarantee)
                item.type = guarantee;

            result.items.push_back(item);

            rpid_embarquement = navitia::type::invalid_idx;
            current_rpid = r.rpid_embarquement;
//            if(retour[countb][current_rpid].type == depart)
//                stop = true;

        } else { // Sinon c'est un trajet TC
            // Est-ce que qu'on a à faire à un nouveau trajet ?
            if(rpid_embarquement == navitia::type::invalid_idx) {
                r = retour[countb][current_rpid];
                rpid_embarquement = r.rpid_embarquement;
                current_st = data.pt_data.stop_times.at(r.stop_time_idx);

                item = PathItem();
                item.type = public_transport;

                if(!reverse) {
                    workingDate = r.departure;
                }
                else {
                    workingDate = r.arrival;
                }
                item.vj_idx = current_st.vehicle_journey_idx;
                while(rpid_embarquement != current_rpid) {

                    //On stocke le sp, et les temps
                    item.stop_points.push_back(data.pt_data.route_points[current_rpid].stop_point_idx);
                    if(!reverse) {
                        workingDate.updatereverse(current_st.departure_time);
                        item.departures.push_back(workingDate);
                        workingDate.updatereverse(current_st.arrival_time);
                        item.arrivals.push_back(workingDate);
                    } else {
                        workingDate.update(current_st.arrival_time);
                        item.arrivals.push_back(workingDate);
                        workingDate.update(current_st.departure_time);
                        item.departures.push_back(workingDate);
                    }

                    //On va chercher le prochain stop time
                    if(!reverse)
                        current_st = data.pt_data.stop_times.at(current_st.idx - 1);
                    else
                        current_st = data.pt_data.stop_times.at(current_st.idx + 1);

                    //Est-ce que je suis sur un route point de fin 
                    current_rpid = current_st.route_point_idx;
                }
                // Je stocke le dernier stop point, et ses temps d'arrivée et de départ
                item.stop_points.push_back(data.pt_data.route_points[current_rpid].stop_point_idx);
                if(!reverse) {
                    workingDate.updatereverse(current_st.departure_time);
                    item.departures.push_back(workingDate);
                    workingDate.updatereverse(current_st.arrival_time);
                    item.arrivals.push_back(workingDate);
                    item.arrival = item.arrivals.front();
                    item.departure = item.departures.back();
                } else {
                    workingDate.update(current_st.arrival_time);
                    item.arrivals.push_back(workingDate);
                    workingDate.update(current_st.departure_time);
                    item.departures.push_back(workingDate);
                    item.arrival = item.arrivals.back();
                    item.departure = item.departures.front();
                }

                //On stocke l'item créé
                result.items.push_back(item);

                --countb;
                rpid_embarquement = navitia::type::invalid_idx ;

            }
        }
        if(retour[countb][current_rpid].type == depart)
            stop = true;

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

    result.nb_changes = 0;
    if(result.items.size() > 2) {
        for(unsigned int i = 1; i <= (result.items.size()-2); ++i) {
            if(result.items[i].type == walking)
                ++ result.nb_changes;
        }
    }

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
