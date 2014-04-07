#include "request_handle.h"
#include "ptreferential/ptreferential.h"
#include "type/meta_data.h"

namespace navitia { namespace timetables {

RequestHandle::RequestHandle(const std::string& /*api*/, const std::string &request,
                             const std::vector<std::string>& forbidden_uris,
                             const std::string &str_dt, uint32_t duration,
                             const type::Data &data) :
    date_time(DateTimeUtils::inf), max_datetime(DateTimeUtils::inf){
    try {
        auto ptime = boost::posix_time::from_iso_string(str_dt);
        if( !data.meta->production_date.contains(ptime.date()) ) {
            fill_pb_error(pbnavitia::Error::date_out_of_bounds, "date is out of bound",pb_response.mutable_error());
        } else if( !data.meta->production_date.contains((ptime + boost::posix_time::seconds(duration)).date()) ) {
             // On regarde si la date + duration ne déborde pas de la période de production
            fill_pb_error(pbnavitia::Error::date_out_of_bounds, "date is not in data production period",pb_response.mutable_error());
        }
        if(! pb_response.has_error()){
            date_time = DateTimeUtils::set((ptime.date() - data.meta->production_date.begin()).days(), ptime.time_of_day().total_seconds());
            max_datetime = date_time + duration;
            const auto jpp_t = type::Type_e::JourneyPatternPoint;
            journey_pattern_points = ptref::make_query(jpp_t, request, forbidden_uris, data);
            total_result = journey_pattern_points.size();
        }
    } catch(const ptref::parsing_error &parse_error) {
        fill_pb_error(pbnavitia::Error::unable_to_parse, "Unable to parse Datetime" + parse_error.more,pb_response.mutable_error());
    } catch(const ptref::ptref_error &ptref_error){
        fill_pb_error(pbnavitia::Error::bad_filter, "ptref : "  + ptref_error.more,pb_response.mutable_error());
    } catch(...) {
        fill_pb_error(pbnavitia::Error::unable_to_parse, "Unable to parse Datetime",pb_response.mutable_error());
  }
}

}}
