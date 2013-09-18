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
                auto last = result.back();
                auto it = std::unique(last.begin(), last.end());
                last.resize(std::distance(last.begin(), it));
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


pbnavitia::Response
render_v0(const std::map<stop_point_line, vector_dt_st> &map_route_stop_point,
          DateTime datetime, DateTime max_datetime,
          const type::Data &data) {
    pbnavitia::Response response;
    auto current_time = pt::second_clock::local_time();
    pt::time_period action_period(to_posix_time(datetime, data),
                                  to_posix_time(max_datetime, data));

    auto sort_predicate =
        [&](datetime_stop_time d1, datetime_stop_time d2) {
            auto hour1 = DateTimeUtils::hour(d1.first) % DateTimeUtils::SECONDS_PER_DAY;
            auto hour2 = DateTimeUtils::hour(d2.first) % DateTimeUtils::SECONDS_PER_DAY;
            return std::abs(hour1 - DateTimeUtils::hour(datetime)) <
                   std::abs(hour2 - DateTimeUtils::hour(datetime));
        };

    for(auto id_vec : map_route_stop_point) {

        auto board = response.add_departure_boards();
        fill_pb_object(data.pt_data.stop_points[id_vec.first.first], data,
                       board->mutable_stop_point(), 1,
                       current_time, action_period);
        fill_pb_object(data.pt_data.routes[id_vec.first.second], data,
                       board->mutable_route(), 2, current_time, action_period);

        auto vec_st = id_vec.second;
        std::sort(vec_st.begin(), vec_st.end(), sort_predicate);

        for(auto vec : make_columuns(vec_st)) {
            pbnavitia::BoardItem *item = board->add_board_items();

            for(DateTime dt : vec) {
                if(!item->has_hour()) {
                    auto hours = DateTimeUtils::hour(dt)/3600;
                    item->set_hour(boost::lexical_cast<std::string>(hours));
                }
                auto minutes = (DateTimeUtils::hour(dt)%3600)/60;
                item->add_minutes(boost::lexical_cast<std::string>(minutes));
            }
        }
    }
    return response;
}


pbnavitia::Response
render_v1(const std::map<stop_point_line, vector_dt_st> &map_route_stop_point,
          DateTime datetime, DateTime max_datetime,
          const type::Data &data) {
    pbnavitia::Response response;
    auto current_time = pt::second_clock::local_time();
    auto now = pt::second_clock::local_time();
    pt::time_period action_period(to_posix_time(datetime, data),
                                  to_posix_time(max_datetime, data));

    for(auto id_vec : map_route_stop_point) {
        auto schedule = response.add_stop_schedules();
        //Each schedule has a stop_point and a route
        fill_pb_object(data.pt_data.stop_points[id_vec.first.first], data,
                       schedule->mutable_stop_point(), 0,
                       current_time, action_period);
//        auto m_route = schedule->mutable_route();
//        fill_pb_object(data.pt_data.routes[id_vec.first.second], data,
//                       m_route, 0, current_time, action_period);
        auto pt_display_information = schedule->mutable_pt_display_informations();
        fill_pb_object(data.pt_data.routes[id_vec.first.second], data,
                               pt_display_information, 0, current_time, action_period);

//        if(data.pt_data.routes[id_vec.first.second]->line != nullptr){
//            auto m_line = m_route->mutable_line();
//            fill_pb_object(data.pt_data.routes[id_vec.first.second]->line, data, m_line, 0, now, action_period);
//            if(data.pt_data.routes[id_vec.first.second]->line->commercial_mode){
//                auto m_commercial_mode = m_line->mutable_commercial_mode();
//                fill_pb_object(data.pt_data.routes[id_vec.first.second]->line->commercial_mode,
//                               data, m_commercial_mode, 0);
//            }
//            if(data.pt_data.routes[id_vec.first.second]->line->network){
//                auto m_network = m_line->mutable_network();
//                fill_pb_object(data.pt_data.routes[id_vec.first.second]->line->network, data, m_network, 0);
//            }
//        }
        //Now we fill the stop_date_times
        for(auto dt_st : id_vec.second) {
            auto stop_date_time = schedule->add_stop_date_times();
            fill_pb_object(dt_st.second, data, stop_date_time, 0,
                           now, action_period, dt_st.first);
        }
    }
    return response;
}


pbnavitia::Response
departure_board(const std::string &request, const std::string &date,
                uint32_t duration, int interface_version,
                int count, int start_page, const type::Data &data) {

    RequestHandle handler("DEPARTURE_BOARD", request, date,  duration, data,
                         count, start_page);

    if(handler.pb_response.has_error())
        return handler.pb_response;

    if(handler.journey_pattern_points.size() == 0)
        return handler.pb_response;

    std::map<stop_point_line, vector_dt_st> map_route_stop_point;
    // On regroupe entre eux les stop_times appartenant
    // au meme couple (stop_point, route)
    // On veut en effet afficher les départs regroupés par route
    // (une route étant une vague direction commerciale
    for(type::idx_t jpp_idx : handler.journey_pattern_points) {
        auto jpp = data.pt_data.journey_pattern_points[jpp_idx];
        const type::StopPoint* stop_point = jpp->stop_point;
        const type::Route* route = jpp->journey_pattern->route;

        auto stop_times = get_stop_times({jpp_idx}, handler.date_time,
                                         handler.max_datetime,
                                         std::numeric_limits<int>::max(), data);

        auto key = std::make_pair(stop_point->idx, route->idx);
        auto iter = map_route_stop_point.find(key);
        if(iter == map_route_stop_point.end()) {
            auto to_insert = std::make_pair(key, vector_dt_st());
            iter = map_route_stop_point.insert(to_insert).first;
        }

        iter->second.insert(iter->second.end(),
                            stop_times.begin(), stop_times.end());
    }

    if(interface_version == 0) {
        handler.pb_response = render_v0(map_route_stop_point,
                                        handler.date_time,
                                        handler.max_datetime, data);
    } else if(interface_version == 1) {
        handler.pb_response = render_v1(map_route_stop_point,
                                        handler.date_time,
                                        handler.max_datetime, data);
    }


    auto pagination = handler.pb_response.mutable_pagination();
    pagination->set_totalresult(handler.total_result);
    pagination->set_startpage(start_page);
    pagination->set_itemsperpage(count);
    pagination->set_itemsonpage(std::max(handler.pb_response.departure_boards_size(),
                                         handler.pb_response.stop_schedules_size()));

    return handler.pb_response;
}
}
}
