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
    for(auto jpp_departure_idx = marked_rp.find_first(); jpp_departure_idx != marked_rp.npos; jpp_departure_idx = marked_rp.find_next(jpp_departure_idx)) {
        const auto* jpp_departure = data.pt_data.journey_pattern_points[jpp_departure_idx];
        BOOST_FOREACH(auto &idx_rpc, data.dataRaptor.footpath_rp(visitor.clockwise()).equal_range(jpp_departure_idx)) {
            const auto & rpc = idx_rpc.second;
            const type::JourneyPatternPoint* jpp = rpc->*visitor.journey_pattern_point();
            type::idx_t jpp_idx = jpp->idx;
            type::DateTime dt = visitor.combine(labels[count][jpp_departure_idx], rpc->duration);
            if(get_type(count, jpp_departure_idx) == boarding_type::vj && visitor.comp(dt, best_labels[jpp_idx]) /*&& !wheelchair*/) {
                labels[count][jpp_idx] = dt;
                best_labels[jpp_idx] = dt;
                boardings[count][jpp_idx] = jpp_departure;
                boarding_types[count][jpp_idx] = boarding_type::connection_extension;
                to_mark.push_back(jpp_idx);
            }
        }
    }

    for(auto rp : to_mark) {
        marked_rp.set(rp);
        const auto* journey_pattern_point = data.pt_data.journey_pattern_points[rp];
        if(visitor.comp(journey_pattern_point->order, Q[journey_pattern_point->journey_pattern->idx]) ) {
            Q[journey_pattern_point->journey_pattern->idx] = journey_pattern_point->order;
        }
    }
}


