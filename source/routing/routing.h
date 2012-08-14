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
    // TODO : on pourrait optimiser la conso mémoire en utilisant 8 bits pour la date, et 24 pour l'heure ;)
    int date;
    int hour;

    DateTime() : date(std::numeric_limits<int>::max()), hour(std::numeric_limits<int>::max()){}
    DateTime(int date, int hour) : date(date), hour(hour) {}
    DateTime(const DateTime & dt) : date(dt.date), hour(dt.hour) {}

    bool operator<(DateTime other) const {
        if(this->date == other.date)
            return hour < other.hour;
        else
            return this->date < other.date;
    }


    bool operator>(DateTime other) const {
        if(this->date == other.date)
            return hour > other.hour;
        else
            return this->date > other.date;
    }

    bool operator>=(DateTime other) const {
        if(this->date == other.date)
            return hour >= other.hour;
        else
            return this->date > other.date;
    }

    bool operator<=(DateTime other) const {
        if(this->date == other.date)
            return hour <= other.hour;
        else
            return this->date < other.date;
    }

    static DateTime infinity() {
        return DateTime();
    }

    static DateTime minimity() {
        return DateTime(std::numeric_limits<int>::min(), std::numeric_limits<int>::min());
    }

    void normalize(){
        if(date > 366) std::cout << "on normalise l'infini..." << std::endl;
        if(this->hour < 0) {
            --this->date;
            this->hour = hour+ (24*3600);
        } else {

            this->date += this->hour / (24*3600);
            this->hour = hour % (24*3600);
        }
    }

    bool operator==(DateTime other) const {
        return this->hour == other.hour && this->date == other.date;
    }

    int operator-(DateTime other) {
        return (this->date - other.date) * 86400 + this->hour - other.hour;
    }
};

std::ostream & operator<<(std::ostream & os, const DateTime & dt);

DateTime operator+(DateTime dt, int seconds);
DateTime operator-(DateTime dt, int seconds);

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
             type::idx_t line_idx = type::invalid_idx, type::idx_t = type::invalid_idx, type::idx_t vj_idx = type::invalid_idx) :
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
