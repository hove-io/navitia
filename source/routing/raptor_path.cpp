#include "raptor_path.h"
#include "raptor.h"


namespace navitia { namespace routing {


std::vector<Path>
makePathes(std::vector<std::pair<type::idx_t, boost::posix_time::time_duration> > destinations,
           DateTime dt,
           const type::AccessibiliteParams & accessibilite_params, const RAPTOR &raptor_, bool clockwise, bool disruption_active) {
    std::vector<Path> result;
    DateTime best_dt = clockwise ? DateTimeUtils::inf : DateTimeUtils::min;
    for(unsigned int i=1;i<=raptor_.count;++i) {
        type::idx_t best_jpp = type::invalid_idx;
        for(auto spid_dist : destinations) {
            for(auto dest : raptor_.data.pt_data.stop_points[spid_dist.first]->journey_pattern_point_list) {
                type::idx_t dest_idx = dest->idx;
                if(raptor_.get_type(i, dest_idx) == boarding_type::vj) {
                    DateTime current_dt = raptor_.labels[i][dest_idx].dt;
                    if(clockwise)
                        current_dt = current_dt + spid_dist.second.total_seconds();
                    else
                        current_dt = current_dt - spid_dist.second.total_seconds();
                    if((clockwise && ((best_dt == DateTimeUtils::inf && current_dt <= dt) || (best_dt != DateTimeUtils::inf && current_dt < best_dt)))
                       ||(!clockwise && ((best_dt == DateTimeUtils::min && current_dt >= dt) || (best_dt != DateTimeUtils::min && current_dt > best_dt))) ){
                        best_dt = current_dt ;
                        best_jpp = dest_idx;
                    }
                }
            }
        }
        if(best_jpp != type::invalid_idx)
            result.push_back(makePath(best_jpp, i, clockwise, disruption_active, accessibilite_params, raptor_));
    }

    return result;
}

std::pair<const type::StopTime*, uint32_t>
get_current_stidx_gap(size_t count, type::idx_t journey_pattern_point, const std::vector<label_vector_t> &labels,
                      const type::AccessibiliteParams & accessibilite_params, bool clockwise,  const navitia::type::Data &data, bool disruption_active) {
    const auto& label = labels[count][journey_pattern_point];
    if(label.type == boarding_type::vj) {
        const type::JourneyPatternPoint* jpp = data.pt_data.journey_pattern_points[journey_pattern_point];
        return best_stop_time(jpp, label.dt, accessibilite_params.vehicle_properties, clockwise, disruption_active, data, true);
    }
    return std::make_pair(nullptr, std::numeric_limits<uint32_t>::max());
}


Path
makePath(type::idx_t destination_idx, unsigned int countb, bool clockwise, bool disruption_active,
         const type::AccessibiliteParams & accessibilite_params,
         const RAPTOR &raptor_) {
    Path result;
    unsigned int current_jpp_idx = destination_idx;
    DateTime l = raptor_.labels[countb][current_jpp_idx].dt,
                   workingDate = l;

    const type::StopTime* current_st;
    type::idx_t boarding_jpp = navitia::type::invalid_idx;

    bool stop = false;

    PathItem item;
    // On boucle tant
    while(!stop) {
        // Est-ce qu'on a une section marche à pied ?
        if(raptor_.get_type(countb, current_jpp_idx) == boarding_type::connection ||
           raptor_.get_type(countb, current_jpp_idx) == boarding_type::connection_stay_in ||
           raptor_.get_type(countb, current_jpp_idx) == boarding_type::connection_guarantee) {
            auto departure = raptor_.data.pt_data.journey_pattern_points[current_jpp_idx]->stop_point;
            auto destination_jpp = raptor_.data.pt_data.journey_pattern_points[raptor_.get_boarding_jpp(countb, current_jpp_idx)->idx];
            auto destination = destination_jpp->stop_point;
            auto connections = departure->stop_point_connection_list;
            l = raptor_.labels[countb][current_jpp_idx].dt;
            auto find_predicate = [&](type::StopPointConnection* connection)->bool {
                return departure == connection->departure && destination == connection->destination;
            };

            auto it = std::find_if(connections.begin(), connections.end(), find_predicate);
            if(it == connections.end()) {
                auto r2 = raptor_.labels[countb][raptor_.get_boarding_jpp(countb, current_jpp_idx)->idx];
                if(clockwise) {
                   item = PathItem(navitia::to_posix_time(r2.dt, raptor_.data), navitia::to_posix_time(l, raptor_.data));
                } else {
                   item = PathItem(navitia::to_posix_time(l, raptor_.data), navitia::to_posix_time(r2.dt, raptor_.data));
                }
            } else {
                const auto stop_point_connection = *it;
                if(clockwise) {
                    item = PathItem(navitia::to_posix_time(l - stop_point_connection->display_duration, raptor_.data), navitia::to_posix_time(l, raptor_.data));
                } else {
                    item = PathItem(navitia::to_posix_time(l, raptor_.data), navitia::to_posix_time(l + stop_point_connection->display_duration, raptor_.data));
                }
                item.connection = stop_point_connection;
            }
            item.stop_points.push_back(departure);
            item.stop_points.push_back(destination);
            if(raptor_.get_type(countb, current_jpp_idx) == boarding_type::connection)
                item.type = walking;
            else if(raptor_.get_type(countb, current_jpp_idx) == boarding_type::connection_stay_in)
                item.type = stay_in;
            else if(raptor_.get_type(countb, current_jpp_idx) == boarding_type::connection_guarantee)
                item.type = guarantee;

            //BOOST_ASSERT(item.arrival >= item.departure);
            //BOOST_ASSERT(result.items.empty() || !clockwise && (result.items.back().arrival >= item.departure));
            //BOOST_ASSERT(result.items.empty() || clockwise &&  (result.items.back().arrival <= item.departure));
            result.items.push_back(item);
            boarding_jpp = type::invalid_idx;
            current_jpp_idx = raptor_.get_boarding_jpp(countb, current_jpp_idx)->idx;
        } else { // Sinon c'est un trajet TC
            // Est-ce que qu'on a à faire à un nouveau trajet ?
            if(boarding_jpp == type::invalid_idx) {
                l = raptor_.labels[countb][current_jpp_idx].dt;
                //BOOST_ASSERT(result.items.empty() || !clockwise && (l <= result.items.back().arrival));
                //BOOST_ASSERT(result.items.empty() || clockwise &&  (l >= result.items.back().arrival));
                boarding_jpp = raptor_.get_boarding_jpp(countb, current_jpp_idx)->idx;

                std::tie(current_st, workingDate) = get_current_stidx_gap(countb, current_jpp_idx, raptor_.labels, accessibilite_params, clockwise,  raptor_.data, disruption_active) ;
                item = PathItem();
                item.type = public_transport;
                while(boarding_jpp != current_jpp_idx) {
                    //On stocke le sp, et les temps
                    item.stop_points.push_back(raptor_.data.pt_data.journey_pattern_points[current_jpp_idx]->stop_point);
                    item.stop_times.push_back(current_st);
                    if(clockwise) {
                        if(current_st->is_frequency())
                            DateTimeUtils::update(workingDate, current_st->f_departure_time(DateTimeUtils::hour(workingDate), !clockwise), !clockwise);
                        else
                            DateTimeUtils::update(workingDate, current_st->departure_time, !clockwise);
                        item.departures.push_back(navitia::to_posix_time(workingDate, raptor_.data));
                        if(current_st->is_frequency())
                            DateTimeUtils::update(workingDate, current_st->f_arrival_time(DateTimeUtils::hour(workingDate), !clockwise), !clockwise);
                        else
                            DateTimeUtils::update(workingDate, current_st->arrival_time, !clockwise);
                        item.arrivals.push_back(navitia::to_posix_time(workingDate, raptor_.data));
                    } else {
                        if(current_st->is_frequency())
                            DateTimeUtils::update(workingDate, current_st->f_arrival_time(DateTimeUtils::hour(workingDate), !clockwise), !clockwise);
                        else
                            DateTimeUtils::update(workingDate, current_st->arrival_time, !clockwise);
                        item.arrivals.push_back(navitia::to_posix_time(workingDate, raptor_.data));
                        if(current_st->is_frequency())
                            DateTimeUtils::update(workingDate, current_st->f_departure_time(DateTimeUtils::hour(workingDate), !clockwise), !clockwise);
                        else
                            DateTimeUtils::update(workingDate, current_st->departure_time, !clockwise);
                        item.departures.push_back(navitia::to_posix_time(workingDate, raptor_.data));
                    }

                    size_t order = current_st->journey_pattern_point->order;
                    // On parcourt les données dans le sens contraire du calcul
                    if(clockwise){
                        BOOST_ASSERT(order>0);
                        order--;
                    }
                    else{
                        order++;
                        BOOST_ASSERT(order < current_st->vehicle_journey->stop_time_list.size());
                    }

                    current_st = current_st->vehicle_journey->stop_time_list[order];
                    current_jpp_idx = current_st->journey_pattern_point->idx;
                }
                // Je stocke le dernier stop point, et ses temps d'arrivée et de départ
                item.stop_points.push_back(raptor_.data.pt_data.journey_pattern_points[current_jpp_idx]->stop_point);
                item.stop_times.push_back(current_st);

                if(clockwise) {
                    if(current_st->is_frequency())
                        DateTimeUtils::update(workingDate, current_st->f_departure_time(DateTimeUtils::hour(workingDate)), !clockwise);
                    else
                        DateTimeUtils::update(workingDate, current_st->departure_time, !clockwise);
                    item.departures.push_back(navitia::to_posix_time(workingDate, raptor_.data));
                    if(current_st->is_frequency())
                        DateTimeUtils::update(workingDate, current_st->f_arrival_time(DateTimeUtils::hour(workingDate)), !clockwise);
                    else
                        DateTimeUtils::update(workingDate, current_st->arrival_time, !clockwise);

                    item.arrivals.push_back(navitia::to_posix_time(workingDate, raptor_.data));
                    item.arrival = item.arrivals.front();
                    item.departure = item.departures.back();
                } else {
                    if(current_st->is_frequency())
                        DateTimeUtils::update(workingDate, current_st->f_arrival_time(DateTimeUtils::hour(workingDate)), !clockwise);
                    else
                        DateTimeUtils::update(workingDate, current_st->arrival_time, !clockwise);
                    item.arrivals.push_back(navitia::to_posix_time(workingDate, raptor_.data));
                    if(current_st->is_frequency())
                        DateTimeUtils::update(workingDate, current_st->f_departure_time(DateTimeUtils::hour(workingDate)), !clockwise);
                    else
                        DateTimeUtils::update(workingDate, current_st->departure_time, !clockwise);

                    item.departures.push_back(navitia::to_posix_time(workingDate, raptor_.data));
                    item.arrival = item.arrivals.back();
                    item.departure = item.departures.front();
                }

                //On stocke l'item créé
                BOOST_ASSERT(item.arrival >= item.departure);
                BOOST_ASSERT(result.items.empty() || !clockwise || (result.items.back().arrival >= item.departure));
                BOOST_ASSERT(result.items.empty() || clockwise ||  (result.items.back().arrival <= item.departure));
                result.items.push_back(item);

                --countb;
                boarding_jpp = navitia::type::invalid_idx ;

            }
        }
        if(raptor_.get_type(countb, current_jpp_idx) == boarding_type::departure)
            stop = true;

    }

    if(clockwise){
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
        result.duration = boost::posix_time::seconds(0);

    result.nb_changes = 0;
    if(result.items.size() > 2) {
        for(unsigned int i = 1; i <= (result.items.size()-2); ++i) {
            if(result.items[i].type == walking)
                ++ result.nb_changes;
        }
    }
    patch_datetimes(result);

    return result;
}

void patch_datetimes(Path &path){
    PathItem previous_item = *path.items.begin();
    std::vector<std::pair<int, PathItem>> to_insert;
    for(auto item = path.items.begin() + 1; item!= path.items.end(); ++item) {
        if(previous_item.departure != boost::posix_time::pos_infin) {
            if(item->type == walking || item->type == stay_in || item->type == guarantee) {
                auto duration = item->arrival - item->departure;
                item->departure = previous_item.arrival;
                item->arrival = item->departure + duration;
            } else {
                if(previous_item.type != stay_in){
                    PathItem waitingItem=PathItem();
                    waitingItem.departure = previous_item.arrival;
                    waitingItem.arrival = item->departure;
                    waitingItem.type = waiting;
                    waitingItem.stop_points.push_back(previous_item.stop_points.front());
                    to_insert.push_back(std::make_pair(item-path.items.begin(), waitingItem));
                    BOOST_ASSERT(previous_item.arrival <= waitingItem.departure);
                    BOOST_ASSERT(waitingItem.arrival <= item->departure);
                    BOOST_ASSERT(previous_item.arrival <= item->departure);
                }
            }
        }
        previous_item = *item;
    }

    std::reverse(to_insert.begin(), to_insert.end());
    for(auto pos_value : to_insert) {
        path.items.insert(path.items.begin()+pos_value.first, pos_value.second);
    }

    //Deletion of waiting items when departure = destination, and update of next waiting item
    auto previous_it = path.items.begin();
    std::vector<size_t> to_delete;
    for(auto item = path.items.begin() + 1; item!= path.items.end(); ++item) {
        if(previous_it->departure != boost::posix_time::pos_infin) {
            if((previous_it->type == walking || previous_it->type == guarantee)
                    && (previous_it->stop_points.front() == previous_it->stop_points.back())) {
                item->departure = previous_it->departure;
                to_delete.push_back(previous_it-path.items.begin());
            }
        }
        previous_it = item;
    }
    std::reverse(to_delete.begin(), to_delete.end());
    for(auto pos_value : to_delete) {
        path.items.erase(path.items.begin() + pos_value);
    }
}


Path
makePathreverse(unsigned int destination_idx, unsigned int countb,
                const type::AccessibiliteParams & accessibilite_params,
                const RAPTOR &raptor_, bool disruption_active) {
    return makePath(destination_idx, countb, disruption_active, false, accessibilite_params, raptor_);
}


}}
