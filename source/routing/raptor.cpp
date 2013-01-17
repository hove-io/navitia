#include "raptor.h"
#include <boost/foreach.hpp>
#include <boost/geometry.hpp>

namespace navitia { namespace routing { namespace raptor {

void RAPTOR::make_queue() {
    marked_rp.reset();
    marked_sp.reset();
}



 void RAPTOR::make_queuereverse() {
    marked_rp.reset();
    marked_sp.reset();
 }




void RAPTOR::route_path_connections_forward(const bool wheelchair) {
    std::vector<type::idx_t> to_mark;
    for(auto rp = marked_rp.find_first(); rp != marked_rp.npos; rp = marked_rp.find_next(rp)) {
        BOOST_FOREACH(auto &idx_rpc, data.dataRaptor.footpath_rp_forward.equal_range(rp)) {
            const auto & rpc = idx_rpc.second;
            DateTime dt = retour[count][rp].arrival + rpc.length;
            if(retour[count][rp].type == vj && dt < best[rpc.destination_route_point_idx].arrival &&
                    (!wheelchair || (data.pt_data.stop_points[rpc.departure_route_point_idx].is_adapted
                                     && data.pt_data.stop_points[rpc.destination_route_point_idx].is_adapted))) {
                if(rpc.connection_kind == type::ConnectionKind::extension)
                    retour[count][rpc.destination_route_point_idx] = type_retour(dt, dt, rp, connection_extension);
                else
                    retour[count][rpc.destination_route_point_idx] = type_retour(dt, dt, rp, connection_guarantee);
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




void RAPTOR::route_path_connections_backward(const bool wheelchair) {
    std::vector<type::idx_t> to_mark;
    for(auto rp = marked_rp.find_first(); rp != marked_rp.npos; rp = marked_rp.find_next(rp)) {
        BOOST_FOREACH(auto &idx_rpc,  data.dataRaptor.footpath_rp_backward.equal_range(rp)) {
            const auto & rpc = idx_rpc.second;
            DateTime dt = retour[count][rp].arrival - rpc.length;
            if(retour[count][rp].type == vj && dt > best[rpc.departure_route_point_idx].arrival&&
                    (!wheelchair || (data.pt_data.stop_points[rpc.departure_route_point_idx].is_adapted
                                     && data.pt_data.stop_points[rpc.destination_route_point_idx].is_adapted))) {
                if(rpc.connection_kind == type::ConnectionKind::extension)
                    retour[count][rpc.departure_route_point_idx] = type_retour(dt, dt, rp, connection_extension);
                else
                    retour[count][rpc.departure_route_point_idx] = type_retour(dt, dt, rp, connection_guarantee);
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




struct walking_backwards_visitor {

    static DateTime worst() {
        return DateTime::min;
    }
    template<typename T1, typename T2> bool comp(const T1& a, const T2& b) const {
        return a > b;
    }
    template<typename T1, typename T2> auto combine(const T1& a, const T2& b) const -> decltype(a-b) {
        return a - b;
    }

    static DateTime type_retour::*  instant;
    static constexpr bool clockwise = false;
};
DateTime type_retour::*  walking_backwards_visitor::instant= &type_retour::departure;

struct walking_visitor {
    static DateTime worst() {
        return DateTime::inf;
    }
    template<typename T1, typename T2> bool comp(const T1& a, const T2& b) const {
        return a < b;
    }
    template<typename T1, typename T2> auto combine(const T1& a, const T2& b) const -> decltype(a+b) {
        return a + b;
    }

    static DateTime type_retour::*  instant;
    static constexpr bool clockwise = true;
};
DateTime type_retour::*  walking_visitor::instant = &type_retour::arrival;


template<typename Visitor>
void RAPTOR::foot_path(const Visitor & v, const bool wheelchair) {
    std::vector<navitia::type::Connection>::const_iterator it = (v.clockwise) ? data.dataRaptor.foot_path_forward.begin() :
                                                                                data.dataRaptor.foot_path_backward.begin();
    int last = 0;

    for(auto stop_point_idx = marked_sp.find_first(); stop_point_idx != marked_sp.npos; 
        stop_point_idx = marked_sp.find_next(stop_point_idx)) {
        //On cherche le plus petit rp du sp 
        DateTime best_arrival = v.worst();
        type::idx_t best_rp = navitia::type::invalid_idx;
        const type::StopPoint & stop_point = data.pt_data.stop_points[stop_point_idx];
        if(!wheelchair || stop_point.is_adapted) {
            for(auto rpidx : stop_point.route_point_list) {
                if((retour[count][rpidx].type == vj || retour[count][rpidx].type == depart) && v.comp(retour[count][rpidx].arrival, best_arrival)) {
                    best_arrival = retour[count][rpidx].arrival;
                    best_rp = rpidx;
                }
            }
            if(best_rp != type::invalid_idx) {
                DateTime best_departure = v.combine(best_arrival, 120);
                //On marque tous les route points du stop point
                for(auto rpidx : stop_point.route_point_list) {
                    if(rpidx != best_rp && v.comp(best_departure, best[rpidx].*Visitor::instant) ) {
                       const type_retour nRetour = type_retour(best_departure, best_departure, best_rp);
                       best[rpidx] = nRetour;
                       retour[count][rpidx] = nRetour;
                       const auto & route_point = data.pt_data.route_points[rpidx];
                       if(!b_dest.ajouter_best(rpidx, nRetour, count, v.clockwise) && v.comp(route_point.order, Q[route_point.route_idx]) ) {
        //                   marked_rp.set(rpidx);
                           Q[route_point.route_idx] = route_point.order;
                       }
                    }
                }


                //On va maintenant chercher toutes les connexions et on marque tous les route_points concernés

                const pair_int & index = (v.clockwise) ? data.dataRaptor.footpath_index_forward[stop_point_idx] :
                                                         data.dataRaptor.footpath_index_backward[stop_point_idx];

                const type_retour & retour_temp = retour[count][best_rp];
                int prec_duration = -1;
                DateTime next = v.worst(), previous = retour_temp.*v.instant;
                it += index.first - last;
                const auto end = it + index.second;

                for(; it != end; ++it) {
                    navitia::type::idx_t destination = (*it).destination_stop_point_idx;
                    if(!wheelchair || data.pt_data.stop_points[destination].is_adapted) {
                        for(auto destination_rp : data.pt_data.stop_points[destination].route_point_list) {
                            if(best_rp != destination_rp) {
                                if(it->duration != prec_duration) {
                                    next = v.combine(previous, it->duration);
                                    prec_duration = it->duration;
                                }
                                if(v.comp(next, best[destination_rp].*v.instant) || next == best[destination_rp].*v.instant) {
                                    const type_retour nRetour = type_retour(next, next, best_rp);
                                    best[destination_rp] = nRetour;
                                    retour[count][destination_rp] = nRetour;
                                    const auto & route_point = data.pt_data.route_points[destination_rp];

                                   if(!b_dest.ajouter_best(destination_rp, nRetour, count, v.clockwise) && v.comp(route_point.order, Q[route_point.route_idx])) {
            //                            marked_rp.set(destination_rp);
                                        Q[route_point.route_idx] = route_point.order;
                                   }
                                }
                            }
                        }
                    }
                }

                 last = index.first + index.second;
            }
        }


     }
}



void RAPTOR::marcheapied(const bool wheelchair) {
    walking_visitor v;
    foot_path(v, wheelchair);
}

void RAPTOR::marcheapiedreverse(const bool wheelchair) {
    walking_backwards_visitor v;
    foot_path(v, wheelchair);
}



void RAPTOR::clear_and_init(std::vector<init::Departure_Type> departs,
                  std::vector<std::pair<type::idx_t, double> > destinations,
                  DateTime borne,  const bool clockwise, const bool clear, const float walking_speed) {

    if(clockwise)
        Q.assign(data.pt_data.routes.size(), std::numeric_limits<int>::max());
    else {
        Q.assign(data.pt_data.routes.size(), -1);
    }

    if(clear) {
        retour.clear();
        best.clear();
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


    for(init::Departure_Type item : departs) {
        retour[0][item.rpidx] = type_retour(item.arrival, item.arrival);
        best[item.rpidx] = retour[0][item.rpidx];
        const auto & route_point = data.pt_data.route_points[item.rpidx];
        if(clockwise && Q[route_point.route_idx] > route_point.order)
            Q[route_point.route_idx] = route_point.order;
        else if(!clockwise &&  Q[route_point.route_idx] < route_point.order)
            Q[route_point.route_idx] = route_point.order;
        if(item.arrival != DateTime::min && item.arrival!=DateTime::inf) {
            marked_sp.set(data.pt_data.route_points[item.rpidx].stop_point_idx);
        }
    }

    for(auto item : destinations) {
        for(type::idx_t rpidx: data.pt_data.stop_points[item.first].route_point_list) {
            if(routes_valides.test(data.pt_data.route_points[rpidx].route_idx) &&
                    ((clockwise && (borne == DateTime::inf || best[rpidx].arrival > borne)) ||
                    ((!clockwise) && (borne == DateTime::min || best[rpidx].departure < borne)))) {
                    b_dest.ajouter_destination(rpidx, (item.second/walking_speed));
                    if(clockwise)
                        best[rpidx].arrival = borne;
                    else
                        best[rpidx].departure = borne;
                }
        }
    }

     count = 1;
}


template<typename Visitor>
std::vector<Path>
RAPTOR::compute_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                    const std::vector<std::pair<type::idx_t, double> > &destinations,
                    const DateTime &dt_depart, const DateTime &borne, const float walking_speed, const bool wheelchair, const std::multimap<std::string, std::string> & forbidden,
                    Visitor vis) {
    std::vector<Path> result;
    std::vector<init::Departure_Type> departures;

    set_routes_valides(dt_depart.date(), forbidden);
    if(vis.first_phase_clockwise) {
        departures = init::getDepartures(departs, dt_depart, vis.first_phase_clockwise, data, walking_speed);
        clear_and_init(departures, destinations, borne, vis.first_phase_clockwise, true, walking_speed);
        boucleRAPTOR(wheelchair, false);
    } else {
        departures = init::getDepartures(destinations, dt_depart, vis.first_phase_clockwise, data, walking_speed);
        clear_and_init(departures, departs, borne, vis.first_phase_clockwise, true, walking_speed);
        boucleRAPTORreverse(wheelchair, false);
    }


    if(b_dest.best_now.type == uninitialized) {
        return result;
    }

    //Second passe
    if(vis.second_phase_clockwise)
        departures = init::getDepartures(destinations, departs, vis.second_phase_clockwise, this->retour, data, walking_speed);
    else
        departures = init::getDepartures(departs, destinations, vis.second_phase_clockwise, this->retour, data, walking_speed);

    for(auto departure : departures) {
        if(vis.second_phase_clockwise) {
            clear_and_init({departure}, destinations, dt_depart, vis.second_phase_clockwise, true, walking_speed);
            boucleRAPTOR(wheelchair, false);
        }
        else {
            clear_and_init({departure}, departs, dt_depart, vis.second_phase_clockwise, true, walking_speed);
            boucleRAPTORreverse(wheelchair, true);
        }

        if(b_dest.best_now.type != uninitialized) {
            std::vector<Path> temp;
            if(vis.second_phase_clockwise)
                temp = makePathes(destinations, dt_depart, walking_speed);
            else
                temp = makePathesreverse(departs, dt_depart, walking_speed);
            result.insert(result.end(), temp.begin(), temp.end());
        }
    }

    return result;
}


struct visitor_clockwise {
    static constexpr bool first_phase_clockwise = true;
    static constexpr bool second_phase_clockwise = false;
};

std::vector<Path>
RAPTOR::compute_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                    const std::vector<std::pair<type::idx_t, double> > &destinations,
                    const DateTime &dt_depart, const DateTime &borne, const float walking_speed, const bool wheelchair, const std::multimap<std::string, std::string> & forbidden) {

    return compute_all(departs, destinations, dt_depart, borne, walking_speed, wheelchair, forbidden, visitor_clockwise());
}

struct visitor_anti_clockwise {
    static constexpr bool first_phase_clockwise = false;
    static constexpr bool second_phase_clockwise = true;
};
std::vector<Path>
RAPTOR::compute_reverse_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                            const std::vector<std::pair<type::idx_t, double> > &destinations,
                            const DateTime &dt_depart, const DateTime &borne, const float walking_speed, const bool wheelchair, const std::multimap<std::string, std::string> & forbidden) {
    return compute_all(departs, destinations, dt_depart, borne, walking_speed, wheelchair, forbidden, visitor_anti_clockwise());
}



std::vector<Path>
RAPTOR::compute_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                    const std::vector<std::pair<type::idx_t, double> > &destinations,
    std::vector<DateTime> dt_departs, const DateTime &borne, const float walking_speed, const bool wheelchair) {
    std::vector<Path> result;
    std::vector<best_dest> bests;

    std::sort(dt_departs.begin(), dt_departs.end(), [](DateTime dt1, DateTime dt2) {return dt1 > dt2;});

    // Attention ! Et si les départs ne sont pas le même jour ?
    //set_routes_valides(dt_departs.front());

    bool reset = true;
    for(auto dep : dt_departs) {
        auto departures = init::getDepartures(departs, dep, true, data, walking_speed);
        clear_and_init(departures, destinations, borne, true, reset, walking_speed);
        boucleRAPTOR(wheelchair);
        bests.push_back(b_dest);
        reset = false;
    }

    for(unsigned int i=0; i<dt_departs.size(); ++i) {
        auto b = bests[i];
        auto dt_depart = dt_departs[i];
        if(b.best_now.type != uninitialized) {
            init::Departure_Type d;
            d.rpidx = b.best_now_rpid;
            d.arrival = b.best_now.arrival;
            clear_and_init({d}, departs, dt_depart, false, true, walking_speed);
            boucleRAPTORreverse(wheelchair);
            if(b_dest.best_now.type != uninitialized){
                auto temp = makePathesreverse(departs, dt_depart, walking_speed);
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
                    std::vector<DateTime> dt_departs, const DateTime &borne, const float walking_speed, const bool wheelchair) {
    std::vector<Path> result;
    std::vector<best_dest> bests;

    std::sort(dt_departs.begin(), dt_departs.end());

    // Attention ! Et si les départs ne sont pas le même jour ?
   // set_routes_valides(dt_departs.front().date);

    bool reset = true;
    for(auto dep : dt_departs) {
        auto departures = init::getDepartures(departs, dep, false, data, walking_speed);
        clear_and_init(departures, destinations, borne, false, reset, walking_speed);
        boucleRAPTORreverse(wheelchair);
        bests.push_back(b_dest);
        reset = false;
    }

    for(unsigned int i=0; i<dt_departs.size(); ++i) {
        auto b = bests[i];
        auto dt_depart = dt_departs[i];
        if(b.best_now.type != uninitialized) {
            init::Departure_Type d;
            d.rpidx = b.best_now_rpid;
            d.arrival = b.best_now.departure;
            clear_and_init({d}, departs, dt_depart, true, true, walking_speed);
            boucleRAPTOR(wheelchair);
            if(b_dest.best_now.type != uninitialized){
                auto temp = makePathes(destinations, dt_depart, walking_speed);
                auto path = temp.back();
                path.request_time = boost::posix_time::ptime(data.meta.production_date.begin(),
                                                             boost::posix_time::seconds(dt_depart.hour()));
                result.push_back(path);
            }
        }
    }

    return result;
}







std::vector<idx_retour>
RAPTOR::isochrone(const std::vector<std::pair<type::idx_t, double> > &departs,
          const DateTime &dt_depart, const DateTime &borne,
          const float walking_speed, const bool wheelchair,
          const std::multimap<std::string, std::string> & forbidden) {

    std::vector<idx_retour> result;
    set_routes_valides(dt_depart.date(), forbidden);
    auto departures = init::getDepartures(departs, dt_depart, true, data, walking_speed);
    clear_and_init(departures, {}, borne, true, true, walking_speed);

    boucleRAPTOR(wheelchair);

    for(const type::StopPoint &sp : data.pt_data.stop_points) {
        DateTime mini = DateTime::inf;
        type::idx_t mini_rp = type::invalid_idx;
        for(type::idx_t rpidx : sp.route_point_list) {
            if(best[rpidx].arrival < mini) {
                mini = best[rpidx].arrival;
                mini_rp = rpidx;
            }    
        }

        type_retour t;
        if(mini_rp != type::invalid_idx) {
            result.push_back(std::make_pair(sp.idx,best[mini_rp]));
        }
    
    }
    std::sort(result.begin(), result.end(), 
            [](idx_retour r1, idx_retour r2) {
               return r1.second.arrival < r2.second.arrival; 
            });
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

    void walking(const bool wheelchair){
        raptor.marcheapied(wheelchair);
    }

    void route_path_connections(const bool wheelchair) {
        raptor.route_path_connections_forward(wheelchair);
    }

    void make_queue(){
        raptor.make_queue();
    }

    std::pair<type::idx_t, uint32_t> best_trip(const type::Route & route, int order, const DateTime & date_time, const bool wheelchair) const {
        return earliest_trip(route, order, date_time, raptor.data, wheelchair);
    }

    void one_more_step() {
        raptor.retour.push_back(raptor.data.dataRaptor.retour_constant);
    }

    bool store_better(const navitia::type::idx_t rpid, DateTime &working_date_time, const type_retour&bound,
                      const navitia::type::StopTime &st, const navitia::type::idx_t embarquement, uint32_t gap) {
        auto & working_retour = raptor.retour[raptor.count];
        working_date_time.update(!st.is_frequency()? st.arrival_time : st.start_time + gap);
        if(better(working_date_time, bound.arrival) && st.drop_off_allowed()) {
            working_retour[rpid] = type_retour(st, working_date_time, embarquement, true, gap);
            raptor.best[rpid] = working_retour[rpid];
            if(!raptor.b_dest.ajouter_best(rpid, working_retour[rpid], raptor.count)) {
                raptor.marked_rp.set(rpid);
                raptor.marked_sp.set(raptor.data.pt_data.route_points[rpid].stop_point_idx);
                return false;
            }
        } else if(working_date_time == bound.arrival &&
                  raptor.retour[raptor.count-1][rpid].type == uninitialized) {
            auto r = type_retour(st, working_date_time, embarquement, true, gap);
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

    void update(DateTime & dt, const type::StopTime & st, const uint32_t gap) {
        dt.update(!st.is_frequency()? st.arrival_time : st.start_time+gap);
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

    void walking(const bool wheelchair) {
        raptor.marcheapiedreverse(wheelchair);
    }

    void route_path_connections(const bool wheelchair) {
        raptor.route_path_connections_backward(wheelchair);
    }

    void make_queue(){
        raptor.make_queuereverse();
    }

    std::pair<type::idx_t, uint32_t> best_trip(const type::Route & route, int order, const DateTime & date_time, const bool wheelchair) const {
        return tardiest_trip(route, order, date_time, raptor.data, wheelchair);
    }

    void one_more_step() {
        raptor.retour.push_back(raptor.data.dataRaptor.retour_constant_reverse);
    }

    bool store_better(const navitia::type::idx_t rpid, DateTime &working_date_time, const type_retour&bound,
                      const navitia::type::StopTime st, const navitia::type::idx_t embarquement, uint32_t gap) {
        auto & working_retour = raptor.retour[raptor.count];
        working_date_time.updatereverse(!st.is_frequency()? st.departure_time : st.start_time + gap);
        if(better(working_date_time, bound.departure) && st.pick_up_allowed()) {
            working_retour[rpid] = type_retour(st, working_date_time, embarquement, false, gap);
            raptor.best[rpid] = working_retour[rpid];
            if(!raptor.b_dest.ajouter_best_reverse(rpid, working_retour[rpid], raptor.count)) {
                raptor.marked_rp.set(rpid);
                raptor.marked_sp.set(raptor.data.pt_data.route_points[rpid].stop_point_idx);
                return false;
            }
        } else if(working_date_time == bound.departure &&
                  raptor.retour[raptor.count-1][rpid].type == uninitialized) {
            auto r = type_retour(st, working_date_time, embarquement, false, gap);
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

    void update(DateTime & dt, const type::StopTime & st, const uint32_t gap) {
        dt.updatereverse(!st.is_frequency() ? st.departure_time : st.start_time + gap);
    }

    void reset_queue_item(int &item) {
        item = -1;
    }
};

template<typename Visitor>
void RAPTOR::raptor_loop(Visitor visitor, const bool wheelchair, bool global_pruning) {
    bool end = false;
    count = 0;
    type::idx_t t=-1;
    int embarquement = visitor.embarquement_init();
    DateTime workingDt = visitor.working_datetime_init();
    uint32_t l_zone = std::numeric_limits<uint32_t>::max();

    visitor.walking(wheelchair);
    while(!end) {
        ++count;
        end = true;
        if(count == retour.size())
            visitor.one_more_step();
        const auto & prec_retour = retour[count -1];
        visitor.make_queue();
        for(const auto & route : data.pt_data.routes) {
            if(Q[route.idx] != std::numeric_limits<int>::max() && Q[route.idx] != -1 && routes_valides.test(route.idx)) {
                t = type::invalid_idx;
                embarquement = visitor.embarquement_init();
                workingDt = visitor.working_datetime_init();
                decltype(visitor.first_stoptime(0)) it_st;
                int gap = 0;
                BOOST_FOREACH(const type::RoutePoint & rp, visitor.route_points(route, Q[route.idx])) {
                    if(t != type::invalid_idx) {
                        ++it_st;
                        if(l_zone == std::numeric_limits<uint32_t>::max() || l_zone != it_st->local_traffic_zone) {
                            //On stocke, et on marque pour explorer par la suite
                            if(visitor.better(best[rp.idx], b_dest.best_now) || !global_pruning)
                                end = visitor.store_better(rp.idx, workingDt, best[rp.idx], *it_st, embarquement, gap) && end;
                            else
                                end = visitor.store_better(rp.idx, workingDt, b_dest.best_now, *it_st, embarquement, gap) && end;
                        }
                    }

                    //Si on peut arriver plus tôt à l'arrêt en passant par une autre route
                    const type_retour & retour_temp = prec_retour[rp.idx];
                    if(retour_temp.type != uninitialized &&
                       (t == type::invalid_idx || visitor.better_or_equal(retour_temp, workingDt, *it_st))) {

                        type::idx_t etemp;
                        std::tie(etemp, gap) = visitor.best_trip(route, rp.order, visitor.previous_datetime(retour_temp), wheelchair);

                        if(etemp != type::invalid_idx && t != etemp) {
                            t = etemp;
                            embarquement = rp.idx;
                            it_st = visitor.first_stoptime(data.pt_data.vehicle_journeys[t].stop_time_list[rp.order]);
                            workingDt = visitor.previous_datetime(retour_temp);
                            visitor.update(workingDt, *it_st, gap);
                            l_zone = it_st->local_traffic_zone;
                        }
                    }
                }
            }
            visitor.reset_queue_item(Q[route.idx]);
        }
        visitor.route_path_connections(wheelchair);
        visitor.walking(wheelchair);
    }
}

void RAPTOR::boucleRAPTOR(const bool wheelchair, bool global_pruning){
    auto visitor = raptor_visitor(*this);
    raptor_loop(visitor, wheelchair, global_pruning);
}

void RAPTOR::boucleRAPTORreverse(const bool wheelchair, bool global_pruning){
    auto visitor = raptor_reverse_visitor(*this);
    raptor_loop(visitor, wheelchair, global_pruning);
}

std::vector<Path> RAPTOR::makePathes(std::vector<std::pair<type::idx_t, double> > destinations, DateTime dt, const float walking_speed) {
    std::vector<Path> result;
    DateTime best_dt = dt;
    for(unsigned int i=1;i<=count;++i) {
        type::idx_t best_rp = type::invalid_idx;
        for(auto spid_dist : destinations) {
            for(auto dest : data.pt_data.stop_points[spid_dist.first].route_point_list) {
                if(retour[i][dest].type != uninitialized) {
                    DateTime current_dt = retour[i][dest].departure + (spid_dist.second/walking_speed);
                    if(current_dt <= best_dt) {
                        best_dt = current_dt;
                        best_rp = dest;
                    }
                }
            }
        }
        if(best_rp != type::invalid_idx)
            result.push_back(makePath(best_rp, i));
    }

    return result;
}





std::vector<Path> RAPTOR::makePathesreverse(std::vector<std::pair<type::idx_t, double> > destinations, DateTime dt, const float walking_speed) {
    std::vector<Path> result;

    DateTime best_dt = dt;

    for(unsigned int i=1;i<=count;++i) {
        type::idx_t best_rp = type::invalid_idx;
        for(auto spid_dist : destinations) {
            for(auto dest : data.pt_data.stop_points[spid_dist.first].route_point_list) {
                if(retour[i][dest].type != uninitialized) {
                    DateTime current_dt = retour[i][dest].departure - (spid_dist.second/walking_speed);
                    if(current_dt >= best_dt) {
                        best_dt = current_dt;
                        best_rp = dest;
                    }
                }
            }
        }

        if(best_rp != type::invalid_idx)
            result.push_back(makePathreverse(best_rp, i));
    }
    return result;
}







Path RAPTOR::makePath(type::idx_t destination_idx, unsigned int countb, bool reverse) {
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
        } else { // Sinon c'est un trajet TC
            // Est-ce que qu'on a à faire à un nouveau trajet ?
            if(rpid_embarquement == navitia::type::invalid_idx) {
                r = retour[countb][current_rpid];
                rpid_embarquement = r.rpid_embarquement;
                current_st = data.pt_data.stop_times.at(r.stop_time_idx);
                uint32_t gap = r.arrival.hour() - current_st.arrival_time%data.dataRaptor.SECONDS_PER_DAY;

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
                        workingDate.updatereverse(current_st.departure_time+gap);
                        item.departures.push_back(workingDate);
                        workingDate.updatereverse(current_st.arrival_time+gap);
                        item.arrivals.push_back(workingDate);
                    } else {
                        workingDate.update(current_st.arrival_time+gap);
                        item.arrivals.push_back(workingDate);
                        workingDate.update(current_st.departure_time+gap);
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
                    workingDate.updatereverse(current_st.departure_time+gap);
                    item.departures.push_back(workingDate);
                    workingDate.updatereverse(current_st.arrival_time+gap);
                    item.arrivals.push_back(workingDate);
                    item.arrival = item.arrivals.front();
                    item.departure = item.departures.back();
                } else {
                    workingDate.update(current_st.arrival_time+gap);
                    item.arrivals.push_back(workingDate);
                    workingDate.update(current_st.departure_time+gap);
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





Path RAPTOR::makePathreverse(unsigned int destination_idx, unsigned int countb) {
    return makePath(destination_idx, countb, true);
}






std::vector<Path> RAPTOR::compute(idx_t departure_idx, idx_t destination_idx, int departure_hour,
                                  int departure_day, bool clockwise, bool wheelchair) {
    if(clockwise)
        return compute(departure_idx, destination_idx, departure_hour, departure_day, DateTime::inf, clockwise, wheelchair);
    else
        return compute(departure_idx, destination_idx, departure_hour, departure_day, DateTime::min, clockwise, wheelchair);

}







std::vector<Path> RAPTOR::compute(idx_t departure_idx, idx_t destination_idx, int departure_hour,
                                  int departure_day, DateTime borne, bool clockwise, bool wheelchair) {
    
    std::vector<std::pair<type::idx_t, double> > departs, destinations;

    for(navitia::type::idx_t spidx : data.pt_data.stop_areas[departure_idx].stop_point_list) {
        departs.push_back(std::make_pair(spidx, 0));
    }

    for(navitia::type::idx_t spidx : data.pt_data.stop_areas[destination_idx].stop_point_list) {
        destinations.push_back(std::make_pair(spidx, 0));
    }

    if(clockwise)
        return compute_all(departs, destinations, DateTime(departure_day, departure_hour), borne, 1, wheelchair);
    else
        return compute_reverse_all(departs, destinations, DateTime(departure_day, departure_hour), borne, 1, wheelchair);


}
}}}
