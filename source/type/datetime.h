#pragma once
#include <cstdint>
#include <limits>
#include <string>
#include <fstream>

#include <boost/date_time/posix_time/posix_time.hpp>

namespace navitia { namespace type {

struct Data;    
/** On se crée une structure qui représente une date et heure
 *
 * Date : sous la forme d'un numéro de jour à chercher dans le validity pattern
 * Heure : entier en nombre de secondes depuis minuit. S'il dépasse minuit, on fait modulo 24h et on incrémente la date
 *
 * On utilise cette structure pendant le calcul d'itinéaire
 */
struct DateTime {

    uint32_t datetime;

    static const uint32_t NB_SECONDS_DAY = 86400;

    static DateTime inf;
    static DateTime min;

    DateTime() : datetime(std::numeric_limits<uint32_t>::max()){}
    DateTime(int date, int hour) : datetime(date*NB_SECONDS_DAY + hour) {}
    DateTime(const DateTime & dt) : datetime(dt.datetime) {}

    uint32_t hour() const {
        return datetime%NB_SECONDS_DAY;
    }

    uint32_t date() const {
        return datetime/NB_SECONDS_DAY;
    }


    bool operator<(DateTime other) const {
        return this->datetime < other.datetime;
    }

    bool operator<=(DateTime other) const {
        return this->datetime <= other.datetime;
    }

    bool operator>(DateTime other) const {
        return (this->datetime > other.datetime) && (other.datetime != std::numeric_limits<uint32_t>::max());
    }

    bool operator>=(DateTime other) const {
        return this->datetime >= other.datetime;
    }

    static DateTime infinity() {
        return DateTime();
    }

    static DateTime minimity() {
        return DateTime(0,0);
    }

    bool operator==(DateTime other) const {
        return this->datetime == other.datetime;
    }

    bool operator!=(DateTime other) const {
        return this->datetime != other.datetime;
    }


    uint32_t operator-(DateTime other) {
        return datetime - other.datetime;
    }

    void update(uint32_t hour, bool clockwise = true) {
        if(hour>=NB_SECONDS_DAY)
            hour -= NB_SECONDS_DAY;
        if(clockwise){
            datetime += ((hour>=this->hour())?0:NB_SECONDS_DAY) + hour - this->hour();
        } else {
            if(hour<=this->hour())
                datetime += hour - this->hour();
            else {
                if(this->date() > 0)
                    datetime += hour - this->hour() - NB_SECONDS_DAY;
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

    void date_decrement(){
        //datetime -= 1 << date_offset;
        datetime -= NB_SECONDS_DAY;
    }

    void date_increment(){
        //datetime += 1 << date_offset;
        datetime += NB_SECONDS_DAY;
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

inline int operator+(const DateTime &dt1, const DateTime &dt2) {
    return dt1.datetime + dt2.datetime;
}

inline int operator-(const DateTime &dt1, const DateTime &dt2) {
    return dt1.datetime - dt2.datetime;
}

std::ostream & operator<<(std::ostream & os, const DateTime & dt);


std::string iso_string(int date, int hour, const Data &d);
std::string iso_string(DateTime &datetime, const Data &d);
boost::posix_time::ptime to_posix_time(DateTime &datetime, const Data &d);

} }



namespace std {
template <>
class numeric_limits<navitia::type::DateTime> {
public:
    static navitia::type::DateTime max() {
        return navitia::type::DateTime::infinity();
    }
};
}
