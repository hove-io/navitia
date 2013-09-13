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
    DateTime prev_date = DateTimeUtils::inf;

    for(auto & item : stop_times) {
        if(prev_date == DateTimeUtils::inf || (DateTimeUtils::hour(prev_date)/3600) != (DateTimeUtils::hour(item.first)/3600)) {
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

    if(handler.pb_response.has_error())
        return handler.pb_response;

    if(handler.journey_pattern_points.size() == 0)
        return handler.pb_response;

    std::map<stop_point_line, vector_dt_st> map_route_stop_point;
    // On regroupe entre eux les stop_times appartenant au meme couple (stop_point, route)
    // On veut en effet afficher les départs regroupés par route (une route étant une vague direction commerciale
    for(type::idx_t journey_pattern_point_idx : handler.journey_pattern_points) {
        const type::StopPoint* stop_point = data.pt_data.journey_pattern_points[journey_pattern_point_idx]->stop_point;
        const type::Route* route = data.pt_data.journey_pattern_points[journey_pattern_point_idx]->journey_pattern->route;

        auto stop_times = get_stop_times({journey_pattern_point_idx}, handler.date_time, handler.max_datetime, std::numeric_limits<int>::max(), data);

        auto key = std::make_pair(stop_point->idx, route->idx);
        auto iter = map_route_stop_point.find(key);
        if(iter == map_route_stop_point.end()) {
            iter = map_route_stop_point.insert(std::make_pair(key, vector_dt_st())).first;
        }

        iter->second.insert(iter->second.end(), stop_times.begin(), stop_times.end());
    }

    auto current_time = pt::second_clock::local_time();
    pt::time_period action_period(to_posix_time(handler.date_time, data), to_posix_time(handler.max_datetime, data));

    for(auto id_vec : map_route_stop_point) {

        auto board = handler.pb_response.add_departure_boards();
        fill_pb_object(data.pt_data.stop_points[id_vec.first.first], data, board->mutable_stop_point(), 1, current_time, action_period);
        fill_pb_object(data.pt_data.routes[id_vec.first.second], data, board->mutable_route(), 2, current_time, action_period);

        auto vec_st = id_vec.second;
        std::sort(vec_st.begin(), vec_st.end(),
                  [&](datetime_stop_time d1, datetime_stop_time d2) {
                    return std::abs((DateTimeUtils::hour(d1.first) % DateTimeUtils::SECONDS_PER_DAY)-DateTimeUtils::hour(handler.date_time))
                        <  std::abs((DateTimeUtils::hour(d2.first) % DateTimeUtils::SECONDS_PER_DAY)-DateTimeUtils::hour(handler.date_time));
                  });

        for(auto vec : make_columuns(vec_st)) {
            pbnavitia::BoardItem *item = board->add_board_items();

            for(DateTime dt : vec) {
                if(!item->has_hour())
                    item->set_hour(boost::lexical_cast<std::string>(DateTimeUtils::hour(dt)/3600));
                item->add_minutes(boost::lexical_cast<std::string>((DateTimeUtils::hour(dt)%3600)/60));
            }
        }
    }

    return handler.pb_response;
}





}

}