template<typename Visitor>
void RAPTOR::foot_path(const Visitor & v, const type::Properties &required_properties) {

    int last = 0;
    std::vector<const type::StopPointConnection*>::const_iterator it = (v.clockwise()) ? data.dataRaptor.foot_path_forward.begin() :
                                                                                data.dataRaptor.foot_path_backward.begin();
    for(auto stop_point_idx = marked_sp.find_first(); stop_point_idx != marked_sp.npos;
        stop_point_idx = marked_sp.find_next(stop_point_idx)) {
        //On cherche le meilleur jpp du stop point
        type::DateTime best_arrival = v.worst_datetime();
        type::idx_t best_jpp = type::invalid_idx;
        const type::StopPoint* stop_point = data.pt_data.stop_points[stop_point_idx];
        if(stop_point->accessible(required_properties)) {
            for(auto journey_pattern_point : stop_point->journey_pattern_point_list) {
                type::idx_t jppidx = journey_pattern_point->idx;
                boarding_type b_type = get_type(count, jppidx);
                //On regarde si on est arrivé avec un vj ou un departure,
                //Puis on compare avec la meilleure arrivée trouvée pour ce stoppoint
                if((b_type == boarding_type::vj || b_type == boarding_type::departure) &&
                    v.comp(labels[count][jppidx], best_arrival)) {
                    best_arrival = labels[count][jppidx];
                    best_jpp = jppidx;
                }
            }
            // Si on a trouvé un journey pattern pour ce stop point
            // NB : l'inverse ne devrait jamais arrivé
            if(best_jpp != type::invalid_idx) {
                const type::DateTime best_departure = v.combine(best_arrival, 120);
                //On marque tous les journey_pattern points du stop point
                for(auto jpp : stop_point->journey_pattern_point_list) {
                    type::idx_t jpp_idx = jpp->idx;
                    if(jpp_idx != best_jpp && v.comp(best_departure, best_labels[jpp_idx]) ) {
                       best_labels[jpp_idx] = best_departure;
                       labels[count][jpp_idx] = best_departure;
                       boardings[count][jpp_idx] = data.pt_data.journey_pattern_points[best_jpp];
                       boarding_types[count][jpp_idx] = boarding_type::connection;

                       if(!b_dest.add_best(jpp_idx, best_departure, count, v.clockwise()) && v.comp(jpp->order, Q[jpp->journey_pattern->idx]) ) {
                           Q[jpp->journey_pattern->idx] = jpp->order;
                       }
                    }
                }


                //On va maintenant chercher toutes les connexions et on marque tous les journey_pattern_points concernés

                //On récupère l'index dans les footpath
                const pair_int & index = (v.clockwise()) ? data.dataRaptor.footpath_index_forward[stop_point_idx] :
                                                         data.dataRaptor.footpath_index_backward[stop_point_idx];

                int prec_duration = -1;
                type::DateTime next = v.worst_datetime(),
                               previous = labels[count][best_jpp];


                it += index.first - last;
                const auto end = it + index.second;

                for(; it != end; ++it) {
                    const type::StopPointConnection* spc = *it;
                    const auto destination = spc->destination;
                    if(destination->accessible(required_properties)) {
                        for(auto destination_jpp : destination->journey_pattern_point_list) {
                            type::idx_t destination_jpp_idx = destination_jpp->idx;
                            if(best_jpp != destination_jpp_idx) {
                                if(spc->duration != prec_duration) {
                                    next = v.combine(previous, spc->duration);
                                    prec_duration = spc->duration;
                                }
                                if(v.comp(next, best_labels[destination_jpp_idx]) || next == best_labels[destination_jpp_idx]) {
                                    best_labels[destination_jpp_idx] = next;
                                    labels[count][destination_jpp_idx] = next;
                                    boardings[count][destination_jpp_idx] = data.pt_data.journey_pattern_points[best_jpp];
                                    boarding_types[count][destination_jpp_idx] = boarding_type::connection;

                                    if(!b_dest.add_best(destination_jpp_idx, next, count, v.clockwise())
                                           && v.comp(destination_jpp->order, Q[destination_jpp->journey_pattern->idx])) {
                                        Q[destination_jpp->journey_pattern->idx] = destination_jpp->order;
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
    boardings =  {std::vector<const type::JourneyPatternPoint*>(data.pt_data.journey_pattern_points.size(), nullptr)};
    boarding_types = {std::vector<boarding_type>(data.pt_data.journey_pattern_points.size(), boarding_type::uninitialized)};
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
                  type::DateTime bound,  const bool clockwise,
                  const float walking_speed, const int walking_distance,
                  const type::Properties &required_properties) {

    this->clear(data, clockwise, bound, std::ceil(walking_distance/walking_speed));

    for(Departure_Type item : departs) {
        const type::JourneyPatternPoint* journey_pattern_point = data.pt_data.journey_pattern_points[item.rpidx];
        const type::StopPoint* stop_point = journey_pattern_point->stop_point;
        if(stop_point->accessible(required_properties)) {
            labels[0][item.rpidx] = item.arrival;
            best_labels[item.rpidx] = item.arrival;
            boarding_types[0][item.rpidx] = boarding_type::departure;
            if(clockwise && Q[journey_pattern_point->journey_pattern->idx] > journey_pattern_point->order)
                Q[journey_pattern_point->journey_pattern->idx] = journey_pattern_point->order;
            else if(!clockwise &&  Q[journey_pattern_point->journey_pattern->idx] < journey_pattern_point->order)
                Q[journey_pattern_point->journey_pattern->idx] = journey_pattern_point->order;
            if(item.arrival != type::DateTime::min && item.arrival != type::DateTime::inf) {
                marked_sp.set(stop_point->idx);
            }
        }
    }

    for(auto item : destinations) {
        const type::StopPoint* sp = data.pt_data.stop_points[item.first];
        if(sp->accessible(required_properties)) {
            for(auto journey_pattern_point : sp->journey_pattern_point_list) {
                type::idx_t jppidx = journey_pattern_point->idx;
                if(journey_patterns_valides.test(journey_pattern_point->journey_pattern->idx) &&
                        ((clockwise && (bound == type::DateTime::inf || best_labels[jppidx] > bound)) ||
                        ((!clockwise) && (bound == type::DateTime::min || best_labels[jppidx] < bound)))) {
                        b_dest.add_destination(jppidx, (item.second/walking_speed));
                        //if(clockwise)
                            best_labels[jppidx] = bound;
                        //else
                        //    best_labels[rpidx].departure = borne;
                    }
            }
        }
    }

     count = 1;
}


std::vector<Path>
RAPTOR::compute_all(const std::vector<std::pair<type::idx_t, double> > &departures_,
                    const std::vector<std::pair<type::idx_t, double> > &destinations,
                    const type::DateTime &departure_datetime, const type::DateTime &bound,
                    const uint32_t max_transfers, const float walking_speed,
                    const int walking_distance, const type::Properties &required_properties,
                    const std::vector<std::string> & forbidden,
                    bool clockwise) {
    std::vector<Path> result;
    set_journey_patterns_valides(departure_datetime.date(), forbidden);

    auto calc_dep = clockwise ? departures_ : destinations;
    auto calc_dest = clockwise ? destinations : departures_;

    std::vector<Departure_Type> departures = getDepartures(calc_dep, departure_datetime, clockwise, walking_speed, data);
    clear_and_init(departures, calc_dest, bound, clockwise, walking_speed, walking_distance);

    boucleRAPTOR(required_properties, clockwise, false, max_transfers);

    // Aucune solution n’a pas été trouvée :'(
    if(b_dest.best_now_rpid == type::invalid_idx) {
        return result;
    }

    //Second passe : permet d’optimiser les temps de correspondance
    departures = getDepartures(calc_dep, calc_dest, !clockwise, walking_speed, labels, boardings, boarding_types, required_properties, data);

    for(auto departure : departures) {
        clear_and_init({departure}, calc_dep, departure_datetime, !clockwise, walking_speed, walking_distance);

        boucleRAPTOR(required_properties, !clockwise, true, max_transfers);

        if(b_dest.best_now_rpid != type::invalid_idx) {
            std::vector<Path> temp = makePathes(calc_dep, departure_datetime, walking_speed, required_properties, *this, !clockwise);
            result.insert(result.end(), temp.begin(), temp.end());
        }
    }

    return result;
}


void
RAPTOR::isochrone(const std::vector<std::pair<type::idx_t, double> > &departures_,
          const type::DateTime &departure_datetime, const type::DateTime &bound, uint32_t max_transfers,
          float walking_speed, int walking_distance, const type::Properties &required_properties,
          const std::vector<std::string> & forbidden,
          bool clockwise) {
    set_journey_patterns_valides(departure_datetime.date(), forbidden);
    auto departures = getDepartures(departures_, departure_datetime, true, walking_speed, data);
    clear_and_init(departures, {}, bound, true, walking_speed, walking_distance);

    boucleRAPTOR(required_properties, clockwise, true, max_transfers);
}


void RAPTOR::set_journey_patterns_valides(uint32_t date, const std::vector<std::string> & forbidden) {
    journey_patterns_valides.clear();
    journey_patterns_valides.resize(data.pt_data.journey_patterns.size());
    for(const type::JourneyPattern* journey_pattern : data.pt_data.journey_patterns) {
        const type::Line* line = journey_pattern->route->line;

        // On gère la liste des interdits
        bool forbidden_journey_pattern = false;
        for(auto forbid_uri : forbidden){
            if(forbid_uri == line->uri)
                forbidden_journey_pattern = true;
            if(forbid_uri == journey_pattern->route->uri)
                forbidden_journey_pattern = true;
            if(forbid_uri == journey_pattern->uri)
                forbidden_journey_pattern = true;
            if(forbid_uri == journey_pattern->commercial_mode->uri)
                forbidden_journey_pattern = true;
            if(forbid_uri == line->network->uri)
                forbidden_journey_pattern = true;
        }

        // Si la journey_pattern n'a pas été bloquée par un paramètre, on la valide s'il y a au moins une circulation à j-1/j+1
        if(!forbidden_journey_pattern){
            for(const type::VehicleJourney* vehicle_journey : journey_pattern->vehicle_journey_list) {
                if(vehicle_journey->validity_pattern->check2(date)) {
                    journey_patterns_valides.set(journey_pattern->idx);
                    break;
                }
            }
        }
    }
}


struct raptor_visitor {
    bool better_or_equal(const type::DateTime &a, const type::DateTime &current_dt, const type::StopTime* st) const {
        return a <= st->section_end_date(current_dt.date(), clockwise());
    }

    std::pair<std::vector<type::JourneyPatternPoint*>::const_iterator, std::vector<type::JourneyPatternPoint*>::const_iterator>
    journey_pattern_points(const std::vector<type::JourneyPatternPoint*> &/*journey_pattern_points*/, const type::JourneyPattern* journey_pattern, size_t order) const {
        return std::make_pair(journey_pattern->journey_pattern_point_list.begin() + order,
                              journey_pattern->journey_pattern_point_list.end());
    }

    typedef std::vector<type::StopTime*>::const_iterator stop_time_iterator;
    stop_time_iterator first_stoptime(const type::StopTime* st) const {
        const type::JourneyPatternPoint* jpp = st->journey_pattern_point;
        const type::VehicleJourney* vj = st->vehicle_journey;
        return vj->stop_time_list.begin() + jpp->order;
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
    constexpr type::JourneyPatternPoint* type::JourneyPatternPointConnection::* journey_pattern_point(){return &type::JourneyPatternPointConnection::destination;}
};


struct raptor_reverse_visitor {
    bool better_or_equal(const type::DateTime &a, const type::DateTime &current_dt, const type::StopTime* st) const {
        return a >= st->section_end_date(current_dt.date(), clockwise());
    }

    std::pair<std::vector<type::JourneyPatternPoint*>::const_reverse_iterator, std::vector<type::JourneyPatternPoint*>::const_reverse_iterator>
    journey_pattern_points(const std::vector<type::JourneyPatternPoint*> &/*journey_pattern_points*/, const type::JourneyPattern* journey_pattern, size_t order) const {
        size_t offset = journey_pattern->journey_pattern_point_list.size() - order - 1;
        const auto begin = journey_pattern->journey_pattern_point_list.rbegin() + offset;
        const auto end = journey_pattern->journey_pattern_point_list.rend();
        return std::make_pair(begin, end);
    }

    typedef std::vector<type::StopTime*>::const_reverse_iterator stop_time_iterator;
    stop_time_iterator first_stoptime(const type::StopTime* st) const {
        const type::JourneyPatternPoint* jpp = st->journey_pattern_point;
        const type::VehicleJourney* vj = st->vehicle_journey;
        return vj->stop_time_list.rbegin() + vj->stop_time_list.size() - jpp->order - 1;
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
    constexpr type::JourneyPatternPoint* type::JourneyPatternPointConnection::* journey_pattern_point(){return &type::JourneyPatternPointConnection::departure;}
};


template<typename Visitor>
void RAPTOR::raptor_loop(Visitor visitor, const type::Properties &required_properties, bool global_pruning, uint32_t max_transfers) {
    bool end = false;
    count = 0; //< Itération de l'algo raptor (une itération par correspondance)
    const type::JourneyPatternPoint* boarding = nullptr; //< Le JPP time auquel on a embarqué
    type::DateTime workingDt = visitor.worst_datetime();
    uint32_t l_zone = std::numeric_limits<uint32_t>::max();

    this->foot_path(visitor, required_properties);
    while(!end && count <= max_transfers) {
        ++count;
        end = true;
        if(count == labels.size()) {
            if(visitor.clockwise()) {
                this->labels.push_back(this->data.dataRaptor.labels_const);
            } else {
                this->labels.push_back(this->data.dataRaptor.labels_const_reverse);
            }
            boardings.push_back(std::vector< const type::JourneyPatternPoint* >(data.pt_data.journey_pattern_points.size(), nullptr));
            boarding_types.push_back(std::vector<boarding_type>(data.pt_data.journey_pattern_points.size(), boarding_type::uninitialized));
        }
        const auto & prec_labels=labels[count -1];
        auto & working_labels = labels[this->count];
        this->make_queue();
        for(const auto & journey_pattern : data.pt_data.journey_patterns) {
            if(Q[journey_pattern->idx] != std::numeric_limits<int>::max() && Q[journey_pattern->idx] != -1 && journey_patterns_valides.test(journey_pattern->idx)) {
                boarding = nullptr;
                workingDt = visitor.worst_datetime();
                typename Visitor::stop_time_iterator it_st;
                int gap = 0;
                BOOST_FOREACH(const type::JourneyPatternPoint* jpp, visitor.journey_pattern_points(this->data.pt_data.journey_pattern_points, journey_pattern, Q[journey_pattern->idx])) {
                    type::idx_t jpp_idx = jpp->idx;
                    if(boarding != nullptr) {
                        ++it_st;
                        const type::StopTime* st = *it_st;
                        if(l_zone == std::numeric_limits<uint32_t>::max() || l_zone != st->local_traffic_zone) {
                            //On stocke le meilleur label, et on marque pour explorer par la suite
                            type::DateTime bound;
                            if(visitor.comp(best_labels[jpp_idx], b_dest.best_now) || !global_pruning)
                                bound = best_labels[jpp_idx];
                            else
                                bound = b_dest.best_now;

                            workingDt.update(!st->is_frequency()? st->section_end_time(visitor.clockwise()) : st->start_time+gap, visitor.clockwise());

                            if(visitor.comp(workingDt, bound) && st->valid_end(visitor.clockwise())) {
                                working_labels[jpp_idx] = workingDt;
                                best_labels[jpp_idx] = working_labels[jpp_idx];
                                boardings[count][jpp_idx] = boarding;
                                boarding_types[count][jpp_idx] = boarding_type::vj;
                                if(!this->b_dest.add_best(jpp_idx, working_labels[jpp_idx], this->count, visitor.clockwise())) {
                                    this->marked_rp.set(jpp_idx);
                                    this->marked_sp.set(this->data.pt_data.journey_pattern_points[jpp_idx]->stop_point->idx);
                                    end = false;
                                }
                            } else if(workingDt == bound &&
                                      get_type(this->count-1, jpp_idx) == boarding_type::uninitialized && st->valid_end(visitor.clockwise())) {
                                if(b_dest.add_best(jpp_idx, workingDt, this->count, visitor.clockwise())) {
                                    working_labels[jpp_idx] = workingDt;
                                    best_labels[jpp_idx] = workingDt;
                                    boardings[count][jpp_idx] = boarding;
                                    boarding_types[count][jpp_idx] = boarding_type::vj;
                                }
                            }
                        }
                    }

                    //Si on peut arriver plus tôt à l'arrêt en passant par une autre journey_pattern
                    const type::DateTime & labels_temp = prec_labels[jpp_idx];
                    const boarding_type b_type = get_type(count-1, jpp_idx);
                    if(b_type != boarding_type::uninitialized && b_type != boarding_type::vj &&
                       (boarding == nullptr || visitor.better_or_equal(labels_temp, workingDt, *it_st))) {

                        const type::StopTime* temp_stop_time;
                        std::tie(temp_stop_time, gap) = best_stop_time(jpp, labels_temp, required_properties, visitor.clockwise(), data);

                        if(temp_stop_time != nullptr) {
                            boarding = /*temp_stop_time->journey_pattern_point*/jpp;
                            it_st = visitor.first_stoptime(temp_stop_time);
                            const type::StopTime* st = *it_st;
                            workingDt = labels_temp;
                            workingDt.update(!st->is_frequency()? st->section_end_time(visitor.clockwise()) : st->start_time+gap, visitor.clockwise());
                            l_zone = st->local_traffic_zone;
                        }
                    }
                }
            }
            Q[journey_pattern->idx] = visitor.init_queue_item();
        }
        // Prolongements de service
        this->journey_pattern_path_connections(visitor/*, wheelchair*/);
        // Correspondances
        this->foot_path(visitor, required_properties);
    }
}


void RAPTOR::boucleRAPTOR(const type::Properties &required_properties, bool clockwise, bool global_pruning, uint32_t max_transfers){
    if(clockwise) {
        raptor_loop(raptor_visitor(), required_properties, global_pruning, max_transfers);
    } else {
        raptor_loop(raptor_reverse_visitor(), required_properties, global_pruning, max_transfers);
    }
}


std::vector<Path> RAPTOR::compute(idx_t departure_idx, idx_t destination_idx, int departure_hour,
                                  int departure_day, type::DateTime borne, bool clockwise,
                                  const type::Properties &required_properties, uint32_t max_transfers) {
    
    std::vector<std::pair<type::idx_t, double> > departures, destinations;

    for(const type::StopPoint* sp : data.pt_data.stop_areas[departure_idx]->stop_point_list) {
        departures.push_back(std::make_pair(sp->idx, 0));
    }

    for(const type::StopPoint* sp : data.pt_data.stop_areas[destination_idx]->stop_point_list) {
        destinations.push_back(std::make_pair(sp->idx, 0));
    }

    return compute_all(departures, destinations, type::DateTime(departure_day, departure_hour), borne, max_transfers, 1, 1000, required_properties, {}, clockwise);
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
