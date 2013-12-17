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

    ValidityPatternTime() : vp_idx(type::invalid_idx), hour(-1) {}
    ValidityPatternTime(int vp_idx, int hour) : vp_idx(vp_idx), hour(hour){}
};


enum ItemType {
    public_transport,
    walking,
    stay_in,
    guarantee,
    waiting
};

/** Étape d'un itinéraire*/
struct PathItem{
    navitia::DateTime arrival;
    navitia::DateTime departure;
    std::vector<navitia::DateTime> arrivals;
    std::vector<navitia::DateTime> departures;
    std::vector<size_t> orders;
    type::idx_t vj_idx;
    std::vector<type::idx_t> stop_points;
    ItemType type;

    PathItem(navitia::DateTime departure = navitia::DateTimeUtils::inf, navitia::DateTime arrival = navitia::DateTimeUtils::inf,
            type::idx_t vj_idx = type::invalid_idx) :
        arrival(arrival), departure(departure), vj_idx(vj_idx), type(public_transport) {
            if(departure != navitia::DateTimeUtils::inf)
                departures.push_back(departure);
            if(arrival != navitia::DateTimeUtils::inf)
                arrivals.push_back(arrival);
        }

    std::string print(const navitia::type::PT_Data & data) const;
};

/** Un itinéraire complet */
struct Path {
    uint32_t duration;
    uint32_t nb_changes;
    boost::posix_time::ptime request_time;
    std::vector<PathItem> items;

    Path() : duration(std::numeric_limits<uint32_t>::max()), nb_changes(std::numeric_limits<uint32_t>::max()) {}

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


}}


