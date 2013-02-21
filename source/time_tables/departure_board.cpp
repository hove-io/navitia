#include "departure_board.h"
#include "parse_request.h"
#include "get_stop_times.h"
#include "type/pb_converter.h"

#include "boost/lexical_cast.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
namespace navitia { namespace timetables {

std::vector<vector_datetime> make_columuns(const vector_dt_st &stop_times) {
    std::vector<vector_datetime> result;
    routing::DateTime prev_date = routing::DateTime::inf;

    for(auto & item : stop_times) {
        if(prev_date == routing::DateTime:: inf || (prev_date.hour()/3600) != (item.first.hour()/3600)) {
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

    request_parser parser("DEPARTURE_BOARD", request, date,  duration, data);


    parser.pb_response.set_requested_api(pbnavitia::DEPARTURE_BOARD);
    if(parser.pb_response.has_error())
        return parser.pb_response;

    if(parser.route_points.size() == 0)
        return parser.pb_response;

    pbnavitia::DepartureBoard * dep_board = parser.pb_response.mutable_departure_board();

    std::map<stop_point_line, vector_dt_st> map_line_stop_point;
    // On regroupe entre eux les stop_times appartenant au meme couple (stop_point, line)
    for(type::idx_t route_point_idx : parser.route_points) {
        auto stop_point_idx = data.pt_data.route_points[route_point_idx].stop_point_idx;
        auto line_idx = data.pt_data.routes[data.pt_data.route_points[route_point_idx].route_idx].line_idx;
       
        auto stop_times = get_stop_times({route_point_idx}, parser.date_time, parser.max_datetime, std::numeric_limits<int>::max(), data);

        auto key = std::make_pair(stop_point_idx, line_idx);
        auto iter = map_line_stop_point.find(key);
        if(iter == map_line_stop_point.end()) {
            iter = map_line_stop_point.insert(std::make_pair(key, vector_dt_st())).first;
        }

        iter->second.insert(iter->second.end(), stop_times.begin(), stop_times.end());
    }


    for(auto id_vec : map_line_stop_point) {
        auto board = dep_board->add_boards();
        fill_pb_object(id_vec.first.first, data, board->mutable_stop_point());
        fill_pb_object(id_vec.first.second, data, board->mutable_line());

        auto vec_st = id_vec.second;
        std::sort(vec_st.begin(), vec_st.end(),
                  [](dt_st d1, dt_st d2) {
                    return (d1.first.hour() / routing::DateTime::NB_SECONDS_DAY) < (d2.first.hour() / routing::DateTime::NB_SECONDS_DAY);
                  });

        for(auto vec : make_columuns(vec_st)) {
            pbnavitia::BoardItem *item = board->add_board_items();

            for(routing::DateTime dt : vec) {
                if(!item->has_hour())
                    item->set_hour(boost::lexical_cast<std::string>(dt.hour()/3600));
                item->add_minutes(boost::lexical_cast<std::string>((dt.hour()%3600)/60));
            }
        }
    }

    return parser.pb_response;
}





}

}
