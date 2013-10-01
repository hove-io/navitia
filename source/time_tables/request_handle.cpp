#include "request_handle.h"
#include "ptreferential/ptreferential.h"

namespace navitia { namespace timetables {

RequestHandle::RequestHandle(const std::string &API, const std::string &request,
                             const std::string &str_dt, uint32_t duration,
                             const type::Data &data) :
    date_time(DateTimeUtils::inf), max_datetime(DateTimeUtils::inf){
    std::string error = API + " ";

    try {
        auto ptime = boost::posix_time::from_iso_string(str_dt);
        if( !data.meta.production_date.contains(ptime.date()) ) {
            error +="Date out of the production period: " + str_dt;
            pb_response.set_error(error);
        } else if( !data.meta.production_date.contains((ptime + boost::posix_time::seconds(duration)).date()) ) {
             // On regarde si la date + duration ne déborde pas de la période de production
            error += "Date + duration is out of the production period : "+str_dt;
            pb_response.set_error(error);
        }

        date_time = DateTimeUtils::set((ptime.date() - data.meta.production_date.begin()).days(), ptime.time_of_day().total_seconds());
        max_datetime = date_time + duration;
        const auto jpp_t = type::Type_e::JourneyPatternPoint;
        journey_pattern_points = ptref::make_query(jpp_t, request, data);
        total_result = journey_pattern_points.size();
    } catch(const ptref::parsing_error &parse_error) {
        pb_response.set_error(parse_error.more);
    } catch(...) {
        error += "Unable to parse Datetime: " + str_dt;
        pb_response.set_error(error);
    }
}

}}
