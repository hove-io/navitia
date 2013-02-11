#include "departure_board.h"
#include "parse_request.h"
#include "get_stop_times.h"
#include "type/pb_converter.h"

#include "boost/lexical_cast.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
namespace navitia { namespace timetables {

std::vector<vector_datetime> make_columuns(const std::vector<dt_st> &stop_times) {
    std::vector<vector_datetime> result;
    routing::DateTime prev_date = routing::DateTime::inf;

    for(auto & item : stop_times) {
        if(prev_date == routing::DateTime:: inf || (prev_date.hour()/3600) != (item.first.hour()/3600)) {
            result.push_back(vector_datetime());
        } else if(prev_date == routing::DateTime::inf) {
            std::cout << item.first << " " << prev_date.hour() << " " << item.first.hour() << std::endl;
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
    
    for(type::idx_t route_point_idx : parser.route_points) {
        auto board = dep_board->add_boards();
        fill_pb_object(data.pt_data.route_points[route_point_idx].stop_point_idx, data, board->mutable_stop_point());
        fill_pb_object(data.pt_data.routes[data.pt_data.route_points[route_point_idx].route_idx].line_idx, data, board->mutable_line());
       
        auto stop_times = get_stop_times({route_point_idx}, parser.date_time, parser.max_datetime, std::numeric_limits<int>::max(), data);

        for(vector_datetime vec : make_columuns(stop_times)) {
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
