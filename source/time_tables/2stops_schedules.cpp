#include "2stops_schedules.h"

namespace pt = boost::posix_time;
namespace navitia { namespace timetables {


std::vector<pair_dt_st> stops_schedule(const std::string &departure_filter, const std::string &arrival_filter,
                                        const type::DateTime &datetime, const type::DateTime &max_datetime,
                                        type::Data & data) {

    std::vector<pair_dt_st> result;
    //On va chercher tous les journey_pattern points correspondant au deuxieme filtre
    std::vector<type::idx_t> departure_journey_pattern_points = navitia::ptref::make_query(type::Type_e::JourneyPatternPoint, departure_filter, data);
    std::vector<type::idx_t> arrival_journey_pattern_points = navitia::ptref::make_query(type::Type_e::JourneyPatternPoint, arrival_filter, data);

    std::unordered_map<type::idx_t, size_t> departure_idx_arrival_order;
    for(type::idx_t idx : departure_journey_pattern_points) {
        const auto & jpp = data.pt_data.journey_pattern_points[idx];
        auto it_idx = std::find_if(arrival_journey_pattern_points.begin(), arrival_journey_pattern_points.end(),
                                  [&](type::idx_t idx2)
                                  {return data.pt_data.journey_pattern_points[idx2].journey_pattern_idx == jpp.journey_pattern_idx;});
        if(it_idx != arrival_journey_pattern_points.end() && jpp.order < data.pt_data.journey_pattern_points[*it_idx].order)
            departure_idx_arrival_order.insert(std::make_pair(idx, data.pt_data.journey_pattern_points[*it_idx].order));
    }
    //On ne garde que departure qui sont dans les deux sets, et dont l'ordre est bon.
    std::remove_if(departure_journey_pattern_points.begin(), departure_journey_pattern_points.end(),
               [&](type::idx_t idx){return departure_idx_arrival_order.find(idx) == departure_idx_arrival_order.end(); });

    //On demande tous les next_departures
    auto departure_dt_st = get_stop_times(departure_journey_pattern_points, datetime, max_datetime, std::numeric_limits<int>::max(), data);

    //On va chercher les retours
    for(auto dep_dt_st : departure_dt_st) {
        const type::StopTime &departure_st = data.pt_data.stop_times[dep_dt_st.second];
        const type::VehicleJourney vj = data.pt_data.vehicle_journeys[departure_st.vehicle_journey_idx];
        const uint32_t arrival_order = departure_idx_arrival_order[departure_st.journey_pattern_point_idx];
        const type::StopTime &arrival_st = data.pt_data.stop_times[vj.stop_time_list[arrival_order]];
        type::DateTime arrival_dt = dep_dt_st.first;
        arrival_dt.update(arrival_st.arrival_time);
        result.push_back(std::make_pair(dep_dt_st, std::make_pair(arrival_dt, arrival_st.idx)));
    }

    return result;
}



pbnavitia::Response stops_schedule(const std::string &departure_filter, const std::string &arrival_filter,
                                    const std::string &str_dt, uint32_t duration, uint32_t depth, type::Data & data) {
    pbnavitia::Response pb_response;
    pb_response.set_requested_api(pbnavitia::STOPS_SCHEDULES);

    boost::posix_time::ptime ptime;
    try{
        ptime = boost::posix_time::from_iso_string(str_dt);
    } catch(boost::bad_lexical_cast) {
        pb_response.set_error("DateTime " + str_dt + " is in a bad format should be YYYYMMddThhmmss ");
        return pb_response;
    }

    type::DateTime dt((ptime.date() - data.meta.production_date.begin()).days(), ptime.time_of_day().total_seconds());

    type::DateTime max_dt;
    max_dt = dt + duration;
    std::vector<pair_dt_st> board;
    try {
        board = stops_schedule(departure_filter, arrival_filter, dt, max_dt, data);
    } catch(ptref::parsing_error parse_error) {
        pb_response.set_error(parse_error.more);
        return pb_response;
    }

    auto current_time = pt::second_clock::local_time();
    pt::time_period action_period(to_posix_time(dt, data), to_posix_time(max_dt, data));

    for(auto pair_dt_idx : board) {
        pbnavitia::PairStopTime * pair_stoptime = pb_response.mutable_stops_schedule()->add_board_items();
        auto stoptime = pair_stoptime->mutable_first();
        const auto &dt_idx = pair_dt_idx.first;
        stoptime->set_departure_date_time(type::iso_string(dt_idx.first.date(),  dt_idx.first.hour(), data));
        stoptime->set_arrival_date_time(type::iso_string(dt_idx.first.date(),  dt_idx.first.hour(), data));
        const auto &rp = data.pt_data.journey_pattern_points[data.pt_data.stop_times[dt_idx.second].journey_pattern_point_idx];
        fill_pb_object(rp.stop_point_idx, data, stoptime->mutable_stop_point(), depth, current_time, action_period);

        stoptime = pair_stoptime->mutable_second();
        const auto &dt_idx2 = pair_dt_idx.second;
        stoptime->set_departure_date_time(type::iso_string(dt_idx2.first.date(),  dt_idx2.first.hour(), data));
        stoptime->set_arrival_date_time(type::iso_string(dt_idx2.first.date(),  dt_idx2.first.hour(), data));
        const auto &rp2 = data.pt_data.journey_pattern_points[data.pt_data.stop_times[dt_idx2.second].journey_pattern_point_idx];
        fill_pb_object(rp2.stop_point_idx, data, stoptime->mutable_stop_point(), depth, current_time, action_period);
    }
    return pb_response;
}


}}
