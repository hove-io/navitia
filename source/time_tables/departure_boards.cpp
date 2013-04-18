#include "departure_boards.h"
#include "request_handle.h"
#include "get_stop_times.h"
#include "type/pb_converter.h"

#include "boost/lexical_cast.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

namespace pt = boost::posix_time;

namespace navitia { namespace timetables {

std::vector<vector_datetime> make_columuns(const vector_dt_st &stop_times) {
    std::vector<vector_datetime> result;
    type::DateTime prev_date = type::DateTime::inf;

    for(auto & item : stop_times) {
        if(prev_date == type::DateTime:: inf || (prev_date.hour()/3600) != (item.first.hour()/3600)) {
            //On supprime les doublons
            if(result.size() > 0) {
                auto it = std::unique(result.back().begin(), result.back().end());
                result.back().resize(std::distance(result.back().begin(), it));
            }
            result.push_back(vector_datetime());
        }

        result.back().push_back(item.first);
        prev_date = item.first;
    }


    if(result.size() > 0) {
        auto it = std::unique(result.back().begin(), result.back().end());
        result.back().resize(std::distance(result.back().begin(), it));
    }

    return result;
}

pbnavitia::Response departure_board(const std::string &request, const std::string &date, uint32_t duration, const type::Data &data) {

    RequestHandle handler("DEPARTURE_BOARD", request, date,  duration, data);


    handler.pb_response.set_requested_api(pbnavitia::DEPARTURE_BOARDS);
    if(handler.pb_response.has_error())
        return handler.pb_response;

    if(handler.journey_pattern_points.size() == 0)
        return handler.pb_response;

    pbnavitia::DepartureBoard * dep_board = handler.pb_response.mutable_departure_board();

    std::map<stop_point_line, vector_dt_st> map_route_stop_point;
    // On regroupe entre eux les stop_times appartenant au meme couple (stop_point, route)
    // On veut en effet afficher les départs regroupés par route (une route étant une vague direction commerciale
    for(type::idx_t journey_pattern_point_idx : handler.journey_pattern_points) {
        auto stop_point_idx = data.pt_data.journey_pattern_points[journey_pattern_point_idx].stop_point_idx;
        auto route_idx = data.pt_data.journey_patterns[data.pt_data.journey_pattern_points[journey_pattern_point_idx].journey_pattern_idx].route_idx;

        auto stop_times = get_stop_times({journey_pattern_point_idx}, handler.date_time, handler.max_datetime, std::numeric_limits<int>::max(), data);

        auto key = std::make_pair(stop_point_idx, route_idx);
        auto iter = map_route_stop_point.find(key);
        if(iter == map_route_stop_point.end()) {
            iter = map_route_stop_point.insert(std::make_pair(key, vector_dt_st())).first;
        }

        iter->second.insert(iter->second.end(), stop_times.begin(), stop_times.end());
    }

    auto current_time = pt::second_clock::local_time();
    pt::time_period action_period(to_posix_time(handler.date_time, data), to_posix_time(handler.max_datetime, data));

    for(auto id_vec : map_route_stop_point) {

        auto board = dep_board->add_boards();
        fill_pb_object(id_vec.first.first, data, board->mutable_stop_point(), 1, current_time, action_period);
        fill_pb_object(id_vec.first.second, data, board->mutable_route(), 2, current_time, action_period);

        auto vec_st = id_vec.second;
        std::sort(vec_st.begin(), vec_st.end(),
                  [&](dt_st d1, dt_st d2) {
                    return std::abs((d1.first.hour() % type::DateTime::SECONDS_PER_DAY)-handler.date_time.hour())
                        <  std::abs((d2.first.hour() % type::DateTime::SECONDS_PER_DAY)-handler.date_time.hour());
                  });

        for(auto vec : make_columuns(vec_st)) {
            pbnavitia::BoardItem *item = board->add_board_items();

            for(type::DateTime dt : vec) {
                if(!item->has_hour())
                    item->set_hour(boost::lexical_cast<std::string>(dt.hour()/3600));
                item->add_minutes(boost::lexical_cast<std::string>((dt.hour()%3600)/60));
            }
        }
    }

    return handler.pb_response;
}





}

}
