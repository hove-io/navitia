#pragma once

#include "type/pt_data.h"

namespace navitia { namespace routing {
using type::idx_t;

struct NotFound{};


/** On se crée une structure qui représente une date et heure
 *
 * Date : sous la forme d'un numéro de jour à chercher dans le validity pattern
 * Heure : entier en nombre de secondes depuis minuit. S'il dépasse minuit, on fait modulo 24h et on incrémente la date
 *
 * On utilise cette structure pendant le calcul d'itinéaire
 */
struct DateTime {

    /** Mesdames, Messieurs, votre attention s'il vous plaît
     * Pour des raisons de micro-optimisation, la date et l'heure sont codés sur un seul entier
     * Les 12 bits les plus forts codent les jours
     * Les 20 bits plus faibles codent l'heure
     *
     */
private:
    uint32_t datetime;
    const static uint32_t hour_mask = 0x000FFFFF;
    const static char date_offset = 20;

public:
    uint32_t hour() const {
        return datetime & hour_mask;
    }

    uint32_t date() const {
        return datetime >> date_offset;
    }

    static DateTime inf;
    static DateTime min;

    DateTime() : datetime(std::numeric_limits<uint32_t>::max()){}
    DateTime(int date, int hour) : datetime((date << date_offset) + hour) {}
    DateTime(const DateTime & dt) : datetime(dt.datetime) {}

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

    void normalize(){
        uint32_t hour = this->hour();
        if(hour > 24*3600) {
            *this = DateTime(this->date() + 1, hour % (24*3600));
        }
    }

    bool operator==(DateTime other) const {
        return this->datetime == other.datetime;
    }

    bool operator!=(DateTime other) const {
        return this->datetime != other.datetime;
    }


    uint32_t operator-(DateTime other) {
        return this->datetime - other.datetime;
    }

    void update(uint32_t hour) {
        int date = this->date();
        if(this->hour() > hour) {
            ++date;
            this->datetime = (date << date_offset) + hour;
        } else {
            this->datetime += (hour - this->hour());
        }

    }

    void updatereverse(uint32_t hour) {
        int date = this->date();
        if(this->hour() < hour) {
            --date;
            this->datetime = (date << date_offset) + hour;
        } else {
            this->datetime -= (this->hour() - hour);
        }

    }

    void increment(uint32_t secs){
        datetime += secs;
        this->normalize();
    }

    void decrement(uint32_t secs){
        uint32_t hour = this->hour();
        if(hour < secs){
            hour = hour + 24*3600 - secs;
            *this = DateTime(this->date() - 1, hour);
        } else {
            *this = DateTime(this->date(), hour - secs);
        }
    }

    void date_decrement(){
        datetime -= 1 << date_offset;
    }

    void date_increment(){
        datetime += 1 << date_offset;
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

std::ostream & operator<<(std::ostream & os, const DateTime & dt);



/** Représente un horaire associé à un validity pattern
 *
 * Il s'agit donc des horaires théoriques
 */
struct ValidityPatternTime {
    type::idx_t vp_idx;
    int hour;

    template<class T>
    bool operator<(T other) const {
        return hour < other.hour;
    }

    ValidityPatternTime() {}
    ValidityPatternTime(int vp_idx, int hour) : vp_idx(vp_idx), hour(hour){}
};


/** Étape d'un itinéraire*/
struct PathItem{
    type::idx_t said;
    DateTime arrival;
    DateTime departure;
    type::idx_t line_idx;
    type::idx_t route_idx;
    type::idx_t vj_idx;

    PathItem(type::idx_t said = type::invalid_idx , DateTime arrival = DateTime::infinity(), DateTime departure = DateTime::infinity(),
             type::idx_t line_idx = type::invalid_idx, type::idx_t route_idx = type::invalid_idx, type::idx_t vj_idx = type::invalid_idx) :
        said(said), arrival(arrival), departure(departure), line_idx(line_idx), route_idx(route_idx), vj_idx(vj_idx) {}
};

/** Un itinéraire complet */
struct Path {
    int duration;
    int nb_changes;
    int percent_visited;
    std::vector<PathItem> items;

    Path() : duration(0), nb_changes(0), percent_visited(0) {}
};

bool operator==(const PathItem & a, const PathItem & b);
std::ostream & operator<<(std::ostream & os, const PathItem & b);
std::ostream & operator<<(std::ostream & os, const Path & path);

/** Classe abstraite que tous les calculateurs doivent implémenter */
struct AbstractRouter {
    virtual Path compute(idx_t departure_idx, idx_t destination_idx, int departure_hour, int departure_day) = 0;
};

Path makeItineraire(const Path &path);



}}



namespace std {
template <>
class numeric_limits<navitia::routing::DateTime> {
public:
    static navitia::routing::DateTime max() {
        return navitia::routing::DateTime::infinity();
    }
};
}
