#pragma once
#include "routing/routing.h"
#include "type/data.h"


namespace navitia { namespace timetables {
typedef std::pair<DateTime, const type::StopTime*> datetime_stop_time;

/**
 * @brief get_stop_times: Return all departures leaving from the journey_pattern points
 * @param journey_pattern_points: list of the journey_pattern_point we want to start from
 * @param dt: departure date time
 * @param calendar_id: id of the calendar (optional)
 * @param max_dt: maximal Datetime
 * @param nb_departures: max number of departure
 * @param data: data container
 * @param accessibilite_params: accebility criteria to restrict the stop times
 * @return: a list of pair <datetime, departure st.idx>. The list is sorted on the datetimes.
 */
std::vector<datetime_stop_time> get_stop_times(const std::vector<type::idx_t> &journey_pattern_points, const DateTime &dt,
                                   const DateTime &max_dt, const size_t max_departures, const type::Data & data, bool disruption_active,
                                   boost::optional<const std::string> calendar_id = {},
                                   const type::AccessibiliteParams & accessibilite_params = type::AccessibiliteParams());



}}
