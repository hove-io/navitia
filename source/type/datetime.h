#pragma once
#include <cstdint>
#include <string>
#include <limits>
#include <fstream>

//#include <boost/date_time/posix_time/posix_time.hpp>

namespace boost{ namespace posix_time{ class ptime; }}

namespace navitia { namespace type {

struct Data;    
/** On se crée une structure qui représente une date et heure
 *
 * C’est représenté par le nombre de secondes à partir de la période de production
 */
struct DateTime {

    uint32_t datetime;

    static const uint32_t SECONDS_PER_DAY = 86400;

    static const DateTime inf;
    static const DateTime min;

    constexpr DateTime() : datetime(std::numeric_limits<uint32_t>::max()){}
    constexpr DateTime(int date, int hour) : datetime(date*SECONDS_PER_DAY + hour) {}
    DateTime(const DateTime & dt) : datetime(dt.datetime) {}

    uint32_t hour() const {
        return datetime%SECONDS_PER_DAY;
    }

    uint32_t date() const {
        return datetime/SECONDS_PER_DAY;
    }


    bool operator<(const DateTime &other) const {
        return this->datetime < other.datetime;
    }

    bool operator<=(const DateTime &other) const {
        return this->datetime <= other.datetime;
    }

    bool operator>(const DateTime  & other) const {
        return this->datetime > other.datetime;
    }

    bool operator>=(const DateTime &other) const {
        return this->datetime >= other.datetime;
    }

    bool operator==(const DateTime &other) const {
        return this->datetime == other.datetime;
    }

    bool operator!=(const DateTime  &other) const {
        return this->datetime != other.datetime;
    }

    void update(uint32_t hour, bool clockwise = true) {
        uint32_t this_hour = this->hour();
        if(hour>=SECONDS_PER_DAY)
            hour -= SECONDS_PER_DAY;
        if(clockwise){
            datetime += ( hour>=this_hour ?0:SECONDS_PER_DAY) + hour - this_hour;
        } else {
            if(hour<=this_hour)
                datetime += hour - this_hour;
            else {
                if(this->date() > 0)
                    datetime += hour - this_hour - SECONDS_PER_DAY;
                else
                    datetime = 0;
            }
        }
    }

    void increment(uint32_t secs){
        datetime += secs;
    }

    void decrement(uint32_t secs){
        datetime -= secs;
    }
};


inline DateTime operator+(DateTime dt, int seconds) {
    dt.increment(seconds);
    return dt;
}

inline DateTime operator-(DateTime dt, int seconds) {
    dt.decrement(seconds);
    return dt;
}

inline int operator-(const DateTime &dt1, const DateTime &dt2) {
    return dt1.datetime - dt2.datetime;
}

std::ostream & operator<<(std::ostream & os, const DateTime & dt);


std::string iso_string(int date, int hour, const Data &d);
std::string iso_string(DateTime &datetime, const Data &d);
boost::posix_time::ptime to_posix_time(DateTime &datetime, const Data &d);

} }
