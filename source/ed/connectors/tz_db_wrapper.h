#pragma once

#include <boost/date_time/time_zone_base.hpp>
#include <boost/date_time/local_time/local_time.hpp>

namespace ed {

namespace utils {

/*
 * Uggly but necessary wrapper for the moment
 *
 * Unfortunately the tz db is not supported by boost::date_time (it is done with boost::locale but we don't want to pay
 * the cost of this module).
 *
 * boost::date_time can only be fed with a csv or with code.
 * We don't want to bother the delivery of the file with the binary so we are stuck with the hardcoding of the timezone
 * data
 *
 * The clean way would be to change boost to read the tz db and we'll see if we can manage some time to do that in the
 * future.
 *
 */
boost::local_time::tz_database fill_tz_db();

}  // namespace utils
}  // namespace ed
