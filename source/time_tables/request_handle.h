#pragma once
#include "type/pb_converter.h"
#include "routing/routing.h"

namespace navitia { namespace timetables {

/** Parse et valide la date et durée et retrouve les journey_pattern_points associés au filtre

  Il est persistant tout le long de la requête
*/
struct RequestHandle {
    pbnavitia::Response pb_response;
    DateTime date_time, max_datetime;
    std::vector<type::idx_t> journey_pattern_points;
    int total_result;

    RequestHandle(const std::string &API, const std::string &request,
                  const std::string &change_time, uint32_t duration,
                  const type::Data & data, uint32_t count=-1, uint32_t start_page=-1);
};
}

}
