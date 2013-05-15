#include "raptor.h"
#include <boost/foreach.hpp>

namespace navitia { namespace routing {

void RAPTOR::make_queue() {
    marked_rp.reset();
    marked_sp.reset();
}


template<class Visitor>
void 
RAPTOR::journey_pattern_path_connections(const Visitor & visitor/*,
                                         const std::bitset<7> & required_properties*/) {
    std::vector<type::idx_t> to_mark;
    for(auto jpp = marked_rp.find_first(); jpp != marked_rp.npos; jpp = marked_rp.find_next(jpp)) {
        BOOST_FOREACH(auto &idx_rpc, data.dataRaptor.footpath_rp(visitor.clockwise()).equal_range(jpp)) {
            const auto & rpc = idx_rpc.second;
            type::idx_t jpp_idx = rpc.*visitor.journey_pattern_point_idx();
            type::DateTime dt = visitor.combine(labels[count][jpp], rpc.duration);
            if(get_type(count, jpp) == boarding_type::vj && visitor.comp(dt, best_labels[jpp_idx]) /*&& !wheelchair*/) {
                labels[count][jpp_idx] = dt;
                best_labels[jpp_idx] = dt;
                boardings[count][jpp_idx] = jpp + data.pt_data.stop_times.size();
                to_mark.push_back(jpp_idx);
            }
        }
    }

    for(auto rp : to_mark) {
        marked_rp.set(rp);
        const auto & journey_pattern_point = data.pt_data.journey_pattern_points[rp];
        if(visitor.comp(journey_pattern_point.order, Q[journey_pattern_point.journey_pattern_idx]) ) {
            Q[journey_pattern_point.journey_pattern_idx] = journey_pattern_point.order;
        }
    }
}


template<typename Visitor>
void RAPTOR::foot_path(const Visitor & v, const type::Properties &required_properties) {
    std::vector<type::Connection>::const_iterator it = (v.clockwise()) ? data.dataRaptor.foot_path_forward.begin() :
                                                                                data.dataRaptor.foot_path_backward.begin();
    int last = 0;

    for(auto stop_point_idx = marked_sp.find_first(); stop_point_idx != marked_sp.npos; 
        stop_point_idx = marked_sp.find_next(stop_point_idx)) {
        //On cherche le plus petit jpp du sp 
        type::DateTime best_arrival = v.worst_datetime();
        type::idx_t best_jpp = type::invalid_idx;
        const type::StopPoint & stop_point = data.pt_data.stop_points[stop_point_idx];
        if(stop_point.accessible(required_properties)) {
            for(auto rpidx : stop_point.journey_pattern_point_list) {
                if(v.comp(labels[count][rpidx], best_arrival) && (get_type(count, rpidx) == boarding_type::vj || get_type(count, rpidx) == boarding_type::departure)) {
                    best_arrival = labels[count][rpidx];
                    best_jpp = rpidx;
                }
            }
            if(best_jpp != type::invalid_idx) {
                const type::DateTime best_departure = v.combine(best_arrival, 120);
                //On marque tous les journey_pattern points du stop point
                for(auto jpp : stop_point.journey_pattern_point_list) {
                    if(jpp != best_jpp && v.comp(best_departure, best_labels[jpp]) ) {
                       best_labels[jpp] = best_departure;
                       labels[count][jpp] = best_departure;
                       boardings[count][jpp] = best_jpp + data.pt_data.stop_times.size();
                       const auto & journey_pattern_point = data.pt_data.journey_pattern_points[jpp];
                       if(!b_dest.add_best(jpp, best_departure, count, v.clockwise()) && v.comp(journey_pattern_point.order, Q[journey_pattern_point.journey_pattern_idx]) ) {
                           Q[journey_pattern_point.journey_pattern_idx] = journey_pattern_point.order;
                       }
                    }
                }


                //On va maintenant chercher toutes les connexions et on marque tous les journey_pattern_points concernés

                const pair_int & index = (v.clockwise()) ? data.dataRaptor.footpath_index_forward[stop_point_idx] :
                                                         data.dataRaptor.footpath_index_backward[stop_point_idx];

                int prec_duration = -1;
                type::DateTime next = v.worst_datetime(), previous = labels[count][best_jpp];
                it += index.first - last;
                const auto end = it + index.second;

                for(; it != end; ++it) {
                    const auto & destination = data.pt_data.stop_points[it->destination_idx];
                    if(destination.accessible(required_properties)) {
                        for(auto destination_jpp : destination.journey_pattern_point_list) {
                            if(best_jpp != destination_jpp) {
                                if(it->duration != prec_duration) {
                                    next = v.combine(previous, it->duration);
                                    prec_duration = it->duration;
                                }
                                if(v.comp(next, best_labels[destination_jpp]) || next == best_labels[destination_jpp]) {
                                    best_labels[destination_jpp] = next;
                                    labels[count][destination_jpp] = next;
                                    boardings[count][destination_jpp] = best_jpp + data.pt_data.stop_times.size();
                                    const auto & journey_pattern_point = data.pt_data.journey_pattern_points[destination_jpp];

                                    if(!b_dest.add_best(destination_jpp, next, count, v.clockwise())
                                           && v.comp(journey_pattern_point.order, Q[journey_pattern_point.journey_pattern_idx])) {
                                        Q[journey_pattern_point.journey_pattern_idx] = journey_pattern_point.order;
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


void RAPTOR::clear(const type::Data & data, bool clockwise, type::DateTime borne, int walking_duration) {
    best_labels.clear();
    boardings = {data.dataRaptor.boardings_const};
    if(clockwise) {
        Q.assign(data.pt_data.journey_patterns.size(), std::numeric_limits<int>::max());
        labels = {data.dataRaptor.labels_const};
    } else {
        Q.assign(data.pt_data.journey_patterns.size(), -1);
        labels = {data.dataRaptor.labels_const_reverse};
    }
    b_dest.reinit(data.pt_data.journey_pattern_points.size(), borne, walking_duration);
    best_labels = labels[0];
}

void RAPTOR::clear_and_init(std::vector<Departure_Type> departs,
                  std::vector<std::pair<type::idx_t, double> > destinations,
                  type::DateTime borne,  const bool clockwise,
                  const float walking_speed, const int walking_distance,
                  const type::Properties &required_properties) {

    this->clear(data, clockwise, borne, std::ceil(walking_distance/walking_speed));

    for(Departure_Type item : departs) {
        const auto & journey_pattern_point = data.pt_data.journey_pattern_points[item.rpidx];
        const type::StopPoint& stop_point = data.pt_data.stop_points[journey_pattern_point.stop_point_idx];
        if(stop_point.accessible(required_properties)) {
            labels[0][item.rpidx] = item.arrival;
            best_labels[item.rpidx] = item.arrival;
            if(clockwise && Q[journey_pattern_point.journey_pattern_idx] > journey_pattern_point.order)
                Q[journey_pattern_point.journey_pattern_idx] = journey_pattern_point.order;
            else if(!clockwise &&  Q[journey_pattern_point.journey_pattern_idx] < journey_pattern_point.order)
                Q[journey_pattern_point.journey_pattern_idx] = journey_pattern_point.order;
            if(item.arrival != type::DateTime::min && item.arrival != type::DateTime::inf) {
                marked_sp.set(data.pt_data.journey_pattern_points[item.rpidx].stop_point_idx);
            }
        }
    }

    for(auto item : destinations) {
        const auto & sp = data.pt_data.stop_points[item.first];
        if(sp.accessible(required_properties)) {
            for(type::idx_t rpidx: sp.journey_pattern_point_list) {
                if(journey_patterns_valides.test(data.pt_data.journey_pattern_points[rpidx].journey_pattern_idx) &&
                        ((clockwise && (borne == type::DateTime::inf || best_labels[rpidx] > borne)) ||
                        ((!clockwise) && (borne == type::DateTime::min || best_labels[rpidx] < borne)))) {
                        b_dest.add_destination(rpidx, (item.second/walking_speed));
                        //if(clockwise)
                            best_labels[rpidx] = borne;
                        //else
                        //    best_labels[rpidx].departure = borne;
                    }
            }
        }
    }

     count = 1;
}


std::vector<Path>
RAPTOR::compute_all(const std::vector<std::pair<type::idx_t, double> > &departs,
                    const std::vector<std::pair<type::idx_t, double> > &destinations,
                    const type::DateTime &dt_depart,
                    const type::DateTime &borne, const float walking_speed,
                    const int walking_distance, const type::Properties &required_properties,
                    const std::vector<std::string> & forbidden,
                    bool clockwise) {
    std::vector<Path> result;
    set_journey_patterns_valides(dt_depart.date(), forbidden);

    auto calc_dep = clockwise ? departs : destinations;
    auto calc_dest = clockwise ? destinations : departs;

    std::vector<Departure_Type> departures = getDepartures(calc_dep, dt_depart, clockwise, walking_speed, data);
    clear_and_init(departures, calc_dest, borne, clockwise, walking_speed, walking_distance);

    boucleRAPTOR(required_properties, clockwise, false);

    // Aucune solution n’a pas été trouvée :'(
    if(b_dest.best_now_rpid == type::invalid_idx) {
        return result;
    }

    //Second passe : permet d’optimiser les temps de correspondance
    departures = getDepartures(calc_dep, calc_dest, !clockwise, walking_speed, labels, boardings, required_properties, data);

    for(auto departure : departures) {
        clear_and_init({departure}, calc_dep, dt_depart, !clockwise, walking_speed, walking_distance);

        boucleRAPTOR(required_properties, !clockwise, true);

        if(b_dest.best_now != type::DateTime::inf && b_dest.best_now != type::DateTime::min) {
            std::vector<Path> temp = makePathes(calc_dep, dt_depart, walking_speed, required_properties, *this, !clockwise);
            result.insert(result.end(), temp.begin(), temp.end());
        }
    }

    return result;
}


void
RAPTOR::isochrone(const std::vector<std::pair<type::idx_t, double> > &departs,
          const type::DateTime &dt_depart, const type::DateTime &borne,
          float walking_speed, int walking_distance, const type::Properties &required_properties,
          const std::vector<std::string> & forbidden,
          bool clockwise) {
    set_journey_patterns_valides(dt_depart.date(), forbidden);
    auto departures = getDepartures(departs, dt_depart, true, walking_speed, data);
    clear_and_init(departures, {}, borne, true, walking_speed, walking_distance);

    boucleRAPTOR(required_properties, clockwise, true);
}


void RAPTOR::set_journey_patterns_valides(uint32_t date, const std::vector<std::string> & forbidden) {
    journey_patterns_valides.clear();
    journey_patterns_valides.resize(data.pt_data.journey_patterns.size());
    for(const auto & journey_pattern : data.pt_data.journey_patterns) {
        const type::Route & route = data.pt_data.routes[journey_pattern.route_idx];
        const type::Line & line = data.pt_data.lines[route.line_idx];
        const type::CommercialMode & commercial_mode = data.pt_data.commercial_modes[journey_pattern.commercial_mode_idx];
        const type::Network & network = data.pt_data.networks[line.network_idx];

        // On gère la liste des interdits
        bool forbidden_journey_pattern = false;
        for(auto forbid_uri : forbidden){
            if(forbid_uri == line.uri)
                forbidden_journey_pattern = true;
            if(forbid_uri == route.uri)
                forbidden_journey_pattern = true;
            if(forbid_uri == journey_pattern.uri)
                forbidden_journey_pattern = true;
            if(forbid_uri == commercial_mode.uri)
                forbidden_journey_pattern = true;
            if(forbid_uri == network.uri)
                forbidden_journey_pattern = true;
        }

        // Si la journey_pattern n'a pas été bloquée par un paramètre, on la valide s'il y a au moins une circulation à j-1/j+1
        if(!forbidden_journey_pattern){
            for(auto vjidx : journey_pattern.vehicle_journey_list) {
                if(data.pt_data.validity_patterns[data.pt_data.vehicle_journeys[vjidx].validity_pattern_idx].check2(date)) {
                    journey_patterns_valides.set(journey_pattern.idx);
                    break;
                }
            }
        }
    }
}


struct raptor_visitor {
    bool better_or_equal(const type::DateTime &a, const type::DateTime &current_dt, const type::StopTime &st) const {
        return a <= st.section_end_date(current_dt.date(), clockwise());
    }

    std::pair<std::vector<type::JourneyPatternPoint>::const_iterator, std::vector<type::JourneyPatternPoint>::const_iterator>
    journey_pattern_points(const std::vector<type::JourneyPatternPoint> &journey_pattern_points, const type::JourneyPattern &journey_pattern, size_t order) const {
        return std::make_pair(journey_pattern_points.begin() + journey_pattern.journey_pattern_point_list[order],
                              journey_pattern_points.begin() + journey_pattern.journey_pattern_point_list.back() + 1);
    }

    std::vector<type::StopTime>::const_iterator first_stoptime(const std::vector<type::StopTime> & stop_times, size_t position) const {
        return stop_times.begin() + position;
    }    

    template<typename T1, typename T2> bool comp(const T1& a, const T2& b) const {
        return a < b;
    }

    template<typename T1, typename T2> auto combine(const T1& a, const T2& b) const -> decltype(a+b) {
        return a + b;
    }

    constexpr bool clockwise(){return true;}
    constexpr int init_queue_item() {return std::numeric_limits<int>::max();}
    constexpr type::DateTime worst_datetime(){return type::DateTime();}
    constexpr type::idx_t type::Connection::* journey_pattern_point_idx(){return &type::Connection::destination_idx;}
};


struct raptor_reverse_visitor {
    bool better_or_equal(const type::DateTime &a, const type::DateTime &current_dt, const type::StopTime &st) const {
        return a >= st.section_end_date(current_dt.date(), clockwise());
    }

    std::pair<std::vector<type::JourneyPatternPoint>::const_reverse_iterator, std::vector<type::JourneyPatternPoint>::const_reverse_iterator>
    journey_pattern_points(const std::vector<type::JourneyPatternPoint> &journey_pattern_points, const type::JourneyPattern &journey_pattern, size_t order) const {
        const auto begin = journey_pattern_points.rbegin() + journey_pattern_points.size() - journey_pattern.journey_pattern_point_list[order] - 1;
        const auto end = begin + order + 1;
        return std::make_pair(begin, end);
    }

    std::vector<type::StopTime>::const_reverse_iterator first_stoptime(const std::vector<type::StopTime> & stop_times, size_t position) const {
        return stop_times.rbegin() + stop_times.size() - position - 1;
    }

    template<typename T1, typename T2> bool comp(const T1& a, const T2& b) const {
        return a > b;
    }

    template<typename T1, typename T2> auto combine(const T1& a, const T2& b) const -> decltype(a-b) {
        return a - b;
    }

    constexpr bool clockwise() {return false;}
    constexpr int init_queue_item() {return -1;}
    constexpr type::DateTime worst_datetime(){return type::DateTime(0,0);}
    constexpr type::idx_t type::Connection::* journey_pattern_point_idx(){return &type::Connection::departure_idx;}
};


template<typename Visitor>
void RAPTOR::raptor_loop(Visitor visitor, const type::Properties &required_properties, bool global_pruning) {
    bool end = false;
    count = 0;
    type::idx_t t= type::invalid_idx;
    type::idx_t boarding = type::invalid_idx;
    type::DateTime workingDt = visitor.worst_datetime();
    uint32_t l_zone = std::numeric_limits<uint32_t>::max();

    this->foot_path(visitor, required_properties);
    while(!end /*&& count < 5*/) {
        ++count;
        end = true;
        if(count == labels.size()) {
            this->boardings.push_back(this->data.dataRaptor.boardings_const);
            if(visitor.clockwise()) {
                this->labels.push_back(this->data.dataRaptor.labels_const);
            } else {
                this->labels.push_back(this->data.dataRaptor.labels_const_reverse);
            }
        }
        const auto & prec_labels= labels[count -1];
        this->make_queue();
        for(const auto & journey_pattern : data.pt_data.journey_patterns) {
            if(Q[journey_pattern.idx] != std::numeric_limits<int>::max() && Q[journey_pattern.idx] != -1 && journey_patterns_valides.test(journey_pattern.idx)) {
                t = type::invalid_idx;
                boarding = type::invalid_idx;
                workingDt = visitor.worst_datetime();
                decltype(visitor.first_stoptime(this->data.pt_data.stop_times, 0)) it_st;
                int gap = 0;
                BOOST_FOREACH(const type::JourneyPatternPoint & jpp, visitor.journey_pattern_points(this->data.pt_data.journey_pattern_points, journey_pattern, Q[journey_pattern.idx])) {
                    if(t != type::invalid_idx) {
                        ++it_st;
                        const type::StopTime &st = *it_st;
                        if(l_zone == std::numeric_limits<uint32_t>::max() || l_zone != st.local_traffic_zone) {
                            //On stocke le meilleur label, et on marque pour explorer par la suite
                            type::DateTime bound;
                            if(visitor.comp(best_labels[jpp.idx], b_dest.best_now) || !global_pruning)
                                bound = best_labels[jpp.idx];
                            else
                                bound = b_dest.best_now;

                            auto & working_labels = this->labels[this->count];
                            workingDt.update(!st.is_frequency()? st.section_end_time(visitor.clockwise()) : st.start_time+gap, visitor.clockwise());

                            if(visitor.comp(workingDt, bound) && st.valid_end(visitor.clockwise())) {
                                working_labels[jpp.idx] = workingDt;
                                best_labels[jpp.idx] = working_labels[jpp.idx];
                                boardings[count][jpp.idx] = boarding;
                                if(!this->b_dest.add_best(jpp.idx, working_labels[jpp.idx], this->count, visitor.clockwise())) {
                                    this->marked_rp.set(jpp.idx);
                                    this->marked_sp.set(this->data.pt_data.journey_pattern_points[jpp.idx].stop_point_idx);
                                    end = false;
                                }
                            } else if(workingDt == bound &&
                                      get_type(this->count-1, jpp.idx) == boarding_type::uninitialized && st.valid_end(visitor.clockwise())) {
                                if(b_dest.add_best(jpp.idx, workingDt, this->count, visitor.clockwise())) {
                                    working_labels[jpp.idx] = workingDt;
                                    best_labels[jpp.idx] = workingDt;
                                    boardings[count][jpp.idx] = boarding;
                                }
                            }
                        }
                    }

                    //Si on peut arriver plus tôt à l'arrêt en passant par une autre journey_pattern
                    const type::DateTime & labels_temp = prec_labels[jpp.idx];
                    if(get_type(count-1, jpp.idx) != boarding_type::uninitialized &&
                       (t == type::invalid_idx || visitor.better_or_equal(labels_temp, workingDt, *it_st))) {

                        type::idx_t etemp;
                        std::tie(etemp, gap) = best_trip(jpp, labels_temp, required_properties, visitor.clockwise(), data);

                        if(etemp != type::invalid_idx && t != etemp) {
                            t = etemp;
                            boarding = data.pt_data.vehicle_journeys[t].stop_time_list[jpp.order];
                            it_st = visitor.first_stoptime(this->data.pt_data.stop_times, boarding);
                            const type::StopTime &st = *it_st;
                            workingDt = labels_temp;
                            workingDt.update(!st.is_frequency()? st.section_end_time(visitor.clockwise()) : st.start_time+gap, visitor.clockwise());
                            l_zone = it_st->local_traffic_zone;
                        }
                    }
                }
            }
            Q[journey_pattern.idx] = visitor.init_queue_item();
        }
        // Prolongements de service
        this->journey_pattern_path_connections(visitor/*, wheelchair*/);
        // Correspondances
        this->foot_path(visitor, required_properties);
    }
}


void RAPTOR::boucleRAPTOR(const type::Properties &required_properties, bool clockwise, bool global_pruning){
    if(clockwise) {
        raptor_loop(raptor_visitor(), required_properties, global_pruning);
    } else {
        raptor_loop(raptor_reverse_visitor(), required_properties, global_pruning);
    }
}


std::vector<Path> RAPTOR::compute(idx_t departure_idx, idx_t destination_idx, int departure_hour,
                                  int departure_day, type::DateTime borne, bool clockwise, const type::Properties &required_properties) {
    
    std::vector<std::pair<type::idx_t, double> > departs, destinations;

    for(type::idx_t spidx : data.pt_data.stop_areas[departure_idx].stop_point_list) {
        departs.push_back(std::make_pair(spidx, 0));
    }

    for(type::idx_t spidx : data.pt_data.stop_areas[destination_idx].stop_point_list) {
        destinations.push_back(std::make_pair(spidx, 0));
    }

    return compute_all(departs, destinations, type::DateTime(departure_day, departure_hour), borne, 1, 1000, required_properties, {}, clockwise);
}


int RAPTOR::best_round(type::idx_t journey_pattern_point_idx){
    for(size_t i = 0; i < labels.size(); ++i){
        if(labels[i][journey_pattern_point_idx] == best_labels[journey_pattern_point_idx]){
            return i;
        }
    }
    return -1;
}

}}
