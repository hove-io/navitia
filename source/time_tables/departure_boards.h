#pragma once
#include "type/pb_converter.h"
#include "routing/routing.h"
#include "get_stop_times.h"


namespace navitia { namespace timetables {
typedef std::vector<DateTime> vector_datetime;
typedef std::pair<uint32_t, uint32_t> stop_point_line;
typedef std::vector<datetime_stop_time> vector_dt_st;

pbnavitia::Response departure_board(const std::string &filter,
                                    boost::optional<const std::string> calendar_id,
                                    const std::vector<std::string>& forbidden_uris,
                                    const std::string &date,
                                    uint32_t duration,
                                    uint32_t depth, int32_t max_date_times,
                                    int interface_version,
                                    int count, int start_page, const type::Data &data, bool disruption_active,
                                    bool show_codes);
}

}
