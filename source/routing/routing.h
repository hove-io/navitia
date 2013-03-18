#pragma once

#include "type/pt_data.h"
#include "type/datetime.h"

namespace navitia { namespace routing {
    using type::idx_t;

struct NotFound{};




/** Représente un horaire associé à un validity pattern
 *
 * Il s'agit donc des horaires théoriques
 */
struct ValidityPatternTime {
    idx_t vp_idx;
    int hour;

    template<class T>
    bool operator<(T other) const {
        return hour < other.hour;
    }

    ValidityPatternTime() {}
    ValidityPatternTime(int vp_idx, int hour) : vp_idx(vp_idx), hour(hour){}
};


enum ItemType {
    public_transport,
    walking,
    extension,
    guarantee,
    waiting
};

/** Étape d'un itinéraire*/
struct PathItem{
    navitia::type::DateTime arrival;
    navitia::type::DateTime departure;
    std::vector<navitia::type::DateTime> arrivals;
    std::vector<navitia::type::DateTime> departures;
    type::idx_t vj_idx;
    std::vector<type::idx_t> stop_points;
    ItemType type;

    PathItem(navitia::type::DateTime departure = navitia::type::DateTime::infinity(), navitia::type::DateTime arrival = navitia::type::DateTime::infinity(),
            type::idx_t vj_idx = type::invalid_idx) :
        arrival(arrival), departure(departure), vj_idx(vj_idx) {
            if(departure != navitia::type::DateTime::inf)
                departures.push_back(departure);
            if(arrival != navitia::type::DateTime::inf)
                arrivals.push_back(arrival);
        }

    std::string print(const navitia::type::PT_Data & data) const;
};

/** Un itinéraire complet */
struct Path {
    int duration;
    int nb_changes;
    int percent_visited;
    boost::posix_time::ptime request_time;
    std::vector<PathItem> items;

    Path() : duration(0), nb_changes(0), percent_visited(0) {}

    void print(const navitia::type::PT_Data & data) const {
        for(auto item : items)
            std::cout << item.print(data) << std::endl;
    }

};

bool operator==(const PathItem & a, const PathItem & b);

class Verification {

public :
    const navitia::type::PT_Data & data;
    Verification(const navitia::type::PT_Data & data) : data(data) {}

    bool verif(Path path);
    bool croissance(Path path);
    bool vj_valides(Path path);
    bool appartenance_rp(Path path);
    bool check_correspondances(Path path);

};

/** Classe abstraite que tous les calculateurs doivent implémenter */
struct AbstractRouter {
    virtual std::vector<Path> compute(idx_t departure_idx, idx_t destination_idx, int departure_hour, int departure_day, bool clockwise = true, const type::Properties &required_properties=0) = 0;
    virtual ~AbstractRouter() {}
};

}}


