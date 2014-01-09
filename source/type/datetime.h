#pragma once
#include <cstdint>
#include <string>
#include <limits>
#include <fstream>

//#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_config.hpp>

namespace boost{ namespace posix_time{ class ptime; class time_duration; }}

namespace navitia{
typedef uint32_t DateTime;

namespace DateTimeUtils{

    const uint32_t SECONDS_PER_DAY = 86400;

    const navitia::DateTime inf = std::numeric_limits<uint32_t>::max();
    const navitia::DateTime min = 0;

    inline navitia::DateTime set(int date, int hour) {
        return date * SECONDS_PER_DAY + hour;
    }

    inline uint32_t hour(navitia::DateTime datetime) {
        return datetime%SECONDS_PER_DAY;
    }

    inline uint32_t date(navitia::DateTime datetime) {
        return datetime/SECONDS_PER_DAY;
    }

    inline void update(navitia::DateTime & datetime, uint32_t hh, bool clockwise = true) {
        uint32_t this_hour = hour(datetime);
        if(hh>=SECONDS_PER_DAY)
            hh -= SECONDS_PER_DAY;
        if(clockwise){
            datetime += (hh>=this_hour ?0:SECONDS_PER_DAY) + hh - this_hour;
        }
        else {
            if(hh<=this_hour)
                datetime += hh - this_hour;
            else {
                if(date(datetime) > 0)
                    datetime += hh - this_hour - SECONDS_PER_DAY;
                else
                    datetime = 0;
            }
        }
    }

}
namespace type {
class Data;
}

//std::ostream & operator<<(std::ostream & os, const DateTime &dt);
std::string str(const DateTime &dt);
std::string iso_string(const DateTime dt, const type::Data &d);
boost::posix_time::ptime to_posix_time(DateTime datetime, const type::Data &d);

inline DateTime operator-(DateTime time, boost::posix_time::time_duration dur) {
    return time - dur.total_seconds();
}
inline DateTime operator+(DateTime time, boost::posix_time::time_duration dur) {
    return time + dur.total_seconds();
}

}
