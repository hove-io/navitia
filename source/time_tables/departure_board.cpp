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

pbnavitia::Response stops_schedule(const std::string &request, std::string &date, std::string &date_changetime, type::Data &data) {
    boost::posix_time::ptime debut, change;
    if(date == "")
        debut = boost::posix_time::second_clock::local_time();
    else
        debut = boost::posix_time::from_iso_string(date);
    //On repositionne le début à minuit
    debut = debut - debut.time_of_day();


    if(date_changetime != "") {
        //Si le change est différent de minuit, on change le début
        debut = debut +  boost::posix_time::from_iso_string(date_changetime).time_of_day();
    }

    change = debut + boost::posix_time::time_duration(24,0,0);

    request_parser parser("DEPARTURE_BOARD", request, boost::posix_time::to_iso_string(debut),  boost::posix_time::to_iso_string(change), std::numeric_limits<int>::max()-1, data);


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
