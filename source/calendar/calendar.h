#pragma once
#include "type/type.h"
#include "type/pt_data.h"
#include "utils/logger.h"
#include "boost/date_time/posix_time/posix_time.hpp"

namespace navitia { namespace calendar {
class Calendar{
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
public:
    // Return idx calendars list
    std::vector<type::idx_t> get_calendars(const std::string& filter,
                    const std::vector<std::string>& forbidden_uris,
                    const type::Data &d,
                    const boost::gregorian::date_period filter_period,
                    const boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time());
};
}
}
