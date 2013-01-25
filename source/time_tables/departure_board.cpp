#include "departure_board.h"
#include "parse_request.h"
#include "get_stop_times.h"
#include "type/pb_converter.h"

#include "boost/lexical_cast.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
namespace navitia { namespace timetables {

std::vector<vector_datetime> make_columuns(const std::vector<dt_st> &stop_times) {
    std::vector<vector_datetime> result;
    routing::DateTime &prev_date = routing::DateTime::inf;

    for(auto & item : stop_times) {
        if((prev_date.hour()/3600) != (item.first.hour()/3600)) {
            result.push_back(vector_datetime());
        }
        result.back().push_back(item.first);
        prev_date = item.first;
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
    fill_pb_object(data.pt_data.route_points[parser.route_points.front()].stop_point_idx, data, dep_board->mutable_stop_point());
    fill_pb_object(data.pt_data.routes[data.pt_data.route_points[parser.route_points.front()].route_idx].line_idx, data, dep_board->mutable_line());

    pbnavitia::Board * board = dep_board->mutable_board();



    auto stop_times = get_stop_times(parser.route_points, parser.date_time, parser.max_datetime, std::numeric_limits<int>::max(), data);

    for(vector_datetime vec : make_columuns(stop_times)) {
        pbnavitia::BoardItem *item = board->add_board_item();

        for(routing::DateTime dt : vec) {
            if(!item->has_hour())
                item->set_hour(boost::lexical_cast<std::string>(dt.hour()/3600));
            item->add_minute(boost::lexical_cast<std::string>((dt.hour()%3600)/60));
        }
    }


    return parser.pb_response;
}





}

}
