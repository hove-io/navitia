#include "request_handle.h"
#include "ptreferential/ptreferential.h"

namespace navitia { namespace timetables {

RequestHandle::RequestHandle(const std::string &API, const std::string &request,
                             const std::string &str_dt, uint32_t duration, const type::Data & data) :
    date_time(DateTimeUtils::inf), max_datetime(DateTimeUtils::inf){

    try {
        auto ptime = boost::posix_time::from_iso_string(str_dt);
        if( !data.meta.production_date.contains(ptime.date()) ) {
            pb_response.set_error(API + " Date out of the production period: " + str_dt);
        } else if( !data.meta.production_date.contains((ptime + boost::posix_time::seconds(duration)).date()) ) {
             // On regarde si la date + duration ne déborde pas de la période de production
            pb_response.set_error(API + " Date + duration is out of the production period: " + str_dt);
        }

        date_time = DateTimeUtils::set((ptime.date() - data.meta.production_date.begin()).days(), ptime.time_of_day().total_seconds());
        max_datetime = date_time + duration;
        journey_pattern_points = navitia::ptref::make_query(type::Type_e::JourneyPatternPoint, request, data);

    } catch(const ptref::parsing_error &parse_error) {
        pb_response.set_error(parse_error.more);
    } catch(...) {
        pb_response.set_error(API + " Unable to parse Datetime: " + str_dt);
    }

}


}}
